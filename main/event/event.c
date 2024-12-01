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

#define NAME_SPACE "JSON_TASK"
nvs_handle_t nvshandle;

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

// 声明结构体链表
typedef struct task_list
{
    char *task_type;
    char *tart_time;
    char *end_time;
    struct task_list *next;
} task_list;

// 创建链表节点函数
task_list *create_task_list(char *task_type, char *tart_time, char *end_time)
{
    task_list *newNode = (task_list *)malloc(sizeof(task_list));
    newNode->task_type = task_type;
    newNode->tart_time = tart_time;
    newNode->end_time = end_time;
    newNode->next = NULL;
    return newNode;
}

// 在链表尾部插入新节点函数
task_list *insertAtTail(task_list **head, int data)
{
    task_list *newNode = create_task_list(1, 2, 3);
    if (*head == NULL)
    {
        *head = newNode;
        return newNode;
    }
    task_list *current = *head;
    while (current->next != NULL)
    {
        current = current->next;
    }
    current->next = newNode;
    return newNode;
}

// 释放无用链表内存函数
void freeList(task_list **head)
{
    task_list *current = *head;
    while (current != NULL)
    {
        task_list *temp = current;
        current = current->next;
        free(temp);
    }
    *head = NULL;
}

void nvs_write(char *jsonStr)
{
}

void nvs_erase()
{
}

// 阿里云端任务下发
void set_task(const char *JOSN_str)
{
    cJSON *json = cJSON_Parse(JOSN_str);
}

// 事件处理函数 - 任务1
static void task1_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{

    RFID_start();
    printf("Task 1: Output 1\n");
}

// 事件处理函数 - 任务2
static void task2_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
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

    // 打印本地时间
    printf("Current local time: %d-%02d-%02d %02d:%02d:%02d\n",
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    if (tm_info->tm_sec == 0)
    {
        esp_event_post_to(timer_event_loop, TIMER_EVENTS, TIMER_EVENT_TASK1, NULL, 0, portMAX_DELAY);
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