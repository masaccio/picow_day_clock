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
#include "pico/multicore.h"
#include "pico/stdlib.h"

/* Local includes */
#include "common.h"
#include "fb.h"
#include "fonts.h"
#include "lcd.h"
#include "ntp.h"
#include "wifi.h"

typedef struct clock_state_t
{
    bool ntp_updated;
    time_t ntp_time;
    lcd_state_t *lcd1;
    lcd_state_t *lcd2;
    ntp_state_t *ntp_state;
} clock_state_t;

void ntp_timer_callback(void *state, time_t *ntp_time)
{
    clock_state_t *clock_state = (clock_state_t *)state;
    clock_state->ntp_time = *ntp_time;
}

static uint8_t lcd2_frame_buffer[(LCD_HEIGHT + 1) * (LCD_WIDTH / 4)];

int main()
{
    clock_state_t *clock_state = (clock_state_t *)calloc(1, sizeof(clock_state_t));

    stdio_init_all();

    // if (!connect_to_wifi(WIFI_SSID, WIFI_PASSWORD)) {
    //     return 1;
    // }

    // clock_state->ntp_state = ntp_init((void *)clock_state, ntp_timer_callback);
    // if (clock_state->ntp_state == NULL) {
    //     CLOCK_DEBUG("NTP: failed to initialise\r\n");
    //     return 1;
    // }

    // ntp_status_t ntp_status = ntp_get_time(clock_state->ntp_state);
    // if (ntp_status != NTP_STATUS_SUCCESS) {
    //     CLOCK_DEBUG("NTP: get time failed\r\n");
    //     return 1;
    // } else {
    //     const char *time_str = time_as_string(clock_state->ntp_time);
    //     CLOCK_DEBUG("NTP: time is %s\r\n", time_str);
    // }

    clock_state->lcd1 = lcd_init(/* RST */ 12,
                                 /* DC */ 6,
                                 /* BL */ 13,
                                 /* CS */ 7,
                                 /* CLK */ 10,
                                 /* MOSI */ 11, /* reset */ true);

    if (clock_state->lcd1 == NULL) {
        CLOCK_DEBUG("LCD 1: failed to initialise\r\n");
        return 1;
    }
    CLOCK_DEBUG("Display LCD1 4-bit\r\n");
    int x1 = get_rand_32() % 50;
    int x2 = get_rand_32() % 50;
    int y1 = get_rand_32() % 50;
    int y2 = get_rand_32() % 50;
    fb_clear(clock_state->lcd1->fb, 0x00);
    fb_write_string(clock_state->lcd1->fb, 0, 0, "SUCCESS LCD 1!", &TEXT_FONT, 0xff, 0x00);
    fb_draw_rectangle(clock_state->lcd1->fb, x1 + 20, y1 + 40, x1 + 60, y1 + 80, 0xaa);
    fb_draw_rectangle(clock_state->lcd1->fb, x2 + 80, y2 + 100, x2 + 120, y2 + 140, 0x55);
    lcd_update_screen(clock_state->lcd1);

    clock_state->lcd2 = lcd_init(/* RST */ 12,
                                 /* DC */ 8,
                                 /* BL */ 13,
                                 /* CS */ 9,
                                 /* CLK */ 10,
                                 /* MOSI */ 11, /* reset */ false);
    if (clock_state->lcd2 == NULL) {
        CLOCK_DEBUG("LCD 2: failed to initialise\r\n");
        return 1;
    }
    CLOCK_DEBUG("Display LCD2 4-bit\r\n");
    x1 = get_rand_32() % 50;
    x2 = get_rand_32() % 50;
    y1 = get_rand_32() % 50;
    y2 = get_rand_32() % 50;
    fb_clear(clock_state->lcd2->fb, 0x00);
    fb_write_string(clock_state->lcd2->fb, 0, 0, "SUCCESS LCD 2!", &TEXT_FONT, 0xff, 0x00);
    fb_draw_rectangle(clock_state->lcd2->fb, x1 + 20, y1 + 40, x1 + 60, y1 + 80, 0x55);
    fb_draw_rectangle(clock_state->lcd2->fb, x2 + 80, y2 + 100, x2 + 120, y2 + 140, 0xaa);
    lcd_update_screen(clock_state->lcd2);

    // int delay_ms = 2000;
    // for (int ii = 0; ii < 100; ii++) {
    //     ntp_status = ntp_get_time(clock_state->ntp_state);
    //     if (ntp_status == NTP_STATUS_KOD) {
    //         delay_ms *= 2;
    //         CLOCK_DEBUG("NTP: backing off: new delay is %d ms\r\n", delay_ms);
    //     } else if (ntp_status != NTP_STATUS_SUCCESS) {
    //         CLOCK_DEBUG("NTP: get time failed with error %d; exiting\r\n", ntp_status);
    //         return 1;
    //     } else {
    //         const char *time_str = time_as_string(clock_state->ntp_time);
    //         CLOCK_DEBUG("NTP: [iter %02d] time is %s\r\n", ii + 1, time_str);
    //         lcd_print_line(clock_state->lcd1, time_str);
    //         update_screen(clock_state->lcd1);
    //     }
    //     sleep_ms(delay_ms);
    // }

    // disconnect_from_wifi();
    return 0;
}
