#ifndef WIFI_SSID
#error "WIFI_SSID is not defined. Please define it in config.cmake or via a compile flag."
#endif

#ifndef WIFI_PASSWORD
#error "WIFI_PASSWORD is not defined. Please define it in config.cmake or via a compile flag."
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#ifndef _WIN32 // Linux/macOS build
#include <sys/time.h>
#else // Windows build
#define gmtime_r(t, result) gmtime_s(result, t)
#endif

// Pico SDK
#ifndef TEST_MODE
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/util/datetime.h"
#else
#include "mock.h"

extern int test_main(void);
extern int watchdog_reboot_called;
#endif

// Local includes
#include "bitmap.h"
#include "clock.h"
#include "config.h"

typedef struct
{
    uint16_t DC;
    uint16_t CS;
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

#define STATUS_CASE(STATUS)                                                                                            \
    case STATUS:                                                                                                       \
        return #STATUS;

const char *watchdog_error_to_string(watchdog_error_t status)
{
    switch (status) {
        STATUS_CASE(WATCHDOG_OK)
        STATUS_CASE(WATCHDOG_RESET)
        STATUS_CASE(WATCHDOG_NTP)
        STATUS_CASE(WATCHDOG_WIFI)
    }
    return "UNKNOWN_STATUS";
}

const char *ntp_error_to_string(ntp_error_t status)
{
    switch (status) {
        STATUS_CASE(NTP_OK)
        STATUS_CASE(NTP_INIT_ERROR)
        STATUS_CASE(NTP_DNS_ERROR)
        STATUS_CASE(NTP_TIMEOUT_ERROR)
        STATUS_CASE(NTP_PROTOCOL_ERROR)
        STATUS_CASE(NTP_MEMORY_ERROR)
    }
    return "UNKNOWN_STATUS";
}

const char *wifi_error_to_string(wifi_error_t status)
{
    switch (status) {
        STATUS_CASE(WIFI_OK)
        STATUS_CASE(WIFI_INIT_ERROR)
        STATUS_CASE(WIFI_TIMEOUT_ERROR)
        STATUS_CASE(WIFI_AUTH_ERROR)
        STATUS_CASE(WIFI_CONNECT_ERROR)
        STATUS_CASE(WIFI_UNKNOWN_ERROR)
    }
    return "UNKNOWN_STATUS";
}

#ifdef TEST_MODE
// In test mode, we want to return from main() but fatal_error() cannot return
// so we use setjmp/longjmp to break out
#include <setjmp.h>

static jmp_buf fatal_jmp_buf;
persistent_state_t persistent_state;

extern void mock_update_icons(watchdog_error_t watchdog_error, ntp_error_t ntp_error, wifi_error_t wifi_error);

// In test mode, key status updates to the LCD are treated like a printf()
// so that the test harness can check the sequence of events.
#define lcd_print_line(state, line_num, color, msg)                                                                    \
    (void)line_num;                                                                                                    \
    (void)color;                                                                                                       \
    mock_printf(msg)

#define lcd_update_icons(state, watchdog, ntp, wifi) mock_update_icons(watchdog, ntp, wifi)

static void __attribute__((noreturn)) fatal_reset(clock_state_t *state, ntp_error_t ntp_error, wifi_error_t wifi_error)
{
    (void)state;
    mock_printf("Watchdog: %s/%s", ntp_error_to_string(ntp_error), wifi_error_to_string(wifi_error));
    persistent_state.ntp_error = ntp_error;
    persistent_state.wifi_error = wifi_error;
    watchdog_reboot((uint32_t)0, SRAM_END, (uint32_t)0 /* delay_ms */);
    // Returns into main() which will then exit with status=1
    longjmp(fatal_jmp_buf, 1);
}
#else // Pico target
persistent_state_t persistent_state __attribute__((section(".uninitialized_data")));

static void __attribute__((noreturn)) fatal_reset(clock_state_t *state, ntp_error_t ntp_error, wifi_error_t wifi_error)
{
    lcd_update_icons(state->lcd_states[0], WATCHDOG_RESET, ntp_error, wifi_error);
    persistent_state.ntp_error = ntp_error;
    persistent_state.wifi_error = wifi_error;
    watchdog_reboot((uint32_t)0, SRAM_END, (uint32_t)0 /* delay_ms */);
    while (1)
        __wfi(); // hang until reset
}
#endif

static char day_of_week[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Callback from NTP called when an NTP request is successful
void ntp_timer_callback(void *state, time_t *ntp_time)
{
    clock_state_t *clock_state = (clock_state_t *)state;
    clock_state->ntp_time = *ntp_time;
}

int last_day_of_month(int day, int month, int year)
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
static struct tm *dst_start(int tm_year)
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

static struct tm *dst_end(int tm_year)
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
int time_is_dst(struct tm *utc)
{
    time_t now = tm_to_epoch(utc);
    time_t start_time = tm_to_epoch(dst_start(utc->tm_year));
    time_t end_time = tm_to_epoch(dst_end(utc->tm_year));

    return now >= start_time && now < end_time;
}

const char *time_as_string(time_t t)
{
    static char buffer[32];
    struct tm tm_utc;

    const char *suffix = "";
    gmtime_r(&t, &tm_utc);
    time_t local_epoch = t;
    if (time_is_dst(&tm_utc)) {
        local_epoch += 3600;
        suffix = " (DST)";
    }
    struct tm tm_local;
    gmtime_r(&local_epoch, &tm_local);
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d%s", tm_local.tm_hour, tm_local.tm_min, tm_local.tm_sec, suffix);
    return buffer;
}

static void set_time_of_day(clock_state_t *state)
{
    struct timeval tv;
#ifdef _WIN32
    if (state->ntp_time < 0) {
        tv.tv_sec = 0;
    } else if (state->ntp_time > (time_t)UINT32_MAX) {
        tv.tv_sec = UINT32_MAX;
    } else {
        tv.tv_sec = (uint32_t)state->ntp_time;
    }
    tv.tv_usec = (long)0;
#else
    tv.tv_sec = state->ntp_time;
    tv.tv_usec = 0;
#endif
    settimeofday(&tv, NULL);
}

bool clock_timer_callback(repeating_timer_t *t)
{
    clock_state_t *state = (clock_state_t *)t->user_data;

    watchdog_update();

    // Adjust the NTP drift a second at a time
    if (state->ntp_drift > 0) {
        state->ntp_drift--;
    } else if (state->ntp_drift < 0) {
        state->ntp_drift++;
    }
    time_t now = time(NULL) + state->ntp_drift;
    struct tm current_time;
    gmtime_r(&now, &current_time);

    // Clear any watchdog error icon after it's een displayed a while
    if (now > (state->last_watchdog_error + WATCHDOG_ICON_INTERVAL)) {
        state->watchdog_reset_error = WATCHDOG_OK;
    }

    if (state->first_clock_tick == 0 || current_time.tm_sec == 0) {
        CLOCK_DEBUG("%s, timestamp=%llu, boot_count=%lu, ntp_reset_error=%d, wifi_reset_error=%d, NTP=%s\r\n",
                    time_as_string(now), now, persistent_state.boot_count, state->ntp_reset_error,
                    state->wifi_reset_error, ntp_error_to_string(state->ntp_state->error));

        if (time_is_dst(&current_time)) {
            /* Apply daylight savings */
            if (current_time.tm_hour == 23) {
                current_time.tm_hour = 0;
                current_time.tm_wday = (current_time.tm_wday + 1) % 7;
            } else {
                current_time.tm_hour++;
            }
        }

        static char lcd_digits[NUM_LCDS + 1];
        strncpy(lcd_digits, day_of_week[current_time.tm_wday], 3);
        lcd_digits[3] = '0' + (char)(current_time.tm_hour / 10);
        lcd_digits[4] = '0' + (char)(current_time.tm_hour % 10);
        lcd_digits[5] = '0' + (char)(current_time.tm_min / 10);
        lcd_digits[6] = '0' + (char)(current_time.tm_min % 10);
        lcd_digits[7] = '\0';

        for (unsigned int ii = 0; ii < NUM_LCDS; ii++) {
#ifndef TEST_MODE
            // Skip updates in test mode for performance reasons; all the subsequent
            // stubbed calls to GPIO consume a lot of unnecessary CPU over the tests
            if (state->current_lcd_digits[ii] != lcd_digits[ii]) {
                lcd_clear_screen(state->lcd_states[ii], BLACK);
                lcd_print_clock_digit(state->lcd_states[ii], (ii < 3) ? CYAN : GREEN, lcd_digits[ii]);
            }
            if (ii == 0) {
                lcd_update_icons(state->lcd_states[0], state->watchdog_reset_error, state->ntp_state->error, WIFI_OK);
            }
#endif
            state->current_lcd_digits[ii] = lcd_digits[ii];
        }

        if ((now - state->ntp_last_sync) >= state->ntp_interval) {
            ntp_error_t ntp_error_status = ntp_get_time(state->ntp_state);
            if (state->ntp_state->status == NTP_KOD) {
                state->ntp_interval *= 2;
                CLOCK_DEBUG("NTP: backing off: new delay is %d minutes\r\n", state->ntp_interval);
            } else if (ntp_error_status != NTP_OK) {
                state->ntp_last_sync = now;
                CLOCK_DEBUG("NTP: get time failed with error %d\r\n", ntp_error_status);
                lcd_update_icons(state->lcd_states[0], state->watchdog_reset_error, state->ntp_state->error, WIFI_OK);
            } else {
                int drift = (int)state->ntp_time - (int)now;
                CLOCK_DEBUG("NTP sync at %s; drift = %ds\r\n", time_as_string(state->ntp_time), drift);
                state->ntp_last_sync = state->ntp_time;
                state->ntp_drift = drift;
                set_time_of_day(state);
            }
        }
        state->first_clock_tick = 1;
    }

    return 1; // Keep repeating
}

#ifdef TEST_MODE
// In test mode, main() always returns, even in the case of a fatal error
// as we need to that fatal errors happened for the correct reasons.
int test_main(void)
{
    static int test_main_watchdog_reentry;
    mock_printf("Test init\n");
    test_main_watchdog_reentry = 0;
    watchdog_reboot_called = 0;
    if (setjmp(fatal_jmp_buf)) {
        test_main_watchdog_reentry = 1;
        mock_printf("Reboot: %s/%s\n", ntp_error_to_string(persistent_state.ntp_error),
                    wifi_error_to_string(persistent_state.wifi_error));
    }
#else
int main(void)
{
#endif

    stdio_init_all();

    clock_state_t *state = (clock_state_t *)calloc(1, sizeof(clock_state_t));
    if (state == NULL) {
        // Unrecoverable state and no chance to display status on the LCD
        printf("Failed to allocate clock state\r\n");
        return 1;
    }
    for (unsigned int ii = 0; ii < NUM_LCDS; ii++) {
        // Reset is shared so only need to do this once
        int reset = (ii == 0);
        state->lcd_states[ii] = lcd_init(/* RST  */ LCD_GPIO_RST,
                                         /* DC   */ lcd_pin_config[ii].DC,
                                         /* BL   */ LCD_GPIO_BL,
                                         /* CS   */ lcd_pin_config[ii].CS,
                                         /* CLK  */ LCD_GPIO_CLK,
                                         /* MOSI */ LCD_GPIO_MOSI, reset);
        if (state->lcd_states[ii] == NULL) {
            // Unrecoverable state and no chance to display status on the LCD
            printf("LCD %u: failed to initialise\r\n", ii + 1);
            return 1;
        }
        lcd_clear_screen(state->lcd_states[ii], BLACK);
    }

    int cold_boot = (watchdog_caused_reboot() == 0);
    if (cold_boot) {
        persistent_state.boot_count = 0;
        persistent_state.watchdog_error = WATCHDOG_OK;
        persistent_state.ntp_error = NTP_OK;
        persistent_state.wifi_error = WIFI_OK;
        lcd_print_line(state->lcd_states[0], 2, GREEN, "LCD init successful");
        CLOCK_DEBUG("Cold boot\r\n");
    } else {
        persistent_state.boot_count++;
        CLOCK_DEBUG("Reboot: count=%lu, ntp_error=%d, wifi_error=%d\r\n", persistent_state.boot_count,
                    persistent_state.ntp_error, persistent_state.wifi_error);
        watchdog_error_t watchdog_error;
        if (persistent_state.ntp_error != NTP_OK) {
            watchdog_error = WATCHDOG_NTP;
        } else if (persistent_state.wifi_error != WIFI_OK) {
            watchdog_error = WATCHDOG_WIFI;
        } else {
            watchdog_error = WATCHDOG_RESET;
        }
        lcd_update_icons(state->lcd_states[0], watchdog_error, persistent_state.ntp_error, persistent_state.wifi_error);
        state->watchdog_reset_error = watchdog_error;
        state->ntp_reset_error = persistent_state.ntp_error;
        state->wifi_reset_error = persistent_state.wifi_error;
        persistent_state.ntp_error = NTP_OK;
        persistent_state.wifi_error = WIFI_OK;
    }
#ifdef TEST_MODE
    // Test has generated a watchdog reset and logged its outcome so we can return to the test harness now
    if (test_main_watchdog_reentry) {
        return 1;
    }
#endif
    watchdog_update();

    wifi_error_t wifi_status = connect_to_wifi(WIFI_SSID, WIFI_PASSWORD);
    if (wifi_status == WIFI_OK) {
        if (cold_boot) {
            lcd_print_line(state->lcd_states[0], 3, GREEN, "Connected to WiFi");
        }
        lcd_update_icons(state->lcd_states[0], WATCHDOG_OK, NTP_OK, WIFI_OK);
    } else {
        fatal_reset(state, NTP_OK, wifi_status);
        // Never reached: reset happens
    }

    state->ntp_state = ntp_init((void *)state, ntp_timer_callback);
    if (state->ntp_state == NULL) {
        fatal_reset(state, NTP_INIT_ERROR, WIFI_OK);
        // Never reached: reset happens
    }
    ntp_error_t ntp_status = ntp_get_time(state->ntp_state);
    if (ntp_status == NTP_OK) {
        if (cold_boot) {
            lcd_print_line(state->lcd_states[0], 4, GREEN, "NTP time sync OK");
        }
        lcd_update_icons(state->lcd_states[0], WATCHDOG_OK, NTP_OK, WIFI_OK);
    } else {
        fatal_reset(state, ntp_status, WIFI_OK);
        // Never reached: reset happens
    }

    state->ntp_drift = 0;
    state->ntp_last_sync = state->ntp_time;
    state->ntp_interval = NTP_SYNC_INTERVAL_SEC;
    if (!cold_boot) {
        state->last_watchdog_error = state->ntp_time;
    }

    // Set the system clock to the NTP time
    set_time_of_day(state);

    // Call the timer every second to enable us to slowly change the clock if
    // the system clock drifts from NTP time. Let the watchdog reset the clock
    // if the timer callback has not happened in a few ticks.
    watchdog_enable(WATCHDOG_TIMEOUT_MS, /* pause_on_debug */ 1);
    sleep_ms(500);
    add_repeating_timer_ms(1 * 1000, clock_timer_callback, state, &state->timer);

#ifndef TEST_MODE
    while (1) {
        sleep_ms(1000);
    }
#endif

    return 0;
}
