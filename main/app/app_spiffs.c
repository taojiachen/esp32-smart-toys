#include "stdio.h"
#include "string.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_http_client.h"
#include <nvs.h>

#include<app_spiffs.h>

// 定义缓冲区大小，用于临时存储从网络读取的数据
#define BUFFER_SIZE 1024  
// 日志标签，方便在日志输出中识别相关信息来源
static const char *TAG = "MP3_DOWNLOAD";  

// HTTP事件处理函数，用于处理HTTP请求过程中的各种事件
static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            // 当接收到数据时会触发此事件
            ESP_LOGD(TAG, "Received data, length=%d", evt->data_len);
            // 在这里可以将接收到的数据写入到Flash文件中，后续代码中会有体现
            break;
        case HTTP_EVENT_ON_FINISH:
            // HTTP请求完成后触发此事件
            ESP_LOGI(TAG, "HTTP request finished");
            break;
        case HTTP_EVENT_ERROR:
            // 如果HTTP请求出现错误会触发此事件，这里简单打印错误提示
            ESP_LOGE(TAG, "HTTP request error occurred");
            break;
        case HTTP_EVENT_ON_HEADER:
            // 当接收到服务器发送的每个头部信息时触发此事件
            ESP_LOGD(TAG, "Received header: %s: %s", evt->header_key, evt->header_value);
            break;
        default:
            break;
    }
    return ESP_OK;
}

void spiffs_get(void)
{
    esp_err_t ret;

    // 配置HTTP客户端结构体，用于设置要下载文件的相关参数
    esp_http_client_config_t config = {
     .url = "http://music.163.com/song/media/outer/url?id=447925558.mp3",  // 需要替换为实际的MP3文件下载网址
     .event_handler = _http_event_handler,  // 设置HTTP事件处理函数
    };

    // 初始化HTTP客户端实例
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return;
    }

    // 打开HTTP连接，准备开始下载
    ret = esp_http_client_open(client, 0);
    if (ret!= ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection");
        // 关闭并清理HTTP客户端资源
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    }

    // 获取HTTP响应头中的内容长度，用于判断文件大小和控制下载进度
    int content_length = esp_http_client_fetch_headers(client);
    ESP_LOGI(TAG, "Content length: %d", content_length);

    // 以二进制写入模式打开SPIFFS中的文件，准备写入下载的数据
    FILE *fp = fopen("/spiffs/your.mp3", "wb");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        // 关闭并清理HTTP客户端资源
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    }

    // 用于累计已经读取的字节数
    int total_read = 0;
    // 分配缓冲区，用于临时存储从网络读取的数据
    char buffer[BUFFER_SIZE];
    while (total_read < content_length) {
        // 从HTTP连接中读取数据到缓冲区
        int data_read = esp_http_client_read(client, buffer, BUFFER_SIZE);
        if (data_read <= 0) {
            // 如果读取到的数据长度小于等于0，表示读取结束或者出现错误
            break;
        }
        // 将缓冲区中的数据写入到SPIFFS文件中
        fwrite(buffer, 1, data_read, fp);
        total_read += data_read;
    }

    // 关闭SPIFFS文件
    fclose(fp);

    // 关闭HTTP客户端连接
    esp_http_client_close(client);
    // 清理HTTP客户端资源，释放相关内存等
    esp_http_client_cleanup(client);
}