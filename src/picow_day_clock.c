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
#include "font.h"
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

static void ntp_timer_callback(void *state, time_t *ntp_time)
{
    clock_state_t *clock_state = (clock_state_t *)state;
    clock_state->ntp_time = *ntp_time;
}

int main()
{
    clock_state_t *clock_state = (clock_state_t *)calloc(1, sizeof(clock_state_t));

    stdio_init_all();

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

    lcd_clear_screen(clock_state->lcd1, BLACK);
    lcd_print_line(clock_state->lcd1, WHITE, "SUCCESS LCD 1!");
    lcd_print_line(clock_state->lcd1, RED, "Magic number: %d", get_rand_32() % 50);
    fb_draw_rectangle(clock_state->lcd1->fb, 0, 100, 172, 320, GREEN);
    sleep_ms(1000);

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

    static char clock_chars[] = {'A', 'D', 'E', 'F', 'H', 'I', 'M', 'N', 'O', 'R', 'S', 'T', 'U',
                                 'W', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0};

    for (char *p = clock_chars; *p != 0; p++) {
        (void)fb_write_char(clock_state->lcd2->fb, 45, 0, *p, &clock_digit_font, GREEN, BLACK);
        lcd_update_screen(clock_state->lcd2);
        sleep_ms(500);
    }
    lcd_clear_screen(clock_state->lcd2, BLACK);

    if (!connect_to_wifi(WIFI_SSID, WIFI_PASSWORD)) {
        return 1;
    }

    clock_state->ntp_state = ntp_init((void *)clock_state, ntp_timer_callback);
    if (clock_state->ntp_state == NULL) {
        CLOCK_DEBUG("NTP: failed to initialise\r\n");
        return 1;
    }

    sleep_ms(1000);
    lcd_clear_screen(clock_state->lcd1, BLACK);
    lcd_clear_screen(clock_state->lcd2, BLACK);

    int delay_ms = 2000;
    for (int ii = 0; ii < 10; ii++) {
        ntp_status_t ntp_status = ntp_get_time(clock_state->ntp_state);
        if (ntp_status == NTP_STATUS_KOD) {
            delay_ms *= 2;
            CLOCK_DEBUG("NTP: backing off: new delay is %d ms\r\n", delay_ms);
        } else if (ntp_status != NTP_STATUS_SUCCESS) {
            CLOCK_DEBUG("NTP: get time failed with error %d; exiting\r\n", ntp_status);
            return 1;
        } else {
            /* Time string is of the form "23:59:59 (BST)" */
            const char *time_str = time_as_string(clock_state->ntp_time);
            CLOCK_DEBUG("NTP: time is %s\r\n", time_str);
            lcd_print_line(clock_state->lcd1, WHITE, time_str);
            (void)fb_write_char(clock_state->lcd2->fb, 45, 0, time_str[7], &clock_digit_font, GREEN, BLACK);
            lcd_update_screen(clock_state->lcd1);
            lcd_update_screen(clock_state->lcd2);
        }
        sleep_ms(delay_ms);
    }

    disconnect_from_wifi();
    return 0;
}
