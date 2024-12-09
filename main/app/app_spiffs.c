
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#define TAG "MP3_DOWNLOAD"
#define MOUNT_POINT "/spiffs"

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static FILE *fp = NULL;

    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;

    case HTTP_EVENT_HEADERS_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADERS_SENT");
        break;

    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;

    case HTTP_EVENT_ON_DATA:
        if (fp == NULL)
        {
            fp = fopen("/spiffs/music.mp3", "wb");
            if (fp == NULL)
            {
                ESP_LOGE(TAG, "Failed to open file for writing");
                return ESP_FAIL;
            }
        }
        fwrite(evt->data, 1, evt->data_len, fp);
        break;

    case HTTP_EVENT_ON_FINISH:
        if (fp)
        {
            fclose(fp);
            fp = NULL;
        }
        ESP_LOGI(TAG, "Download completed");
        break;

    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        if (fp)
        {
            fclose(fp);
            fp = NULL;
        }
        break;

    case HTTP_EVENT_ERROR:
        if (fp)
        {
            fclose(fp);
            fp = NULL;
        }
        ESP_LOGE(TAG, "HTTP Error");
        break;

    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}

void download_mp3(const char *url)
{
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
 
    esp_http_client_cleanup(client);
}

void spiffs_get(void)
{
    FILE *f = NULL;
    f = fopen("/spiffs/music.mp3", "wb");
    if (f == NULL)
    {
        // 下载MP3文件
        const char *mp3_url = "http://music.163.com/song/media/outer/url?id=447925558.mp3";
        download_mp3(mp3_url);
    }
}

// 查看当前spiifs文件系统中的文件。
void list_files_in_spiffs()
{
    DIR *dir = opendir("/spiffs");
    if (dir == NULL)
    {
        printf("Error opening SPIFFS directory\n");
        return;
    }

    struct dirent *de;
    while ((de = readdir(dir)) != NULL)
    {
        // 忽略. 和.. 目录
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
        {
            continue;
        }
        printf("found file: %s\n", de->d_name);
    }

    closedir(dir);
}

// 删除spiifs文件系统中的指定文件

void delete_file_in_spiffs(const char *file_name)
{
    char file_path[50];
    sprintf(file_path, "/spiffs/%s.mp3", file_name);
    int result = unlink(file_path);
    if (result == 0)
    {
        printf("文件 %s 已成功删除。\n", file_path);
    }
    else
    {
        printf("删除文件 %s 时出错。\n", file_path);
    }
}

// 判断文件是否存在
