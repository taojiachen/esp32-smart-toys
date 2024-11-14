#include <stdio.h>
#include <esp_log.h>

#define TAG "健康度:  "
//饥饿度
int satiation;
//清洁度
int cleanliness;

//初始化健康状态
void health_init()
{
    satiation = 100;
    cleanliness = 100;
}

//判断是否健康
void is_health()
{
    if (satiation <= 50 || cleanliness <= 50)
    {
        ESP_LOGE(TAG, "乐鑫生病了, 需要治疗");
    }
}

//治疗
void treatment()
{
    satiation = 100;
    cleanliness = 100;
}

//RFID触发喂食
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

//RFID触发清洁
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
