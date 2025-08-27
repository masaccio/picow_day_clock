/**
 * Copyright (c) 2025 Jon Connell
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef WIFI_SSID
#error "WIFI_SSID is not defined. Please define it in secrets.cmake or via a compile flag."
#endif

#ifndef WIFI_PASSWORD
#error "WIFI_PASSWORD is not defined. Please define it in secrets.cmake or via a compile flag."
#endif

#include <stdarg.h>
#include <string.h>
#include <time.h>

/* Pico SDK includes */
#include "lwip/dns.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

/* Local includes */
#include "fonts_extra.h"
#include "gui_paint_extra.h"
#include "lcd.h"
#include "ntp.h"

#define WIFI_CONNECT_TIMEOUT_MS (10 * 1000)
#define WIFI_ABANDON_TIMEOUT_MS (60 * 1000)

// Determines if given UTC time is in British Summer Time (BST)
int is_bst(struct tm *utc) {
    if (!utc) return false;

    int year = utc->tm_year + 1900;

    // Find last Sunday in March
    struct tm start = {
        .tm_year = year - 1900, .tm_mon = 2, .tm_mday = 31, .tm_hour = 1, .tm_min = 0, .tm_sec = 0, .tm_isdst = 0};
    mktime(&start);
    while (start.tm_wday != 0) {
        start.tm_mday--;
        mktime(&start);
    }

    // Find last Sunday in October
    struct tm end = {
        .tm_year = year - 1900, .tm_mon = 9, .tm_mday = 31, .tm_hour = 1, .tm_min = 0, .tm_sec = 0, .tm_isdst = 1};
    mktime(&end);
    while (end.tm_wday != 0) {
        end.tm_mday--;
        mktime(&end);
    }

    time_t now = mktime(utc);
    time_t start_time = mktime(&start);
    time_t end_time = mktime(&end);

    return now >= start_time && now < end_time;
}

void ntp_timer_callback(void *state, time_t *ntp_time) {
    lcd_state_t *lcd_state = (lcd_state_t *)state;
    lcd_state->ntp_time = *ntp_time;
    lcd_state->ntp_updated = true;
}

void debug_print_time(time_t ntp_time) {
    int bst = is_bst(gmtime(&ntp_time));
    time_t local_time_t = ntp_time + (bst ? 3600 : 0);
    struct tm *local = gmtime(&local_time_t);

    printf("NTP time is %02d/%02d/%04d %02d:%02d:%02d (%s)\r\n", local->tm_mday, local->tm_mon + 1,
           local->tm_year + 1900, local->tm_hour, local->tm_min, local->tm_sec, bst ? "BST" : "GMT");

    // char buffer[256];
    // snprintf(buffer, sizeof(buffer), "NTP time is %02d/%02d/%04d %02d:%02d:%02d (%s)\r\n", local->tm_mday,
    //          local->tm_mon + 1, local->tm_year + 1900, local->tm_hour, local->tm_min, local->tm_sec,
    //          bst ? "BST" : "GMT");
    // print_line(state, buffer);
}

bool connect_to_wifi(const char ssid[], const char password[]) {
    printf("Wi-Fi: attempting connection to '%s'\r\n", ssid);
    if (cyw43_arch_init()) {
        printf("Wi-Fi: failed to initialise CYW43\r\n");
        return 1;
    } else {
        printf("Wi-Fi: CYW43 initialised\r\n");
    }

    cyw43_arch_enable_sta_mode();

    absolute_time_t start_time = get_absolute_time();
    while (true) {
        int ret = cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECT_TIMEOUT_MS);

        if (ret == 0) {
            printf("Wi-Fi: connected to %s\r\n", ssid);
            return true;
        } else if (ret == PICO_ERROR_TIMEOUT) {
            if (absolute_time_diff_us(start_time, get_absolute_time()) > (WIFI_ABANDON_TIMEOUT_MS * 1000)) {
                printf("Wi-Fi: exceeded maximum timeout; giving up\r\n");
                return false;
            }
            printf("Wi-Fi: timeout; trying again\r\n");
        } else if (ret == PICO_ERROR_BADAUTH) {
            printf("Wi-Fi: invalid credentials; giving up\r\n");
            return false;
        } else if (ret == PICO_ERROR_CONNECT_FAILED) {
            printf("Wi-Fi: connection failed for unknown reason; giving up\r\n");
            return false;
        } else {
            printf("Wi-Fi: unknown error %d; giving up\r\n", ret);
            return false;
        }
    }
}

int main() {
    stdio_init_all();

    if (!connect_to_wifi(WIFI_SSID, WIFI_PASSWORD)) {
        return 1;
    }

    lcd_state_t *lcd_state = lcd_init();
    if (lcd_state == NULL) {
        printf("Failed to initialise LCD\r\n");
        return 1;
    }

    ntp_state_t *ntp_state = ntp_init((void *)lcd_state, ntp_timer_callback);
    if (ntp_state == NULL) {
        printf("Failed to initialise NTP\r\n");
        return 1;
    }

    ntp_status_t ntp_status = ntp_get_time(ntp_state);
    if (ntp_status != NTP_STATUS_SUCCESS) {
        printf("NTP get time failed\r\n");
        return 1
    } else {
        debug_print_time(lcd_state->ntp_time);
    }

    cyw43_arch_deinit();
    return 0;
}

// int main() {
//     stdio_init_all();

//     lcd_state_t *lcd_state = init_lcd();
//     if (!wifi_connect_with_timeout()) {
//         printf("Failed to connect to Wi-Fi\r\n");
//         return 1;
//     }

//     // print_line(lcd_state, "Wi-Fi initialised:");
//     // print_line(lcd_state, WIFI_SSID);
//     printf("Connected to %s\r\n", WIFI_SSID);

//     bool ntp_updated = run_ntp_blocking(lcd_state);
//     if (ntp_updated) {
//         print_line(lcd_state, "NTP updated");
//         debug_print_time(lcd_state->ntp_time);
//     } else {
//         print_line(lcd_state, "NTP update failed");
//     }

//     // run_ntp_test(lcd_state);

//     Paint_SetRotate(ROTATE_0);
//     print_line(lcd_state, "LCD initialised");
//     LCD_1IN47_Display(lcd_state->frame_buffer);

//     free(lcd_state->frame_buffer);
//     DEV_Module_Exit();
//     cyw43_arch_deinit();
//     return 0;
// }
