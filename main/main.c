#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "esp_bt.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_blufi_api.h"
#include "blufi_example.h"
#include "esp_blufi.h"
#include "esp_board_init.h"
#include <app_sntp.h>
#include <esp_psram.h>
#include <event.h>
#include <app_RFID.h>
#include <app_aliyun_mqtt.h>
#include <app_health.h>
#include <app_sr.h>
#include <app_play_music.h>
#include <bsp_board.h>

static const char *TAG = "main";

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    //health_init();
    //esp_board_init();
    //spi_bus_init();
    //esp_spiffs_mount();
    //blufi_start();
    //app_sntp_init();
    //ESP_ERROR_CHECK(app_sr_start());
    //app_aliyun_mqtt_init();
    app_play_music();
    //event_start();
    ESP_LOGI(TAG, "Free memory after start: %d bytes", heap_caps_get_total_size(MALLOC_CAP_INTERNAL));
    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}