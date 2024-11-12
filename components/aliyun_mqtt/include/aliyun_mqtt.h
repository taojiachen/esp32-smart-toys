#ifndef _ALIYUN_MQTT_H_
#define _ALIYUN_MQTT_H_

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Initializes the Alibaba Cloud MQTT client and attempts to establish a connection.
     *
     * This function serves as the starting point for MQTT client operations, configuring the client and initiating the connection process.
     * Success or failure scenarios are notified through the registered event handler \p app_mqtt_event_handler.
     *
     * @param app_mqtt_event_handler Pointer to the callback function for receiving MQTT connection and message events.
     */
    void aliyun_mqtt_init(esp_event_handler_t app_mqtt_event_handler);

    /**
     * @brief Cleans up and closes the Alibaba Cloud MQTT client.
     *
     * Call this function when MQTT client services are no longer needed to properly release all resources and disconnect from the server.
     */
    void aliyun_mqtt_deinit(void);
#ifdef __cplusplus
}
#endif

#endif