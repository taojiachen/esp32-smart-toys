#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "bsp_board.h"
#include "esp_err.h"
#include "esp_board_init.h"

esp_err_t esp_spiffs_mount()
{
    return bsp_spiffs_mount();
}

esp_err_t esp_spiffs_unmount()
{
    return bsp_spiffs_unmount();
}

int esp_get_feed_channel()
{
    return bsp_get_feed_channel();
}

esp_err_t esp_i2s_read(int32_t *buffer, int buffer_len)
{
    return bsp_i2s_read(buffer, buffer_len);
}

esp_err_t esp_board_init()
{
    return bsp_board_init();
}

esp_err_t esp_audio_play(const int16_t *data, size_t size)
{
    return bsp_audio_play(data, size);
}
