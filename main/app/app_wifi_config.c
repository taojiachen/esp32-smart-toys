/* WiFi Manager Example
   This code implements a WiFi configuration portal for ESP32
*/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_server.h"

#define WIFI_AP_SSID "ESP32_Config"
#define WIFI_AP_PASS "12345678"
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_MAX_CONNECTIONS 1

static const char *TAG = "wifi_config";

static const char *html_page = "<!DOCTYPE html>\
<html>\
<head>\
    <title>ESP32 WiFi Config</title>\
    <meta name='viewport' content='width=device-width, initial-scale=1'>\
    <style>\
        body { font-family: Arial; text-align: center; margin: 20px; }\
        .form-container { max-width: 400px; margin: 0 auto; padding: 20px; }\
        input[type=text], input[type=password] { width: 100%; padding: 12px 20px; \
            margin: 8px 0; display: inline-block; border: 1px solid #ccc; \
            box-sizing: border-box; }\
        button { background-color: #4CAF50; color: white; padding: 14px 20px; \
            margin: 8px 0; border: none; cursor: pointer; width: 100%; }\
        button:hover { opacity: 0.8; }\
    </style>\
</head>\
<body>\
    <div class='form-container'>\
        <h2>ESP32 WiFi Configuration</h2>\
        <form action='/save-config' method='post'>\
            <input type='text' name='ssid' placeholder='WiFi SSID' required>\
            <input type='password' name='password' placeholder='WiFi Password' required>\
            <button type='submit'>Save Configuration</button>\
        </form>\
    </div>\
</body>\
</html>";

// 改名为 saved_wifi_config_t 避免冲突
typedef struct
{
    char ssid[32];
    char password[64];
} saved_wifi_config_t;

static saved_wifi_config_t saved_wifi_config;

static void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 正确初始化 wifi_config_t 结构体
    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(WIFI_AP_SSID),
            .max_connection = WIFI_AP_MAX_CONNECTIONS,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .channel = WIFI_AP_CHANNEL,
        },
    };

    // 使用 memcpy 复制 SSID 和密码
    memcpy(wifi_config.ap.ssid, WIFI_AP_SSID, strlen(WIFI_AP_SSID));
    memcpy(wifi_config.ap.password, WIFI_AP_PASS, strlen(WIFI_AP_PASS));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started with SSID:%s password:%s",
             WIFI_AP_SSID, WIFI_AP_PASS);
}

static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_send(req, html_page, strlen(html_page));
    return ESP_OK;
}

static esp_err_t save_config_handler(httpd_req_t *req)
{
    char content[100];
    // 使用标准宏定义
    size_t recv_size = (req->content_len < sizeof(content)) ? req->content_len : sizeof(content);

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0)
    {
        return ESP_FAIL;
    }
    content[recv_size] = '\0';

    char ssid[32] = {0};
    char password[64] = {0};
    if (httpd_query_key_value(content, "ssid", ssid, sizeof(ssid)) == ESP_OK &&
        httpd_query_key_value(content, "password", password, sizeof(password)) == ESP_OK)
    {

        // 使用正确的结构体成员名
        strcpy(saved_wifi_config.ssid, ssid);
        strcpy(saved_wifi_config.password, password);

        // 保存到NVS
        nvs_handle_t nvs_handle;
        ESP_ERROR_CHECK(nvs_open("wifi_config", NVS_READWRITE, &nvs_handle));
        ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "ssid", ssid));
        ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "password", password));
        ESP_ERROR_CHECK(nvs_commit(nvs_handle));
        nvs_close(nvs_handle);

        const char *success_html = "<html><body><h2>Configuration Saved!</h2></body></html>";
        httpd_resp_send(req, success_html, strlen(success_html));

        // 延时一段时间后重启
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    return ESP_OK;
}

// 定义一个简单的处理函数，用于返回没有找到的提示
static esp_err_t favicon_handler(httpd_req_t *req)
{
    const char *response = "Favicon not available";
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

// HTTP服务器配置
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL};

    httpd_uri_t save_config = {
        .uri = "/save-config",
        .method = HTTP_POST,
        .handler = save_config_handler,
        .user_ctx = NULL};

    httpd_uri_t favicon = {
        .uri = "/favicon.ico",
        .method = HTTP_GET,
        .handler = favicon_handler,
        .user_ctx = NULL};

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &save_config);
        httpd_register_uri_handler(server, &favicon);
        return server;
    }

    return NULL;
}

void app_main(void)
{
    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 启动AP模式
    wifi_init_softap();

    // 启动HTTP服务器
    start_webserver();
}