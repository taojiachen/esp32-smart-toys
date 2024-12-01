#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/i2s_std.h"

// INMP441 引脚定义
#define INMP441_WS_PIN 4
#define INMP441_SCK_PIN 5
#define INMP441_SD_PIN 6

// MAX98357A 引脚定义
#define MAX98357A_LRC_PIN 16
#define MAX98357A_BCLK_PIN 15
#define MAX98357A_DIN_PIN 7

#define SAMPLE_RX_RATE 16000
#define SAMPLE_TX_RATE 44100
#define SAMPLE_BITS 32 // INMP441 uses 32-bit samples
#define DMA_BUF_COUNT 8
#define DMA_BUF_LEN 1024
#define BUFFER_SIZE 1024
#define WAKE_WORD_NUM 2

esp_err_t bsp_board_init();

// i2s_chan_handle_t init_i2s_output(void);

esp_err_t bsp_i2s_read(int32_t *buffer, int buffer_len);

esp_err_t bsp_spiffs_mount(void);

esp_err_t bsp_spiffs_unmount(void);

