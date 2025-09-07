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

#include <stdio.h>
#include <string.h>
#include <time.h>

/* Pico SDK */
#ifndef TEST_MODE
#include "hardware/powman.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/util/datetime.h"
#else
#include "tests/mock.h"
#endif

/* Local includes */
#include "common.h"
#include "fb.h"
#include "font.h"
#include "lcd.h"
#include "ntp.h"
#include "wifi.h"

typedef struct clock_state_t
{
    /* NTP state */
    time_t ntp_time;
    ntp_state_t *ntp_state;
    /* LCD state */
    lcd_state_t *lcd1;
    lcd_state_t *lcd2;
    char clock_buffer[8];
    /* Timer state */
    uint64_t epoch_ms;
    uint32_t tick_count;
    uint32_t ntp_sync_interval_minutes;
    repeating_timer_t timer;
} clock_state_t;

typedef struct simple_time_t
{
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    const char *day_of_week;
} simple_time_t;

static char day_of_week[][4] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

/* Callback from NTP called when an NTP request is successful */
static void ntp_timer_callback(void *state, time_t *ntp_time)
{
    clock_state_t *clock_state = (clock_state_t *)state;
    clock_state->ntp_time = *ntp_time;
}

simple_time_t current_time(uint64_t time_ms)
{
    static simple_time_t time;
    uint64_t now_sec = time_ms / 1000;

    uint32_t sec_in_day = now_sec % 86400;

    time.hours = sec_in_day / 3600;
    time.minutes = (sec_in_day % 3600) / 60;
    time.seconds = sec_in_day % 60;
    time.day_of_week = day_of_week[(4 + (now_sec / 86400)) % 7];
    return time;
}

static bool minute_timer_callback(struct repeating_timer *t)
{
    clock_state_t *state = (clock_state_t *)t->user_data;
    state->tick_count++;

    uint64_t time_ms = powman_timer_get_ms() + state->epoch_ms;
    simple_time_t time_now = current_time(time_ms);
    printf("Current time is %s %02d:%02d:%02d\n", time_now.day_of_week, time_now.hours, time_now.minutes,
           time_now.seconds);

    lcd_clear_screen(state->lcd1, BLACK);
    lcd_print_clock_digit(state->lcd1, GREEN, time_now.seconds % 10 + '0');
    lcd_update_screen(state->lcd1);
    lcd_clear_screen(state->lcd2, BLACK);
    lcd_print_clock_digit(state->lcd2, GREEN, time_now.seconds / 10 + '0');
    lcd_update_screen(state->lcd2);

    if (state->tick_count >= NTP_SYNC_INTERVAL_MINUTES) {
        ntp_status_t ntp_status = ntp_get_time(state->ntp_state);
        if (ntp_status == NTP_STATUS_KOD) {
            state->ntp_sync_interval_minutes *= 2;
            CLOCK_DEBUG("NTP: backing off: new delay is %d minutes\r\n", state->ntp_sync_interval_minutes);
        } else if (ntp_status != NTP_STATUS_SUCCESS) {
            state->tick_count = 0;
            CLOCK_DEBUG("NTP: get time failed with error %d; exiting\r\n", ntp_status);
            /* TODO: Do something with the display to indicate a problem */
        } else {
            printf("NTP sync at %s\r\n", time_as_string(state->ntp_time));
            powman_timer_set_ms(state->ntp_time * 1000 - state->epoch_ms);
            state->tick_count = 0;
        }
    }
    return true;
}

#ifdef TEST_MODE
int test_main(void)
#else
int main(void)
#endif
{
    clock_state_t *state = (clock_state_t *)calloc(1, sizeof(clock_state_t));
    if (state == NULL) {
        printf("Failed to allocate clock state\r\n");
        return 1;
    }

    stdio_init_all();
    powman_timer_start();

    state->lcd1 = lcd_init(/* RST */ 12,
                           /* DC */ 6,
                           /* BL */ 13,
                           /* CS */ 7,
                           /* CLK */ 10,
                           /* MOSI */ 11, /* reset */ true);

    if (state->lcd1 == NULL) {
        printf("LCD 1: failed to initialise\r\n");
        return 1;
    }
    lcd_clear_screen(state->lcd1, BLACK);
    lcd_print_text(state->lcd1, WHITE, "");
    lcd_print_text(state->lcd1, GREEN, "LCD init OK");
    lcd_update_screen(state->lcd1);

    state->lcd2 = lcd_init(/* RST */ 12,
                           /* DC */ 8,
                           /* BL */ 13,
                           /* CS */ 9,
                           /* CLK */ 10,
                           /* MOSI */ 11, /* reset */ false);
    if (state->lcd2 == NULL) {
        printf("LCD 2: failed to initialise\r\n");
        return 1;
    }
    lcd_clear_screen(state->lcd2, BLACK);

    if (connect_to_wifi(WIFI_SSID, WIFI_PASSWORD) != 0) {
        return 1;
    }
    lcd_print_text(state->lcd1, GREEN, "Wi-Fi connect OK");
    lcd_update_screen(state->lcd1);

    state->ntp_state = ntp_init((void *)state, ntp_timer_callback);
    if (state->ntp_state == NULL) {
        CLOCK_DEBUG("NTP: failed to initialise\r\n");
        return 1;
    }
    lcd_print_text(state->lcd1, GREEN, "NTP init OK");
    lcd_update_screen(state->lcd1);

    ntp_status_t ntp_status = ntp_get_time(state->ntp_state);
    if (ntp_status != NTP_STATUS_SUCCESS) {
        CLOCK_DEBUG("NTP: get time failed with error %d; exiting\r\n", ntp_status);
        lcd_print_text(state->lcd1, RED, "NTP error");
        lcd_update_screen(state->lcd1);
        return 1;
    }
    lcd_print_text(state->lcd1, GREEN, "NTP get time OK");
    lcd_update_screen(state->lcd1);

    state->tick_count = 0;
    state->ntp_sync_interval_minutes = NTP_SYNC_INTERVAL_MINUTES;
    state->epoch_ms = state->ntp_time * 1000;
    powman_timer_set_ms(0);
    /* TODO: change this to a minute */
    add_repeating_timer_ms(1 * 1000, minute_timer_callback, state, &state->timer);

    while (true) {
        sleep_ms(1000);
    }

    disconnect_from_wifi();
    return 0;
}
