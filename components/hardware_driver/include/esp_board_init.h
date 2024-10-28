#pragma once

#include <stdbool.h>
#include "esp_err.h"

esp_err_t esp_board_init();

int esp_get_feed_channel();

esp_err_t esp_i2s_read(int32_t *buffer, int buffer_len);

esp_err_t esp_audio_play(const int16_t *data, size_t size);

esp_err_t esp_spiffs_mount();

esp_err_t esp_spiffs_unmount();