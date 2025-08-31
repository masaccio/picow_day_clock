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

/* Pico SDK */
#include "lwip/dns.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

/* Local includes */
#include "common.h"
#include "fonts_extra.h"
#include "gui_paint_extra.h"
#include "lcd.h"
#include "ntp.h"

#include "Debug.h" /* Waveshare */

#define WIFI_CONNECT_TIMEOUT_MS (10 * 1000)
#define WIFI_ABANDON_TIMEOUT_MS (60 * 1000)
#define WIFI_BAD_AUTH_RETRY_COUNT 3
#define WIFI_BAD_AUTH_RETRY_DELAY_MS 500

typedef struct clock_state_t
{
    bool ntp_updated;
    time_t ntp_time;
    lcd_state_t *lcd_state;
    ntp_state_t *ntp_state;
} clock_state_t;

void ntp_timer_callback(void *state, time_t *ntp_time)
{
    clock_state_t *clock_state = (clock_state_t *)state;
    clock_state->ntp_time = *ntp_time;
}

bool connect_to_wifi(const char ssid[], const char password[])
{
    if (cyw43_arch_init()) {
        CLOCK_DEBUG("Wi-Fi: failed to initialise CYW43\r\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    absolute_time_t start_time = get_absolute_time();
    int bad_auth_count = 0;
    while (true) {
        int ret = cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECT_TIMEOUT_MS);

        if (ret == 0) {
            CLOCK_DEBUG("Wi-Fi: connected to %s\r\n", ssid);
            return true;
        } else if (ret == PICO_ERROR_TIMEOUT) {
            if (absolute_time_diff_us(start_time, get_absolute_time()) > (WIFI_ABANDON_TIMEOUT_MS * 1000)) {
                CLOCK_DEBUG("Wi-Fi: exceeded maximum timeout; giving up\r\n");
                return false;
            }
            CLOCK_DEBUG("Wi-Fi: timeout; trying again\r\n");
        } else if (ret == PICO_ERROR_BADAUTH) {
            if (++bad_auth_count > WIFI_BAD_AUTH_RETRY_COUNT) {
                CLOCK_DEBUG("Wi-Fi: too many bad authentication failures; giving up\r\n");
                return false;
            } else {
                CLOCK_DEBUG("Wi-Fi: invalid credentials; retrying...\r\n");
                sleep_ms(WIFI_BAD_AUTH_RETRY_DELAY_MS);
            }
        } else if (ret == PICO_ERROR_CONNECT_FAILED) {
            CLOCK_DEBUG("Wi-Fi: connection failed for unknown reason; giving up\r\n");
            return false;
        } else {
            CLOCK_DEBUG("Wi-Fi: unknown error %d; giving up\r\n", ret);
            return false;
        }
    }
}

int main()
{
    clock_state_t *clock_state = (clock_state_t *)calloc(1, sizeof(clock_state_t));

    stdio_init_all();

    if (!connect_to_wifi(WIFI_SSID, WIFI_PASSWORD)) {
        return 1;
    }

    clock_state->ntp_state = ntp_init((void *)clock_state, ntp_timer_callback);
    if (clock_state->ntp_state == NULL) {
        CLOCK_DEBUG("NTP: failed to initialise\r\n");
        return 1;
    }

    ntp_status_t ntp_status = ntp_get_time(clock_state->ntp_state);
    if (ntp_status != NTP_STATUS_SUCCESS) {
        CLOCK_DEBUG("NTP: get time failed\r\n");
        return 1;
    } else {
        const char *time_str = time_as_string(clock_state->ntp_time);
        CLOCK_DEBUG("NTP: time is %s\r\n", time_str);
    }

    clock_state->lcd_state = lcd_init();
    if (clock_state->lcd_state == NULL) {
        CLOCK_DEBUG("LCD: failed to initialise\r\n");
        return 1;
    }

    clear_screen(clock_state->lcd_state);
    print_line(clock_state->lcd_state, "SUCCESS!");
    update_screen(clock_state->lcd_state);

    int delay_ms = 2000;
    for (int ii = 0; ii < 100; ii++) {
        ntp_status = ntp_get_time(clock_state->ntp_state);
        if (ntp_status == NTP_STATUS_KOD) {
            delay_ms *= 2;
            CLOCK_DEBUG("NTP: backing off: new delay is %d ms\r\n", delay_ms);
        } else if (ntp_status != NTP_STATUS_SUCCESS) {
            CLOCK_DEBUG("NTP: get time failed with error %d; exiting\r\n", ntp_status);
            return 1;
        } else {
            const char *time_str = time_as_string(clock_state->ntp_time);
            CLOCK_DEBUG("NTP: [iter %02d] time is %s\r\n", ii + 1, time_str);
            print_line(clock_state->lcd_state, time_str);
            update_screen(clock_state->lcd_state);
        }
        sleep_ms(delay_ms);
    }

    cyw43_arch_deinit();
    return 0;
}
