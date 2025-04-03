#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_spiffs.h"
#include "cJSON.h"
#include "esp_log.h"
#include <limits.h>
#include <event.h>

#define TASKS_FILE "/spiffs/task_list.json"
#define TAG "JSON_Handler"

// 读取文件内容
char *read_file(void)
{
    FILE *f = fopen(TASKS_FILE, "r");
    if (f == NULL)
    {
        // 如果文件不存在，创建一个空的JSON数组
        f = fopen(TASKS_FILE, "w");
        if (f != NULL)
        {
            fprintf(f, "[]");
            fclose(f);
        }
        f = fopen(TASKS_FILE, "r");
        if (f == NULL)
        {
            ESP_LOGE(TAG, "Failed to open file for reading");
            return NULL;
        }
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(fsize + 1);
    if (buffer == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory");
        fclose(f);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, fsize, f);
    fclose(f);

    if (read_size != fsize)
    {
        ESP_LOGE(TAG, "Failed to read file completely");
        free(buffer);
        return NULL;
    }

    buffer[fsize] = 0;
    return buffer;
}

// 写入文件内容
bool write_file(const char *content)
{
    if (content == NULL)
    {
        ESP_LOGE(TAG, "Content is NULL");
        return false;
    }

    FILE *f = fopen(TASKS_FILE, "w");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return false;
    }

    fprintf(f, "%s", content);
    fclose(f);
    return true;
}

// 更新或添加任务
bool update_task(const char *json_string)
{
    if (json_string == NULL)
    {
        ESP_LOGE(TAG, "Input JSON string is NULL");
        return false;
    }

    // 解析输入的JSON
    cJSON *root = cJSON_Parse(json_string);
    if (!root)
    {
        ESP_LOGE(TAG, "Failed to parse input JSON");
        return false;
    }

    // 获取任务信息
    cJSON *params = cJSON_GetObjectItem(root, "params");
    if (!params)
    {
        ESP_LOGE(TAG, "No params in JSON");
        cJSON_Delete(root);
        return false;
    }

    cJSON *tasks = cJSON_GetObjectItem(params, "tasks");
    if (!tasks || !cJSON_IsArray(tasks))
    {
        ESP_LOGE(TAG, "No tasks array in JSON");
        cJSON_Delete(root);
        return false;
    }

    cJSON *task = cJSON_GetArrayItem(tasks, 0);
    if (!task)
    {
        ESP_LOGE(TAG, "No task in tasks array");
        cJSON_Delete(root);
        return false;
    }

    cJSON *key_item = cJSON_GetObjectItem(task, "key");
    if (!key_item || !key_item->valuestring)
    {
        ESP_LOGE(TAG, "No valid key in task");
        cJSON_Delete(root);
        return false;
    }

    char *key = key_item->valuestring;

    // 读取现有文件
    char *file_content = read_file();
    if (!file_content)
    {
        cJSON_Delete(root);
        return false;
    }

    cJSON *file_root = cJSON_Parse(file_content);
    free(file_content);

    if (!file_root)
    {
        file_root = cJSON_CreateArray();
        if (!file_root)
        {
            ESP_LOGE(TAG, "Failed to create array");
            cJSON_Delete(root);
            return false;
        }
    }

    // 查找是否存在相同key的任务
    bool found = false;
    int array_size = cJSON_GetArraySize(file_root);
    for (int i = 0; i < array_size; i++)
    {
        cJSON *item = cJSON_GetArrayItem(file_root, i);
        if (!item)
            continue;

        cJSON *item_params = cJSON_GetObjectItem(item, "params");
        if (!item_params)
            continue;

        cJSON *item_tasks = cJSON_GetObjectItem(item_params, "tasks");
        if (!item_tasks)
            continue;

        cJSON *item_task = cJSON_GetArrayItem(item_tasks, 0);
        if (!item_task)
            continue;

        cJSON *item_key = cJSON_GetObjectItem(item_task, "key");
        if (item_key && item_key->valuestring && strcmp(item_key->valuestring, key) == 0)
        {
            // 替换整个对象
            cJSON *new_item = cJSON_Duplicate(root, 1);
            if (new_item)
            {
                cJSON_ReplaceItemInArray(file_root, i, new_item);
                found = true;
                break;
            }
        }
    }

    // 如果没找到相同key的任务，添加新任务
    if (!found)
    {
        cJSON *new_item = cJSON_Duplicate(root, 1);
        if (new_item)
        {
            cJSON_AddItemToArray(file_root, new_item);
        }
    }

    // 写回文件
    char *new_content = cJSON_Print(file_root);
    bool success = false;
    if (new_content)
    {
        success = write_file(new_content);
        free(new_content);
        Update_Nearest_Task();
    }

    cJSON_Delete(root);
    cJSON_Delete(file_root);
    return success;
}

// 将时间字符串转换为秒数
int time_str_to_seconds(const char *time_str)
{
    if (!time_str)
        return -1;

    int hours, minutes, seconds;
    if (sscanf(time_str, "%d:%d:%d", &hours, &minutes, &seconds) != 3)
    {
        ESP_LOGE(TAG, "Invalid time format: %s", time_str);
        return -1;
    }

    if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59 || seconds < 0 || seconds > 59)
    {
        ESP_LOGE(TAG, "Invalid time values: %s", time_str);
        return -1;
    }

    return hours * 3600 + minutes * 60 + seconds;
}

// 将秒数转换为时间字符串
void seconds_to_time_str(long total_seconds, char *time_str, size_t size)
{
    if (!time_str || size < 9)
        return; // 需要至少9个字符的空间 (HH:mm:ss\0)

    int hours = total_seconds / 3600;
    int minutes = (total_seconds % 3600) / 60;
    int seconds = total_seconds % 60;

    snprintf(time_str, size, "%02d:%02d:%02d", hours, minutes, seconds);
}

// 给定一个开始时间字符串(HH:mm:ss\0)和一个keeptime，计算出endtime。
void get_endtime(char **endtime, const char *starttime, int keeptime)
{
    if (endtime == NULL || starttime == NULL)
    {
        return;
    }
    int seconds1 = time_str_to_seconds(starttime);
    int total_seconds = seconds1 + keeptime * 60;
    char endtime_temp[16];
    seconds_to_time_str(total_seconds, endtime_temp, sizeof(endtime_temp));
    printf("截止时间:%s---\n", endtime_temp);
    *endtime = strdup(endtime_temp);
}

// 获取最近任务函数
bool get_nearest_task(const int current_time, char **key, char **datavalue, int *starttime, int *keeptime)
{
    if (!current_time || !key || !datavalue || !starttime || !keeptime)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return false;
    }

    char *file_content = read_file();
    if (!file_content)
    {
        return false;
    }

    cJSON *root = cJSON_Parse(file_content);
    free(file_content);

    if (!root)
    {
        ESP_LOGE(TAG, "No tasks found");
        return false;
    }

    int current_seconds = current_time;
    if (current_seconds < 0)
    {
        cJSON_Delete(root);
        return false;
    }

    int min_diff = 24 * 3600; // 最大差值为24小时
    cJSON *nearest_task = NULL;

    int array_size = cJSON_GetArraySize(root);
    for (int i = 0; i < array_size; i++)
    {
        cJSON *item = cJSON_GetArrayItem(root, i);
        if (!item)
            continue;

        cJSON *params = cJSON_GetObjectItem(item, "params");
        if (!params)
            continue;

        cJSON *tasks = cJSON_GetObjectItem(params, "tasks");
        if (!tasks)
            continue;

        cJSON *task = cJSON_GetArrayItem(tasks, 0);
        if (!task)
            continue;

        cJSON *starttime_item = cJSON_GetObjectItem(task, "starttime");
        if (!starttime_item || !starttime_item->valuestring)
            continue;

        int task_seconds = time_str_to_seconds(starttime_item->valuestring);
        if (task_seconds < 0)
            continue;

        int diff = task_seconds - current_seconds;
        if (diff < 0)
        {
            // 如果任务时间早于当前时间，考虑第二天的时间
            diff += 24 * 3600;
        }

        if (diff < min_diff)
        {
            min_diff = diff;
            nearest_task = task;
        }
    }

    bool success = false;
    if (nearest_task)
    {
        cJSON *key_item = cJSON_GetObjectItem(nearest_task, "key");
        cJSON *datavalue_item = cJSON_GetObjectItem(nearest_task, "datavalue");
        cJSON *starttime_item = cJSON_GetObjectItem(nearest_task, "starttime");
        cJSON *keeptime_item = cJSON_GetObjectItem(nearest_task, "keeptime");

        if (key_item && key_item->valuestring &&
            datavalue_item && datavalue_item->valuestring &&
            starttime_item && starttime_item->valuestring &&
            keeptime_item)
        {
            *key = strdup(key_item->valuestring);
            *datavalue = strdup(datavalue_item->valuestring);
            *starttime = time_str_to_seconds(strdup(starttime_item->valuestring));
            *keeptime = keeptime_item->valueint;
            success = true;
        }
    }

    cJSON_Delete(root);
    return success;
}

// 打印所有任务
void print_all_tasks(void)
{
    char *file_content = read_file();
    if (file_content)
    {
        ESP_LOGI(TAG, "All tasks:\n%s", file_content);
        free(file_content);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to read tasks");
    }
}

// 清空所有任务
bool clear_all_tasks(void)
{
    return write_file("[]");
}
