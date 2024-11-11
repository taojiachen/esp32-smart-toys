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

static const char *TAG = "TIMER_EVENT";

// 定义自定义事件基础值
ESP_EVENT_DEFINE_BASE(TIMER_EVENTS);

// 定义事件类型
enum
{
    TIMER_EVENT_TASK1,
    TIMER_EVENT_TASK2,
    TIMER_EVENT_TASK3,
};

struct timeval tv;
struct tm *tm_info;
TaskHandle_t Task1_handle = NULL;

// 事件循环句柄
static esp_event_loop_handle_t timer_event_loop;

// 事件处理函数 - 任务1
static void task1_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    gettimeofday(&tv, NULL);

    // 将时间戳转换为本地时间
    tm_info = localtime(&tv.tv_sec);

    // 打印本地时间
    printf("Current local time: %d-%02d-%02d %02d:%02d:%02d\n",
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    // app_sr_start();
    RFID_start();
    printf("Task 1: Output 1\n");
}

// 事件处理函数 - 任务2
static void task2_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    gettimeofday(&tv, NULL);

    // 将时间戳转换为本地时间
    tm_info = localtime(&tv.tv_sec);

    // 打印本地时间
    printf("Current local time: %d-%02d-%02d %02d:%02d:%02d\n",
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    printf("Task 2: Output 2\n");
}

// 事件处理函数 - 任务3
static void task3_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    gettimeofday(&tv, NULL);

    // 将时间戳转换为本地时间
    tm_info = localtime(&tv.tv_sec);

    // 打印本地时间
    printf("Current local time: %d-%02d-%02d %02d:%02d:%02d\n",
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    printf("Task 3: Output 3\n");
}

// 定时器回调函数
static void timer_callback(void *arg)
{
    struct timeval tv;
    struct tm *tm_info;

    gettimeofday(&tv, NULL);

    // 将时间戳转换为本地时间
    tm_info = localtime(&tv.tv_sec);

    // 打印本地时间
    printf("Current local time: %d-%02d-%02d %02d:%02d:%02d\n",
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    if (tm_info->tm_hour == 22 &&(tm_info->tm_min == 38||tm_info->tm_min == 40) && tm_info->tm_sec == 0)
    {
        esp_event_post_to(timer_event_loop, TIMER_EVENTS, TIMER_EVENT_TASK1, NULL, 0, portMAX_DELAY);
    }
    if (tm_info->tm_hour == 22 && tm_info->tm_min == 39 && tm_info->tm_sec == 0)
    {
        RFID_stop();
    }
    // else if ( <= 20)
    // {
    //     esp_event_post_to(timer_event_loop, TIMER_EVENTS, TIMER_EVENT_TASK2, NULL, 0, portMAX_DELAY);
    // }
    // else if ( <= 30)
    // {
    //     esp_event_post_to(timer_event_loop, TIMER_EVENTS, TIMER_EVENT_TASK3, NULL, 0, portMAX_DELAY);
    // }
}

void event_start(void)
{
    // 创建事件循环
    esp_event_loop_args_t loop_args = {
        .queue_size = 10,
        .task_name = "timer_event_task",
        .task_priority = 6,
        .task_stack_size = 1024 * 20,
        .task_core_id = 0};

    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &timer_event_loop));

    // 注册事件处理程序
    ESP_ERROR_CHECK(esp_event_handler_register_with(timer_event_loop, TIMER_EVENTS, TIMER_EVENT_TASK1, task1_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(timer_event_loop, TIMER_EVENTS, TIMER_EVENT_TASK2, task2_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(timer_event_loop, TIMER_EVENTS, TIMER_EVENT_TASK3, task3_handler, NULL));

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