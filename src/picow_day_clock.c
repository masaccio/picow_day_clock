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

#if defined(_WIN32)
struct timeval
{
    long tv_sec;  /* seconds */
    long tv_usec; /* microseconds */
};
extern int gettimeofday(struct timeval *tp, void *tzp);
#else
#include <sys/time.h>
#endif

// Pico SDK
#ifndef TEST_MODE
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/util/datetime.h"
#else
#include "tests/mock.h"
#endif

// Local includes
#include "clock.h"
#include "fb.h"
#include "font.h"
#include "lcd.h"
#include "ntp.h"
#include "wifi.h"

typedef struct
{
    unsigned int DC;
    unsigned int CS;
} lcd_pin_config_t;

static lcd_pin_config_t lcd_pin_config[NUM_LCDS] = {
    /* LCD 1 */ {.DC = LCD1_GPIO_DC, .CS = LCD1_GPIO_CS},
    /* LCD 2 */ {.DC = LCD2_GPIO_DC, .CS = LCD2_GPIO_CS},
    /* LCD 3 */ {.DC = LCD3_GPIO_DC, .CS = LCD3_GPIO_CS},
    /* LCD 4 */ {.DC = LCD4_GPIO_DC, .CS = LCD4_GPIO_CS},
    /* LCD 5 */ {.DC = LCD5_GPIO_DC, .CS = LCD5_GPIO_CS},
    /* LCD 6 */ {.DC = LCD6_GPIO_DC, .CS = LCD6_GPIO_CS},
    /* LCD 7 */ {.DC = LCD7_GPIO_DC, .CS = LCD7_GPIO_CS},
};

static char day_of_week[][4] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

// Callback from NTP called when an NTP request is successful
void ntp_timer_callback(void *state, time_t *ntp_time)
{
    clock_state_t *clock_state = (clock_state_t *)state;
    clock_state->ntp_time = *ntp_time;
}

static int last_day_of_month(int day, int month, int year)
{
    // Compute the last day of the month using Zeller's congruence
    // See https://en.wikipedia.org/wiki/Zeller%27s_congruence
    if (month < 3) {
        month += 12;
        year -= 1;
    }
    int K = year % 100;
    int J = year / 100;
    int h = (day + (13 * (month + 1)) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;

    // Zeller: 0=Saturday so convert to Unix-offset where 0=Sunday
    int d = (h + 6) % 7;
    return d;
}

// Returns a static struct tm* with the DST start time (last Sunday in March at 01:00 UTC)
struct tm *dst_start(int tm_year)
{
    static struct tm tm_start;

    tm_start.tm_year = tm_year; /* Years since 1900 */
    tm_start.tm_mon = 2;        /* March (0 = Jan) */
    tm_start.tm_mday = 31;      /* Start from end of month */
    tm_start.tm_hour = 1;       /* 01:00 UTC */
    tm_start.tm_min = 0;
    tm_start.tm_sec = 0;
    tm_start.tm_isdst = 0;

    int wday = last_day_of_month(31, 3, tm_year + 1900);
    tm_start.tm_mday = 31 - wday;
    tm_start.tm_wday = 0;

    return &tm_start;
}

struct tm *dst_end(int tm_year)
{
    static struct tm tm_end;

    tm_end.tm_year = tm_year; /* Years since 1900 */
    tm_end.tm_mon = 9;        /* October (0 = Jan) */
    tm_end.tm_mday = 31;      /* Start from end of month */
    tm_end.tm_hour = 1;       /* 01:00 UTC */
    tm_end.tm_min = 0;
    tm_end.tm_sec = 0;
    tm_end.tm_isdst = 0;

    int wday = last_day_of_month(31, 10, tm_year + 1900);
    tm_end.tm_mday = 31 - wday;
    tm_end.tm_wday = 0;

    return &tm_end;
}

// The Pico SDK doesn't have timegm() for UTC calculations since it doesn't
// manage timezones, but for testing we need to ensure we stick to UTC
time_t tm_to_epoch(struct tm *tm)
{
#if defined(__APPLE__) || defined(__linux__)
    return timegm(tm); // UTC on host
#else
    return mktime(tm); // Pico: local = UTC
#endif
}

// Determines if given UTC time is in daylight savings time. We assume
// the European convention of time changing at 0100 on the last Sundays
// in March and October
bool time_is_dst(struct tm *utc)
{
    time_t now = tm_to_epoch(utc);
    time_t start_time = tm_to_epoch(dst_start(utc->tm_year));
    time_t end_time = tm_to_epoch(dst_end(utc->tm_year));

    return now >= start_time && now < end_time;
}

const char *time_as_string(time_t t)
{
    static char buffer[32];

    int dst = time_is_dst(gmtime(&t));
    time_t local_time_t = t + (dst ? 3600 : 0);
    struct tm *local = gmtime(&local_time_t);

    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d%s", local->tm_hour, local->tm_min, local->tm_sec,
             dst ? " (DST)" : "");
    return (const char *)buffer;
}

bool clock_timer_callback(struct repeating_timer *t)
{
    clock_state_t *state = (clock_state_t *)t->user_data;

    /* Adjust the NTP drift a second at a time */
    if (state->ntp_drift > 0) {
        state->ntp_drift--;
    } else if (state->ntp_drift < 0) {
        state->ntp_drift++;
    }
    time_t now = time(NULL) + state->ntp_drift;
    struct tm *current_time = gmtime(&now);

    if (state->init_done == false || current_time->tm_sec == 0) {
        if (time_is_dst(current_time)) {
            /* Apply daylight savings */
            if (current_time->tm_hour == 23) {
                current_time->tm_hour = 0;
                current_time->tm_wday = (current_time->tm_wday + 1) % 7;
            } else {
                current_time->tm_hour++;
            }
        }

        static char lcd_digits[NUM_LCDS + 1];
        strncpy(lcd_digits, day_of_week[current_time->tm_wday], 3);
        lcd_digits[3] = '0' + (current_time->tm_hour / 10);
        lcd_digits[4] = '0' + (current_time->tm_hour % 10);
        lcd_digits[5] = '0' + (current_time->tm_min / 10);
        lcd_digits[6] = '0' + (current_time->tm_min % 10);
        lcd_digits[7] = '\0';

        for (unsigned int ii = 0; ii < NUM_LCDS; ii++) {
            if (state->current_lcd_digits[ii] != lcd_digits[ii]) {
#ifndef TEST_MODE
                lcd_clear_screen(state->lcd_states[ii], BLACK);
                lcd_print_clock_digit(state->lcd_states[ii], GREEN, lcd_digits[ii]);
                lcd_update_screen(state->lcd_states[ii]);
#endif
                state->current_lcd_digits[ii] = lcd_digits[ii];
            }
        }

        if ((now - state->ntp_last_sync) >= NTP_SYNC_INTERVAL_SEC) {
            ntp_status_t ntp_status = ntp_get_time(state->ntp_state);
            if (ntp_status == NTP_STATUS_KOD) {
                state->ntp_interval *= 2;
                CLOCK_DEBUG("NTP: backing off: new delay is %d minutes\r\n", state->ntp_interval);
            } else if (ntp_status != NTP_STATUS_SUCCESS) {
                state->ntp_last_sync = state->ntp_time;
                CLOCK_DEBUG("NTP: get time failed with error %d; exiting\r\n", ntp_status);
                // TODO: Do something with the display to indicate a problem
            } else {
                int drift = state->ntp_time - now;
                CLOCK_DEBUG("NTP sync at %s; drift = %ds\r\n", time_as_string(state->ntp_time), drift);
                state->ntp_last_sync = state->ntp_time;
                state->ntp_drift = drift;
                struct timeval tv = {.tv_sec = state->ntp_time, .tv_usec = 0};
                settimeofday(&tv, NULL);
            }
        }
        state->init_done = true;
    }

    return true; // Keep repeating
}

#ifdef TEST_MODE
int test_main(void)
#define lcd_print_line(state, line_num, color, msg)                                                                    \
    (void)line_num;                                                                                                    \
    (void)color;                                                                                                       \
    mock_printf("LCD: %s", msg)
#else
int main(void)
#endif
{
    stdio_init_all();

    clock_state_t *state = (clock_state_t *)calloc(1, sizeof(clock_state_t));
    if (state == NULL) {
        printf("Failed to allocate clock state\r\n");
        return 1;
    }
    for (unsigned int ii = 0; ii < NUM_LCDS; ii++) {
        bool reset = (ii == 0) ? true : false;
        state->lcd_states[ii] = lcd_init(/* RST  */ LCD_GPIO_RST,
                                         /* DC   */ lcd_pin_config[ii].DC,
                                         /* BL   */ LCD_GPIO_BL,
                                         /* CS   */ lcd_pin_config[ii].CS,
                                         /* CLK  */ LCD_GPIO_CLK,
                                         /* MOSI */ LCD_GPIO_MOSI, reset);
        if (state->lcd_states[ii] == NULL) {
            printf("LCD %d: failed to initialise\r\n", ii + 1);
            return 1;
        }
        lcd_clear_screen(state->lcd_states[ii], BLACK);
    }

    uint16_t line_num = 1;
    lcd_print_line(state->lcd_states[0], line_num++, GREEN, "LCD init OK");
    lcd_update_screen(state->lcd_states[0]);

    wifi_status_t wifi_status = connect_to_wifi(WIFI_SSID, WIFI_PASSWORD);
    switch (wifi_status) {
        case WIFI_STATUS_SUCCESS:
            lcd_print_line(state->lcd_states[0], line_num++, GREEN, "Wi-Fi connect OK");
            lcd_update_screen(state->lcd_states[0]);
            break;
        case WIFI_STATUS_INIT_FAIL:
            lcd_print_line(state->lcd_states[0], line_num, RED, "Wi-Fi init error");
            lcd_update_screen(state->lcd_states[0]);
            return 1;
        case WIFI_STATUS_TIMEOUT:
            lcd_print_line(state->lcd_states[0], line_num, RED, "Wi-Fi timeout error");
            lcd_update_screen(state->lcd_states[0]);
            return 1;
        case WIFI_STATUS_BAD_AUTH:
            lcd_print_line(state->lcd_states[0], line_num, RED, "Wi-Fi auth error");
            lcd_update_screen(state->lcd_states[0]);
            return 1;
        case WIFI_STATUS_CONNECT_FAILED:
            lcd_print_line(state->lcd_states[0], 2, RED, "Wi-Fi connect error");
            lcd_update_screen(state->lcd_states[0]);
            return 1;
        default: // whould be WIFI_STATUS_UNKNOWN_ERROR
            lcd_print_line(state->lcd_states[0], line_num, RED, "Wi-Fi unknown error");
            lcd_update_screen(state->lcd_states[0]);
            return 1;
    }

    state->ntp_state = ntp_init((void *)state, ntp_timer_callback);
    if (state->ntp_state == NULL) {
        lcd_print_line(state->lcd_states[0], line_num, RED, "NTP init error");
        lcd_update_screen(state->lcd_states[0]);
        return 1;
    }
    lcd_print_line(state->lcd_states[0], line_num++, GREEN, "NTP init OK");
    lcd_update_screen(state->lcd_states[0]);

    ntp_status_t ntp_status = ntp_get_time(state->ntp_state);
    switch (ntp_status) {
        case NTP_STATUS_SUCCESS:
            lcd_print_line(state->lcd_states[0], line_num++, GREEN, "NTP time OK");
            lcd_update_screen(state->lcd_states[0]);
            break;
        case NTP_STATUS_DNS_ERROR:
            lcd_print_line(state->lcd_states[0], line_num, RED, "NTP DNS failed");
            lcd_update_screen(state->lcd_states[0]);
            return 1;
        case NTP_STATUS_TIMEOUT:
            lcd_print_line(state->lcd_states[0], line_num, RED, "NTP timeout");
            lcd_update_screen(state->lcd_states[0]);
            return 1;
        case NTP_STATUS_MEMORY_ERROR:
            lcd_print_line(state->lcd_states[0], line_num, RED, "NTP memory error");
            lcd_update_screen(state->lcd_states[0]);
            return 1;
        default: // Should be NTP_STATUS_INVALID_RESPONSE
            lcd_print_line(state->lcd_states[0], line_num, RED, "NTP unknown error");
            lcd_update_screen(state->lcd_states[0]);
            return 1;
    }

    state->ntp_drift = 0;
    state->ntp_last_sync = state->ntp_time;
    state->ntp_interval = NTP_SYNC_INTERVAL_SEC;

    // Set the system clock to the NTP time
    struct timeval tv = {.tv_sec = state->ntp_time, .tv_usec = 0};
    settimeofday(&tv, NULL);

    // Call the timer every second to enable us to slowly change the clock if
    // the system clock drifts from NTP time
    add_repeating_timer_ms(1 * 1000, clock_timer_callback, state, &state->timer);

#ifndef TEST_MODE
    while (true) {
        sleep_ms(1000);
    }
#endif

    disconnect_from_wifi();
    return 0;
}
