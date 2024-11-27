#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2s_std.h"

esp_err_t esp_board_init();

esp_err_t esp_i2s_read(int32_t *buffer, int buffer_len);

esp_err_t esp_audio_play(const int16_t *data, size_t size);

esp_err_t esp_spiffs_mount();

esp_err_t esp_spiffs_unmount();

i2s_chan_handle_t esp_init_i2s_output(void);
