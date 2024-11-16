/* SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <dirent.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_board_init.h"
#include "file_iterator.h"
#include "app_play_music.h"

static const char *TAG = "app_play_music";

static file_iterator_instance_t *file_iterator;

static bool isMP3(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (dot && dot != filename) {
        if (strcasecmp(dot, ".mp3") == 0) {
            return true;
        }
    }
    return false;
}

// static void play_index(int index)
// {
//     ESP_LOGI(TAG, "play_index(%d)", index);

//     char filename[128];
//     int retval = file_iterator_get_full_path_from_index(file_iterator, index, filename, sizeof(filename));

//     const char* name = file_iterator_get_name_from_index(file_iterator, index);

//     if (retval == 0) {
//         ESP_LOGE(TAG, "unable to retrieve filename");
//         return;
//     }

//     FILE *fp = fopen(filename, "rb");
//     // if (fp) {
//     //     audio_player_play(fp);
//     // } else {
//     //     ESP_LOGE(TAG, "unable to open index %d, filename '%s'", index, filename);
//     // }
// }



