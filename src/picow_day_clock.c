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

lcd_config_t lcd1_config = {
    .RST_gpio = 12, // GPIO12 (shared)
    .DC_gpio = 6,   // GPIO8
    .BL_gpio = 13,  // GPIO13 (shared)
    .CS_gpio = 7,   // GPIO9
    .CLK_gpio = 10, // GPIO10 (shared)
    .MOSI_gpio = 11 // GPIO11 (shared)
};

lcd_config_t lcd2_config = {
    .RST_gpio = 12, // GPIO12 (shared)
    .DC_gpio = 8,   // GPIO8
    .BL_gpio = 13,  // GPIO13 (shared)
    .CS_gpio = 9,   // GPIO9
    .CLK_gpio = 10, // GPIO10 (shared)
    .MOSI_gpio = 11 // GPIO11 (shared)
};

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

    clock_state->lcd1 = lcd_init(&lcd1_config, 1);
    if (clock_state->lcd1 == NULL) {
        CLOCK_DEBUG("LCD 1: failed to initialise\r\n");
        return 1;
    }
    CLOCK_DEBUG("Display LCD1 4-bit\r\n");
    fb_clear(clock_state->lcd1->frame_buffer, 0x00);
    fb_write_string(clock_state->lcd1->frame_buffer, 0, 0, "SUCCESS LCD 1!", &TEXT_FONT, 0xff, 0x00);
    fb_draw_rectangle(clock_state->lcd1->frame_buffer, 0, 50, 172, 75, 0xaa);
    fb_draw_rectangle(clock_state->lcd1->frame_buffer, 0, 75, 172, 100, 0x55);
    LCD_1IN47_Display_2Bit(&lcd1_config, clock_state->lcd1->frame_buffer_data);

    // sleep_ms(2000);

    clock_state->lcd2 = lcd_init(&lcd2_config, 2);
    if (clock_state->lcd2 == NULL) {
        CLOCK_DEBUG("LCD 2: failed to initialise\r\n");
        return 1;
    }
    CLOCK_DEBUG("Display LCD2 4-bit\r\n");
    fb_clear(clock_state->lcd2->frame_buffer, 0x00);
    fb_write_string(clock_state->lcd2->frame_buffer, 0, 0, "SUCCESS LCD 2!", &TEXT_FONT, 0xff, 0x00);
    fb_draw_rectangle(clock_state->lcd2->frame_buffer, 0, 100, 172, 125, 0x55);
    fb_draw_rectangle(clock_state->lcd2->frame_buffer, 0, 125, 172, 150, 0xaa);
    LCD_1IN47_Display_2Bit(&lcd2_config, clock_state->lcd2->frame_buffer_data);

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
    //         print_line(clock_state->lcd1, time_str);
    //         update_screen(clock_state->lcd1);
    //     }
    //     sleep_ms(delay_ms);
    // }

    // disconnect_from_wifi();
    return 0;
}
