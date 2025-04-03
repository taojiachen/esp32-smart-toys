#include <stdio.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <event.h>
#include <time.h>
#include <sys/time.h>
#include <app_sr.h>
#include <app_RFID.h>
#include <cJSON.h>
#include <app_sr.h>
#include <string.h>
#include "nvs_flash.h"
#include <app_play_music.h>
#include <app_task_list.h>
#include <app_health.h>

#define NAME_SPACE "JSON_TASK"

static const char *TAG = "TIMER_EVENT";

extern int flag;

// 定义自定义事件基础值
ESP_EVENT_DEFINE_BASE(TIMER_EVENTS);

struct timeval tv;
struct tm *tm_info;
TaskHandle_t Task1_handle = NULL;

// 事件循环句柄
static esp_event_loop_handle_t timer_event_loop;

struct Nearest_Task
{
    char *key;
    char *datavalue;
    int starttime;
    int keeptime;
    int average_time;
} Nearest_Task;

int Current_time;

// 更新距离当前时间最近的任务信息
void Update_Nearest_Task()
{
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    Nearest_Task.key = NULL;
    Nearest_Task.datavalue = NULL;
    Nearest_Task.starttime = -1;
    Nearest_Task.keeptime = -1;
    Nearest_Task.average_time = -1;
    Current_time = tm_info->tm_hour * 3600 + tm_info->tm_min * 60 + tm_info->tm_sec;
    get_nearest_task(Current_time, &Nearest_Task.key, &Nearest_Task.datavalue, &Nearest_Task.starttime, &Nearest_Task.keeptime);
    Nearest_Task.average_time = Nearest_Task.keeptime * 60 / 5;
    if (Nearest_Task.key != NULL && Nearest_Task.datavalue != NULL && Nearest_Task.starttime != -1 && Nearest_Task.keeptime != -1 && Nearest_Task.average_time != -1)
    {
        printf("Nearest task: key=%s, datavalue=%s, starttime=%d, keeptime=%d, average_time=%d\n", Nearest_Task.key, Nearest_Task.datavalue, Nearest_Task.starttime, Nearest_Task.keeptime, Nearest_Task.average_time);
    }
    else
    {
        printf("No nearest task found\n");
    }
    // free(Nearest_Task.key);
    // free(Nearest_Task.datavalue);
    // free(Nearest_Task.starttime);
}

// 事件处理函数（到达距离当前时间最近的任务时间触发事件）
static void task(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    int temp = 1;
    int flag = 0;
    printf("--------------------------------------------------------------------\n");
    if (!strcmp(Nearest_Task.key, "test"))
    {
        app_play_music("woele");
        set_health_down(Nearest_Task.key);
        update_value();
    }
    else if (!strcmp(Nearest_Task.key, "breakfast"))
    {
        app_play_music("woele");
        set_health_down(Nearest_Task.key);
        update_value();
    }
    else if (!strcmp(Nearest_Task.key, "lunch"))
    {
        app_play_music("woele");
        set_health_down(Nearest_Task.key);
        update_value();
    }
    else if (!strcmp(Nearest_Task.key, "dinner"))
    {
        app_play_music("woele");
        set_health_down(Nearest_Task.key);
        update_value();
    }
    else if (!strcmp(Nearest_Task.key, "flash"))
    {
        app_play_music("shuaya");
        set_health_down(Nearest_Task.key);
        update_value();
    }
    else if (!strcmp(Nearest_Task.key, "sleep"))
    {
        app_play_music("kunle");
        set_health_down(Nearest_Task.key);
        update_value();
    }
    RFID_start();
    ESP_LOGI("TASK中的Current_time:  ", "%d", Current_time);
    while (Current_time <= Nearest_Task.starttime + Nearest_Task.keeptime * 60)
    {
        if (flag == 1)
        {

            if (temp == 1)
            {
                set_health_up(Nearest_Task.key);
                update_value();
                temp = 0;
            }
            ESP_LOGI("flag:                    ", "%d", flag);
            RFID_stop();
            vTaskDelay(pdMS_TO_TICKS(1000));
            if (Current_time == Nearest_Task.starttime + Nearest_Task.average_time || Current_time == Nearest_Task.starttime + Nearest_Task.average_time * 2 || Current_time == Nearest_Task.starttime + Nearest_Task.average_time * 3 || Current_time == Nearest_Task.starttime + Nearest_Task.average_time * 4)
            {
                RFID_start();
                temp = 1;
            }
        }
        else if (flag == 0)
        {
            ESP_LOGE("TASK中的flag:  ", "%d", flag);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    RFID_stop();
}

/*
{"method":"thing.service.property.set","id":"2115005914","params":{"tasks":[{"datavalue":"5B 9D 44 0C","index":1,"starttime":"22:45:00","type":1,"key":"test","keeptime":30}]},"version":"1.0.0"}
{"method":"thing.service.property.set","id":"2115005914","params":{"tasks":[{"datavalue":"F4 21 CF EC","index":1,"starttime":"14:23:00","type":1,"key":"test","keeptime":3}]},"version":"1.0.0"}
*/

// 定时器回调函数
static void timer_callback(void *arg)
{
    char *task_type;
    char *tart_time;
    char *end_time;
    struct timeval tv;
    struct tm *tm_info;
    gettimeofday(&tv, NULL);

    // 将时间戳转换为本地时间
    tm_info = localtime(&tv.tv_sec);

    // 打印本地时间
    printf("Current local time: %d-%02d-%02d %02d:%02d:%02d\n",
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    // Update_Nearest_Task();
    // 任务执行处理：根据任务类型和时间执行相应的操作
    Current_time = tm_info->tm_hour * 3600 + tm_info->tm_min * 60 + tm_info->tm_sec;
    printf("%d\n", Current_time);
    if (Nearest_Task.starttime == Current_time)
    {
        // app_play_music("kunle");
        ESP_LOGE(TAG, "Timer event triggered");
        // printf("--------------------------------------------------------------------");
        esp_event_post_to(timer_event_loop, TIMER_EVENTS, 0, NULL, 0, portMAX_DELAY);
    }
}

void event_start(void)
{
    // 创建事件循环
    esp_event_loop_args_t loop_args = {
        .queue_size = 10,
        .task_name = "timer_event_task",
        .task_priority = 6,
        .task_stack_size = 1024 * 10,
        .task_core_id = 1};

    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &timer_event_loop));
    // 注册事件处理程序
    ESP_ERROR_CHECK(esp_event_handler_register_with(timer_event_loop, TIMER_EVENTS, 0, task, NULL));
    // 创建定时器
    esp_timer_handle_t timer;
    const esp_timer_create_args_t timer_args = {
        .callback = &timer_callback,
        .name = "timer"};
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer));

    // 启动定时器，每秒触发一次
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, 1000000)); // 1秒 = 1000000微秒

    ESP_LOGI(TAG, "Timer and event loop started");
}