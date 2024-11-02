/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#ifndef _APP_SNTP_H_
#define _APP_SNTP_H_


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the SNTP application
 *
 * This function initializes the SNTP application. It increments the boot count and checks if the time is set.
 * If the time is not set (i.e., the year is before 2016), it obtains the time by connecting to WiFi and getting the time over NTP.
 * After obtaining the time, it logs the current date and time in Shanghai.
 *
 */
void app_sntp_init(void);

#ifdef __cplusplus
}
#endif

#endif
