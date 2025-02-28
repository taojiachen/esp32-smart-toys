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

#define NAME_SPACE "JSON_TASK"

static const char *TAG = "TIMER_EVENT";

// 定义自定义事件基础值
ESP_EVENT_DEFINE_BASE(TIMER_EVENTS);

enum
{
    EAT,
    BRUSH_TEETH,
};

struct timeval tv;
struct tm *tm_info;
TaskHandle_t Task1_handle = NULL;

// 事件循环句柄
static esp_event_loop_handle_t timer_event_loop;

struct Nearest_Task
{
    char *key;
    char *datavalue;
    char *starttime;
    long keeptime;
} Nearest_Task;

int Hour = 0;
int Minute = 0;
int Second = 0;

// 定义事件类型
void Update_Nearest_Task()
{
    char time_str[9];
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    Nearest_Task.key = NULL;
    Nearest_Task.datavalue = NULL;
    Nearest_Task.starttime = NULL;
    Nearest_Task.keeptime = 0;
    get_nearest_task(time_str, &Nearest_Task.key, &Nearest_Task.datavalue, &Nearest_Task.starttime, &Nearest_Task.keeptime);
    if (Nearest_Task.key != NULL && Nearest_Task.datavalue != NULL && Nearest_Task.starttime != NULL)
    {
        printf("Nearest task: key=%s, datavalue=%s, starttime=%s, keeptime=%ld\n", Nearest_Task.key, Nearest_Task.datavalue, Nearest_Task.starttime, Nearest_Task.keeptime);
    }else{
        printf("No nearest task found\n");
    }
    free(Nearest_Task.key);
    free(Nearest_Task.datavalue);
    free(Nearest_Task.starttime);
}

// 事件处理函数 - 吃饭
static void EAT_task(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{

    RFID_start();
    printf("Task 1: Output 1\n");
}

// 事件处理函数 - 刷牙
static void BRUSH_TEETH_task(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{

    printf("Task 2: Output 2\n");
}

// 事件处理函数 - 任务3
static void task3_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{

    printf("Task 3: Output 3\n");
}

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

    //打印本地时间
    printf("Current local time: %d-%02d-%02d %02d:%02d:%02d\n",
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    Update_Nearest_Task();

    // 任务执行处理：根据任务类型和时间执行相应的操作
    if ( tm_info->tm_sec == 0)
    {
        RFID_start();
        // esp_event_post_to(timer_event_loop, TIMER_EVENTS, TIMER_EVENT_TASK1, NULL, 0, portMAX_DELAY);
    }
    if (tm_info->tm_sec == 30)
    {

        RFID_stop();
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
    ESP_ERROR_CHECK(esp_event_handler_register_with(timer_event_loop, TIMER_EVENTS, EAT, EAT_task, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(timer_event_loop, TIMER_EVENTS, BRUSH_TEETH, BRUSH_TEETH_task, NULL));

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