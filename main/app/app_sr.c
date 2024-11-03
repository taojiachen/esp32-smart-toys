/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "app_sr.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "app_sr_handler.h"
#include "model_path.h"
#include "esp_mn_speech_commands.h"
#include "esp_process_sdkconfig.h"
#include <esp_board_init.h>

#define I2S_CHANNEL_NUM (1)
#define BUFFER_SIZE (1024)

static const char *TAG = "app_sr";

static model_iface_data_t *model_data = NULL;
static const esp_mn_iface_t *multinet = NULL;
static const esp_afe_sr_iface_t *afe_handle = NULL;
static QueueHandle_t g_result_que = NULL;
static srmodel_list_t *models = NULL;

const char *cmd_phoneme[12] = {
    "da kai kong qi jing hua qi",
    "guan bi kong qi jing hua qi",
    "da kai tai deng",
    "guan bi tai deng",
    "tai deng tiao liang",
    "tai deng tiao an",
    "da kai deng dai",
    "guan bi deng dai",
    "bo fang yin yue",
    "ting zhi bo fang",
    "da kai shi jian",
    "da kai ri li"};

static void audio_feed_task(void *pvParam)
{
    size_t bytes_read = 0;
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *)pvParam;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    ESP_LOGI(TAG, "audio_chunksize=%d, feed_channel=%d", audio_chunksize, 3);

    /* Allocate audio buffer and check for result */
    int16_t *max98357a_data = heap_caps_malloc(audio_chunksize * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    int32_t *audio_buffer = heap_caps_malloc(audio_chunksize * sizeof(int16_t) * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (NULL == audio_buffer)
    {
        esp_system_abort("No mem for audio buffer");
    }

    while (true)
    {
        /* Read audio data from I2S bus */
        esp_err_t read_result = esp_i2s_read(audio_buffer, audio_chunksize * sizeof(int32_t));
        //printf("audio_chunksize * sizeof(int16_t)   %d", audio_chunksize * sizeof(int16_t));
        if (read_result != ESP_OK)
        {
            ESP_LOGE(TAG, "======== bsp_extra_i2s_read failed ==========");
        }
        /* Feed samples of an audio stream to the AFE_SR */
        for (int i = 0; i < audio_chunksize * sizeof(int32_t) / sizeof(int32_t); i++)
        {
            // 右移16位将32位数据转换为16位
            max98357a_data[i] = (int16_t)(audio_buffer[i] >> 16);
        }
        afe_handle->feed(afe_data, max98357a_data);
        // ESP_ERROR_CHECK(esp_audio_play(max98357a_data,
        //                                audio_chunksize * sizeof(int16_t) // 因为从32位转换到16位，所以字节数减半
        //                                ));
        // esp_audio_play(audio_buffer,1024*audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t));
    }

    /* Clean up if audio feed ends */
    afe_handle->destroy(afe_data);
    /* Task never returns */
    vTaskDelete(NULL);
}

static void audio_detect_task(void *pvParam)
{
    bool detect_flag = false;
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *)pvParam;

    /* Check if audio data has same chunksize with multinet */
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    int mu_chunksize = multinet->get_samp_chunksize(model_data);
    //printf("afe_chunksize  %d   mu_chunksize  %d", afe_chunksize, mu_chunksize);
    assert(mu_chunksize == afe_chunksize);
    ESP_LOGI(TAG, "------------detect start------------\n");

    while (true)
    {
        afe_fetch_result_t *res = afe_handle->fetch(afe_data);

        if (!res || res->ret_value == ESP_FAIL)
        {
            ESP_LOGE(TAG, "fetch error!");
            continue;
        }

        if (res->wakeup_state == WAKENET_DETECTED)
        {
            ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Wakeword detected");
            sr_result_t result = {
                .wakenet_mode = WAKENET_DETECTED,
                .state = ESP_MN_STATE_DETECTING,
                .command_id = 0,
            };
            xQueueSend(g_result_que, &result, 10);
        }
        else if (res->wakeup_state == WAKENET_CHANNEL_VERIFIED)
        {
            ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Channel verified");
            detect_flag = true;
            afe_handle->disable_wakenet(afe_data);
        }

        if (true == detect_flag)
        {
            esp_mn_state_t mn_state = ESP_MN_STATE_DETECTING;

            mn_state = multinet->detect(model_data, res->data);

            if (ESP_MN_STATE_DETECTING == mn_state)
            {
                continue;
            }

            if (ESP_MN_STATE_TIMEOUT == mn_state)
            {
                ESP_LOGW(TAG, "Time out");
                sr_result_t result = {
                    .wakenet_mode = WAKENET_NO_DETECT,
                    .state = mn_state,
                    .command_id = 0,
                };
                xQueueSend(g_result_que, &result, 10);
                afe_handle->enable_wakenet(afe_data);
                detect_flag = false;
                continue;
            }

            if (ESP_MN_STATE_DETECTED == mn_state)
            {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                for (int i = 0; i < mn_result->num; i++)
                {
                    ESP_LOGI(TAG, "TOP %d, command_id: %d, phrase_id: %d, prob: %f",
                             i + 1, mn_result->command_id[i], mn_result->phrase_id[i], mn_result->prob[i]);
                }

                int sr_command_id = mn_result->command_id[0];
                ESP_LOGI(TAG, "Deteted command : %d", sr_command_id);
                sr_result_t result = {
                    .wakenet_mode = WAKENET_NO_DETECT,
                    .state = mn_state,
                    .command_id = sr_command_id,
                };
                xQueueSend(g_result_que, &result, 10);
#if !SR_CONTINUE_DET
                afe_handle->enable_wakenet(afe_data);
                detect_flag = false;
#endif
                continue;
            }
            ESP_LOGE(TAG, "Exception unhandled");
        }
    }

    /* Clean up if audio feed ends */
    afe_handle->destroy(afe_data);

    /* Task never returns */
    vTaskDelete(NULL);
}

esp_err_t app_sr_start(void)
{
    g_result_que = xQueueCreate(1, sizeof(sr_result_t));
    ESP_RETURN_ON_FALSE(NULL != g_result_que, ESP_ERR_NO_MEM, TAG, "Failed create result queue");

    models = esp_srmodel_init("model");

    afe_handle = &ESP_AFE_SR_HANDLE;
    afe_config_t afe_config = {
        .aec_init = true,
        .se_init = true,
        .vad_init = true,
        .wakenet_init = true,
        .voice_communication_init = false,
        .voice_communication_agc_init = false,
        .voice_communication_agc_gain = 15,
        .vad_mode = VAD_MODE_3,
        .wakenet_model_name = NULL,
        .wakenet_model_name_2 = NULL,
        .wakenet_mode = DET_MODE_90,
        .afe_mode = SR_MODE_LOW_COST,
        .afe_perferred_core = 0,
        .afe_perferred_priority = 5,
        .afe_ringbuf_size = 50,
        .memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM,
        .afe_linear_gain = 1.0,
        .agc_mode = AFE_MN_PEAK_AGC_MODE_2,
        .pcm_config.total_ch_num = 1,
        .pcm_config.mic_num = 1,
        .pcm_config.ref_num = 0,
        .pcm_config.sample_rate = 16000,
        .debug_init = false,
        .debug_hook = {{AFE_DEBUG_HOOK_MASE_TASK_IN, NULL}, {AFE_DEBUG_HOOK_FETCH_TASK_IN, NULL}},
        .afe_ns_mode = NS_MODE_SSP,
        .afe_ns_model_name = NULL,
        .fixed_first_channel = true,
    };

    afe_config.wakenet_model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
    afe_config.aec_init = false;

    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(&afe_config);
    ESP_LOGI(TAG, "load wakenet:%s", afe_config.wakenet_model_name);

    // for (int i = 0; i < models->num; i++) {
    //     ESP_LOGI(TAG, "Current Model:%s", models->model_name[i]);
    // }

    char *mn_name = esp_srmodel_filter(models, ESP_MN_CHINESE, NULL);
    if (NULL == mn_name)
    {
        ESP_LOGE(TAG, "No multinet model found");
        return ESP_FAIL;
    }

    multinet = esp_mn_handle_from_name(mn_name);
    model_data = multinet->create(mn_name, 5760);
    ESP_LOGI(TAG, "load multinet:%s", mn_name);

    esp_mn_commands_clear();

    for (int i = 0; i < sizeof(cmd_phoneme) / sizeof(cmd_phoneme[0]); i++)
    {
        esp_mn_commands_add(i, (char *)cmd_phoneme[i]);
    }

    esp_mn_commands_update();
    esp_mn_commands_print();
    multinet->print_active_speech_commands(model_data);

    BaseType_t ret_val = xTaskCreatePinnedToCore(audio_feed_task, "Feed Task", 4 * 1024, afe_data, 5, NULL, 1);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG, "Failed create audio feed task");

    ret_val = xTaskCreatePinnedToCore(audio_detect_task, "Detect Task", 6 * 1024, afe_data, 5, NULL, 0);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG, "Failed create audio detect task");

    ret_val = xTaskCreatePinnedToCore(sr_handler_task, "SR Handler Task", 4 * 1024, g_result_que, 1, NULL, 1);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG, "Failed create audio handler task");

    return ESP_OK;
}

esp_err_t app_sr_reset_command_list(char *command_list)
{
    char *err_id = heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);
    ESP_RETURN_ON_FALSE(NULL != err_id, ESP_ERR_NO_MEM, TAG, "memory is not enough");
    free(err_id);
    return ESP_OK;
}