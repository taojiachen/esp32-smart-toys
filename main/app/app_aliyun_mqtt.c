#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/adc.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "mqtt_client.h"
#include "aliyun_mqtt.h"
#include "cJSON.h"
//#include "motor_ctrl.h"

#include "esp_system.h"
#include "esp_sntp.h"
#include "esp_pm.h"

#define ADC_CHANNEL GPIO_NUM_0 // IO1 引脚
#define ADC_WIDTH ADC_WIDTH_BIT_DEFAULT
#define ADC_ATTEN ADC_ATTEN_DB_12

char local_data_buffer[1024] = {0};
char mqtt_publish_data1[] = "mqtt connect ok ";
char mqtt_publish_data2[] = "mqtt subscribe successful";
char mqtt_publish_data3[] = "mqtt i am esp32";
char pub_payload[512];
static const char *TAG = "APP_ALIYUN_MQTT";

esp_mqtt_client_handle_t client;
static esp_err_t app_mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    client = event->client;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        char *msg_id;
        // msg_id = esp_mqtt_client_subscribe(client, CONFIG_AliYun_SUBSCRIBE_TOPIC_USER_GET, 0);
        // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        msg_id = "{\"method\": \"thing.service.property.set\",\"id\": \"1490775934\",\"params\": {\"RealTime_Hydration\": {\"waterport\": 0,\"switch\": 0}},\"version\": \"1.0.0\"}";
        //motor_sta(msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // 注意：这里使用了同样的发布主题，根据实际逻辑可能需要调整
        // msg_id = esp_mqtt_client_publish(client, CONFIG_AliYun_PUBLISH_TOPIC_USER_UPDATE, mqtt_publish_data2, strlen(mqtt_publish_data2), 0, 0);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        // 云端事件处理
        // 云流转
        ESP_LOGE(TAG, "MQTT_EVENT_DATA");
        const char *jsonStr = event->data;
        ESP_LOGE(TAG, "DATA=%.*s\r\n", event->data_len, jsonStr);
        //motor_sta(jsonStr);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    case MQTT_EVENT_ANY:
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

static void app_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    app_mqtt_event_handler_cb(event_data);
}

void app_aliyun_mqtt_init(void)
{

    aliyun_mqtt_init(app_mqtt_event_handler);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    vTaskDelete(NULL);
}

// {"method":"thing.service.property.set","id":"27738049","params":{"RealTime_Hydration":{"waterport":0,"switch":0}},"version":"1.0.0"}

// {"method":"thing.service.property.set","id":"1898751389","params":{"RealTime_Hydration":{"waterport":1,"switch":1}},"version":"1.0.0"}

