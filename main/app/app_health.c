#include <stdio.h>
#include <esp_log.h>
#include <app_aliyun_mqtt.h>
#include <mqtt_client.h>

#define TAG "健康度:  "

char pub_payload[512];

// 饥饿度
int satiation;
// 清洁度
int cleanliness;

// 初始化健康状态
void health_init()
{
    satiation = 100;
    cleanliness = 100;
}

// 判断是否健康
void is_health()
{
    if (satiation <= 50 || cleanliness <= 50)
    {
        ESP_LOGE(TAG, "乐鑫生病了, 需要治疗");
    }
}

// 治疗
void treatment()
{
    satiation = 100;
    cleanliness = 100;
}

void set_health_down(const char *type)
{
    if (!strcmp(type, "breakfast"))
    {
        satiation -= 30;
    }
    else if (!strcmp(type, "lunch"))
    {
        satiation -= 30;
    }
    else if (!strcmp(type, "dinner"))
    {
        satiation -= 30;
    }
    else if (!strcmp(type, "flash"))
    {
        cleanliness -= 20;
    }
    else if (!strcmp(type, "sleep"))
    {
    }
}
void set_health_up(const char *type)
{
    if (!strcmp(type, "breakfast"))
    {
        satiation += 6;
    }
    else if (!strcmp(type, "lunch"))
    {
        satiation += 6;
    }
    else if (!strcmp(type, "dinner"))
    {
        satiation += 6;
    }
    else if (!strcmp(type, "flash"))
    {
        cleanliness += 4;
    }
    else if (!strcmp(type, "sleep"))
    {
    }
}

// RFID触发喂食
void set_satiation(int sat)
{
    if (sat >= 0)
    {
        satiation += sat;
    }
    else
    {
        satiation -= sat;
    }
    is_health();
}

// RFID触发清洁
void set_cleanliness(int clean)
{
    if (clean >= 0)
    {
        cleanliness += clean;
    }
    else
    {
        cleanliness -= clean;
    }
    is_health();
}

extern esp_mqtt_client_handle_t client;

void update_value()
{
    sprintf(pub_payload, "{\"params\": {\"Hunger_Level\":%d ,\"Cleaning_Metrics\":%d, \"MSI\":%d, \"health_value\":%d }, \"method\": \"thing.event.property.post\"}", satiation, cleanliness, 99, 99);

    int ret = esp_mqtt_client_publish(client, CONFIG_AliYun_PUBLISH_TOPIC_USER_POST, pub_payload, strlen(pub_payload), 2, 0);
    if (ret >= 0)
    {
        printf("正常发布，返回消息 ID: %d\n", ret);
    }
    else if (ret == -1)
    {
        printf("发布出现错误，返回 -1\n");
    }
    else if (ret == -2)
    {
        printf("缓冲区已满，返回 -2\n");
    }
}
