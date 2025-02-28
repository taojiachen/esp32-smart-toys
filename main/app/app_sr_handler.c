/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_check.h"
#include "app_sr.h"
#include "app_sr_handler.h"
#include "esp_afe_sr_iface.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include<app_play_music.h>

static const char *TAG = "app_sr_handler";

void sr_handler_task(void *pvParam)
{
    ESP_LOGE(TAG, "======== sr_handler_task running ==========");

    QueueHandle_t xQueue = (QueueHandle_t)pvParam;

    while (true)
    {
        sr_result_t result;
        xQueueReceive(xQueue, &result, portMAX_DELAY);

        ESP_LOGI(TAG, "cmd:%d, wakemode:%d,state:%d", result.command_id, result.wakenet_mode, result.state);

        if (ESP_MN_STATE_TIMEOUT == result.state)
        {
            ESP_LOGI(TAG, "timeout");
            // lv_obj_clear_flag(ui_cat_gif, LV_OBJ_FLAG_HIDDEN);
            // lv_obj_add_flag(ui_Image1, LV_OBJ_FLAG_HIDDEN);
            // lv_obj_add_flag(ui_Labelwakenet, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        if (WAKENET_DETECTED == result.wakenet_mode)
        {
            ESP_LOGI(TAG, "wakenet detected");

            /* Modify UI */
            // lv_obj_add_flag(ui_cat_gif, LV_OBJ_FLAG_HIDDEN);
            // lv_obj_clear_flag(ui_Image1, LV_OBJ_FLAG_HIDDEN);
            // lv_label_set_text(ui_Labelwakenet, "请讲！");
            // lv_obj_clear_flag(ui_Labelwakenet, LV_OBJ_FLAG_HIDDEN);
            // _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 100, 0, &ui_Screen4_screen_init);
            continue;
        }

        if (ESP_MN_STATE_DETECTED & result.state)
        {
            ESP_LOGI(TAG, "mn detected");

            switch (result.command_id)
            {
            case 0:
                ESP_LOGI(TAG, "Turn on the air purifier");
                // lv_label_set_text(ui_Labelwakenet, "已打开");
                // mqtt_air_purifer_on();
                app_play_music("chiwanfanle");
                break;
            case 1:
                ESP_LOGI(TAG, "Turn off the air purifier");
                // mqtt_air_purifer_off();
                // lv_label_set_text(ui_Labelwakenet, "已关闭");
                break;
            case 2:
                ESP_LOGI(TAG, "Turn On the Lamp");
                // mqtt_lamp_on();
                // lv_label_set_text(ui_Labelwakenet, "已打开");
                break;
            case 3:
                ESP_LOGI(TAG, "Turn Off the Lamp");
                // mqtt_lampr_off();
                // lv_label_set_text(ui_Labelwakenet, "已关闭");
                break;
            case 4:
                ESP_LOGI(TAG, "Turn Lamp Brighter");
                // mqtt_lamp_brighter();
                // lv_label_set_text(ui_Labelwakenet, "好的！");
                break;
            case 5:
                ESP_LOGI(TAG, "Turn Lamp Dimmer");
                // mqtt_lampr_dimmer();
                // lv_label_set_text(ui_Labelwakenet, "好的！");
                break;
            case 6:
                ESP_LOGI(TAG, "Turn on the LED Strip");
                // mqtt_led_on();
                // lv_label_set_text(ui_Labelwakenet, "已打开");
                break;
            case 7:
                ESP_LOGI(TAG, "Turn off the LED Strip");
                // mqtt_led_off();
                // lv_label_set_text(ui_Labelwakenet, "已关闭");
                break;
            case 8:
                ESP_LOGI(TAG, "Play Music");
                // lv_label_set_text(ui_Labelwakenet, "好的！");
                // _ui_screen_change(&ui_Screen4, LV_SCR_LOAD_ANIM_FADE_ON, 100, 0, &ui_Screen4_screen_init);
                // music_play_resume();
                app_play_music("music");
                break;
            case 9:
                ESP_LOGI(TAG, "Stop Music");
                // lv_label_set_text(ui_Labelwakenet, "好的！");
                // _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 100, 0, &ui_Screen4_screen_init);
                // music_play_pause();
                break;
            case 10:
                ESP_LOGI(TAG, "Show Time");
                // lv_label_set_text(ui_Labelwakenet, "好的！");
                // _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 100, 0, &ui_Screen1_screen_init);
                break;
            case 11:
                ESP_LOGI(TAG, "Show Calander");
                // lv_label_set_text(ui_Labelwakenet, "好的！");
                // _ui_screen_change(&ui_Screen3, LV_SCR_LOAD_ANIM_FADE_ON, 100, 0, &ui_Screen1_screen_init);
                break;
            default:
            
                break;
            }
            /* **************** REGISTER COMMAND CALLBACK HERE **************** */
        }
    }

    vTaskDelete(NULL);
}