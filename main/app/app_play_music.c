#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "fatfs_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "esp_peripherals.h"
#include "periph_spiffs.h"
#include "board.h"
#include "esp_spiffs.h"
#include "driver/i2s_std.h"

static const char *TAG = "MP3_EXAMPLE";

// I2S引脚配置
#define I2S_BCK_IO (GPIO_NUM_15) // I2S BCLK引脚
#define I2S_WS_IO (GPIO_NUM_16)  // I2S LRCLK引脚
#define I2S_DO_IO (GPIO_NUM_7)   // I2S DATA引脚
#define I2S_PORT (I2S_NUM_1)     // 使用I2S_1

// SPIFFS配置
static const char *base_path = "/spiffs";
static const esp_vfs_spiffs_conf_t spiffs_conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = true};

void app_play_music(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "[ 1 ] 初始化外设管理");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    ESP_LOGI(TAG, "[ 2 ] 初始化SPIFFS");
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&spiffs_conf));

    ESP_LOGI(TAG, "[ 3 ] 创建音频管道");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    audio_pipeline_handle_t pipeline = audio_pipeline_init(&pipeline_cfg);

    ESP_LOGI(TAG, "[3.1] 创建FATFS流读取SPIFFS数据");
    fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_cfg.type = AUDIO_STREAM_READER;
    audio_element_handle_t fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);

    ESP_LOGI(TAG, "[3.2] 创建I2S流写入数据到编解码器");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;

    // 新的I2S标准配置
    i2s_chan_config_t chan_cfg = {
        .id = I2S_PORT,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 8,
        .dma_frame_num = 64,
        .auto_clear = true};

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DO_IO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    // 创建并初始化I2S通道
    i2s_chan_handle_t tx_handle;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

    // 初始化I2S标准模式
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));

    // 启动I2S通道
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));

    audio_element_handle_t i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[3.3] 创建MP3解码器");
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    audio_element_handle_t mp3_decoder = mp3_decoder_init(&mp3_cfg);

    ESP_LOGI(TAG, "[3.4] 注册所有元素到音频管道");
    audio_pipeline_register(pipeline, fatfs_stream_reader, "file");
    audio_pipeline_register(pipeline, mp3_decoder, "mp3");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

    ESP_LOGI(TAG, "[3.5] 链接元素 [spiffs]-->fatfs_stream-->mp3_decoder-->i2s_stream-->[codec_chip]");
    const char *link_tag[3] = {"file", "mp3", "i2s"};
    audio_pipeline_link(pipeline, &link_tag[0], 3);

    ESP_LOGI(TAG, "[ 4 ] 设置事件监听器");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    audio_pipeline_set_listener(pipeline, evt);
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

    ESP_LOGI(TAG, "[ 5 ] 启动音频管道");
    audio_element_set_uri(fatfs_stream_reader, "/spiffs/LemonTree.mp3");

    // 在设置URI后添加
    FILE *f = fopen("/spiffs/LemonTree.mp3", "rb");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        ESP_LOGI(TAG, "MP3文件大小: %d bytes", size);
        fclose(f);
    }

    esp_err_t ret = audio_pipeline_run(pipeline);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "音频管道运行失败: %d", ret);
        return;
    }

    while (1)
    {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "[ * ] 事件接口错误: %d", ret);
            continue;
        }

        ESP_LOGI(TAG, "收到事件 - 源类型: %d, 命令: %d", msg.source_type, msg.cmd);

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *)mp3_decoder && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
        {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(mp3_decoder, &music_info);

            ESP_LOGI(TAG, "[ * ] 接收到MP3解码器音乐信息, 采样率=%d, 位数=%d, 声道=%d",
                     music_info.sample_rates, music_info.bits, music_info.channels);

            // 更新I2S时钟配置
            i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(music_info.sample_rates);
            i2s_channel_disable(tx_handle);
            ESP_ERROR_CHECK(i2s_channel_reconfig_std_clock(tx_handle, &clk_cfg));
            i2s_channel_enable(tx_handle);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *)i2s_stream_writer && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int)msg.data == AEL_STATUS_STATE_STOPPED)
        {
            ESP_LOGI(TAG, "[ * ] 收到停止事件");
            break;
        }
    }

    // 清理资源
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);

    audio_pipeline_unregister(pipeline, fatfs_stream_reader);
    audio_pipeline_unregister(pipeline, i2s_stream_writer);
    audio_pipeline_unregister(pipeline, mp3_decoder);

    audio_pipeline_deinit(pipeline);
    audio_element_deinit(fatfs_stream_reader);
    audio_element_deinit(i2s_stream_writer);
    audio_element_deinit(mp3_decoder);

    // 删除I2S通道
    ESP_ERROR_CHECK(i2s_channel_disable(tx_handle));
    ESP_ERROR_CHECK(i2s_del_channel(tx_handle));

    esp_periph_set_destroy(set);
    esp_vfs_spiffs_unregister(NULL);
}