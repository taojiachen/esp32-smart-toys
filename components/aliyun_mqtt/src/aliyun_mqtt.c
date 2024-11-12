#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"

#include "mqtt_client.h"
#include "aliyun_mqtt.h"

static const char *TAG = "ALIYUN_MQTT";
static esp_mqtt_client_handle_t client;
/**
 * @brief Initializes and starts the Alibaba Cloud MQTT client.
 *
 * This function is responsible for initializing the ESP-MQTT client based on the provided configuration
 * and attempting to connect to the Alibaba Cloud MQTT service. The configuration includes server address,
 * authentication credentials, and registers an event handler to manage MQTT-related events.
 *
 * @param app_mqtt_event_handler Pointer to the callback function for handling MQTT events.
 * @return esp_err_t Status of the function execution, ESP_OK indicates success, other values indicate failure.
 */
static esp_err_t aliyun_mqtt_start(esp_event_handler_t app_mqtt_event_handler)
{
    esp_err_t ret;

    /** Configuration structure defining the MQTT client's connection settings */
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {.address = {
                       .uri = CONFIG_AliYun_MQTT_HOST_URL,
                   }},
        .credentials = {
            .username = CONFIG_AliYun_MQTT_USERNAME,
            .client_id = CONFIG_AliYun_MQTT_CLIENT_ID,
            .authentication = {
                .password = CONFIG_AliYun_MQTT_PASSWORD,
                .use_secure_element = 0,
            },
        },
    };

    /* Initialize the MQTT client */
    client = esp_mqtt_client_init(&mqtt_cfg);
    if (!client)
    {
        return ESP_FAIL;
    }

    /* Register MQTT client event handling function */
    ret = esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, app_mqtt_event_handler, client);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_mqtt_client_register_event failed");
        return ret;
    }

    /* Begin MQTT client connection */
    ret = esp_mqtt_client_start(client);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_mqtt_client_start failed");
        return ret;
    }

    return ESP_OK;
}

void aliyun_mqtt_init(esp_event_handler_t app_mqtt_event_handler)
{
    esp_err_t ret = aliyun_mqtt_start(app_mqtt_event_handler);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "aliyun_mqtt_start failed");
    }
}
void aliyun_mqtt_deinit(void)
{
    esp_mqtt_client_destroy(client);
    ESP_LOGI(TAG, "aliyun_mqtt_deinit");
}