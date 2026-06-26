#ifndef WIFI_SSID
#error "WIFI_SSID is not defined. Please define it in config.cmake or via a compile flag."
#endif

#ifndef WIFI_PASSWORD
#error "WIFI_PASSWORD is not defined. Please define it in config.cmake or via a compile flag."
#endif

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

// Pico SDK
#ifndef TEST_MODE
#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/util/datetime.h"
#else
#include "mock.h"
#include "test.h"

extern int test_main(void);
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

volatile uint trigger_ap_mode = 0;

// Callback from NTP called when an NTP request is successful
void ntp_timer_callback(void *state, time_t *ntp_time)
{
    clock_state_t *clock_state = (clock_state_t *)state;
    clock_state->ntp_time = *ntp_time;
}

int day_of_week(int day, int month, int year)
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
    return (h + 6) % 7;
}

// target_wday: 0=Sunday, 1=Monday, ..., 6=Saturday
// n: 1 for 1st, 2 for 2nd... -1 for LAST
static int nth_weekday(int n, int target_wday, int month, int year)
{
    int dow_first = day_of_week(1, month, year);
    int first_occurrence = 1 + (target_wday - dow_first + 7) % 7;

    if (n > 0) {
        return first_occurrence + (n - 1) * 7;
    } else {
        int last = first_occurrence + 28;
        int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
            days_in_month[2] = 29;
        }
        if (last > days_in_month[month]) {
            last -= 7;
        }
        return last;
    }
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

// Generates the Unix Epoch bounds for DST based on the current year
static void get_dst_bounds(dst_rule_t rule, int year, time_t *start, time_t *end)
{
    struct tm ts = {0}, te = {0};
    ts.tm_year = year - 1900;
    te.tm_year = year - 1900;

    switch (rule) {
        case DST_RULE_NA: // 2nd Sun Mar (02:00) to 1st Sun Nov (02:00)
            ts.tm_mon = 2;
            ts.tm_mday = nth_weekday(2, 0, 3, year);
            ts.tm_hour = 2;
            te.tm_mon = 10;
            te.tm_mday = nth_weekday(1, 0, 11, year);
            te.tm_hour = 2;
            break;

        case DST_RULE_EU: // Last Sun Mar (01:00) to Last Sun Oct (01:00)
            ts.tm_mon = 2;
            ts.tm_mday = nth_weekday(-1, 0, 3, year);
            ts.tm_hour = 1;
            te.tm_mon = 9;
            te.tm_mday = nth_weekday(-1, 0, 10, year);
            te.tm_hour = 1;
            break;

        case DST_RULE_AU: // 1st Sun Oct (02:00) to 1st Sun Apr (03:00)
            ts.tm_mon = 9;
            ts.tm_mday = nth_weekday(1, 0, 10, year);
            ts.tm_hour = 2;
            te.tm_mon = 3;
            te.tm_mday = nth_weekday(1, 0, 4, year);
            te.tm_hour = 3;
            break;

        case DST_RULE_NZ: // Last Sun Sep (02:00) to 1st Sun Apr (03:00)
            ts.tm_mon = 8;
            ts.tm_mday = nth_weekday(-1, 0, 9, year);
            ts.tm_hour = 2;
            te.tm_mon = 3;
            te.tm_mday = nth_weekday(1, 0, 4, year);
            te.tm_hour = 3;
            break;

        case DST_RULE_CL: // 1st Sat Sep (24:00) to 1st Sat Apr (24:00)
            ts.tm_mon = 8;
            ts.tm_mday = nth_weekday(1, 6, 9, year);
            ts.tm_hour = 23;
            ts.tm_min = 59;
            te.tm_mon = 3;
            te.tm_mday = nth_weekday(1, 6, 4, year);
            te.tm_hour = 23;
            te.tm_min = 59;
            break;

        case DST_RULE_IL: // Fri before last Sun Mar to Last Sun Oct (02:00)
            ts.tm_mon = 2;
            ts.tm_mday = nth_weekday(-1, 0, 3, year) - 2;
            ts.tm_hour = 2;
            te.tm_mon = 9;
            te.tm_mday = nth_weekday(-1, 0, 10, year);
            te.tm_hour = 2;
            break;

        case DST_RULE_NONE:
            *start = 0;
            *end = 0;
            return;
    }

    *start = tm_to_epoch(&ts);
    *end = tm_to_epoch(&te);
}

// Determines if a given UTC time requires a 1-hour shift
int time_is_dst(time_t utc_now, clock_state_t *state)
{
    dst_rule_t dst_rule = state->clock_config.dst_rule;
    if (dst_rule == DST_RULE_NONE)
        return 0;

    struct tm *utc_tm = gmtime(&utc_now);
    int year = utc_tm->tm_year + 1900;

    time_t start_time, end_time;
    get_dst_bounds(dst_rule, year, &start_time, &end_time);

    if (start_time < end_time) {
        return (utc_now >= start_time && utc_now < end_time); // Northern
    } else {
        return (utc_now >= start_time || utc_now < end_time); // Southern Wrap
    }
}

static time_t calculate_local_time(time_t utc_now, clock_state_t *state)
{
    time_t local_time = utc_now + (state->clock_config.tz_offset_mins * 60);

    if (time_is_dst(utc_now, state)) {
        local_time += 3600;
    }

    return local_time;
}

const char *time_as_string(time_t t, clock_state_t *state)
{
    static char buffer[32];
    struct tm tm_local;

    time_t local_epoch = calculate_local_time(t, state);
    gmtime_r(&local_epoch, &tm_local);

    const char *suffix = "";
    if (time_is_dst(t, state)) {
        suffix = " (DST)";
    }
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d%s", tm_local.tm_hour, tm_local.tm_min, tm_local.tm_sec, suffix);

    return buffer;
}

static void set_time_of_day(clock_state_t *state)
{
    struct timeval tv;
    tv.tv_sec = state->ntp_time;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
}

bool clock_timer_callback(repeating_timer_t *t)
{
    clock_state_t *state = (clock_state_t *)t->user_data;

    watchdog_update();

    // uint32_t interrupts = save_and_disable_interrupts();
    // Step clock if drift is huge, otherwise slew a second at a time
    if (state->ntp_drift > 60 || state->ntp_drift < -60) {

        state->ntp_drift = 0;
    } else if (state->ntp_drift > 0) {
        state->ntp_drift--;
    } else if (state->ntp_drift < 0) {
        state->ntp_drift++;
    }

    time_t local_epoch = calculate_local_time(time(NULL), state) + state->ntp_drift;
    struct tm local_time;
    gmtime_r(&local_epoch, &local_time);

    // Clear any watchdog error icon after it's been displayed a while
    if (local_epoch > (state->last_watchdog_error + WATCHDOG_ICON_INTERVAL)) {
        state->watchdog_reset_error = WATCHDOG_OK;
    }

    static uint8_t button_hold_seconds = 0;
    // Check the physical state of the button (0 means pressed due to pull-up)
    if (gpio_get(CONFIG_BUTTON_GPIO) == 0) {
        button_hold_seconds++;
        CLOCK_DEBUG("Configuration mode selected %d second(s)\n", button_hold_seconds);

        if (button_hold_seconds >= CONFIG_BUTTON_HOLD_SECONDS) {
            button_hold_seconds = 0;
            CLOCK_DEBUG("Configuration mode triggered!\n");
            trigger_ap_mode = 1;
        }
    } else {
        button_hold_seconds = 0;
    }

    if (state->first_clock_tick == 0 || local_time.tm_sec == 0) {
        CLOCK_DEBUG("%s, timestamp=%llu, boot_count=%lu, ntp_reset_error=%d, wifi_reset_error=%d, NTP=%s\r\n",
                    time_as_string(local_epoch, state), local_epoch, persistent_state.boot_count,
                    state->ntp_reset_error, state->wifi_reset_error, ntp_error_to_string(state->ntp_state->error));

        static char lcd_digits[NUM_LCDS + 1];
        static char day_of_week_str[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        strncpy(lcd_digits, day_of_week_str[local_time.tm_wday], 3);

        lcd_digits[3] = '0' + (char)(local_time.tm_hour / 10);
        lcd_digits[4] = '0' + (char)(local_time.tm_hour % 10);
        lcd_digits[5] = '0' + (char)(local_time.tm_min / 10);
        lcd_digits[6] = '0' + (char)(local_time.tm_min % 10);
        lcd_digits[7] = '\0';

        for (unsigned int ii = 0; ii < NUM_LCDS; ii++) {
            if (state->current_lcd_digits[ii] != lcd_digits[ii]) {
                lcd_clear_screen(state->lcd_states[ii], BLACK);
                lcd_print_clock_digit(state->lcd_states[ii], (ii < 3) ? CYAN : GREEN, lcd_digits[ii]);
            }
            if (ii == 0) {
                lcd_update_icons(state->lcd_states[0], state->watchdog_reset_error, state->ntp_state->error, WIFI_OK);
            }
            state->current_lcd_digits[ii] = lcd_digits[ii];
        }

        if ((local_epoch - state->ntp_last_sync) >= state->ntp_interval) {
            ntp_error_t ntp_error_status = ntp_get_time(state->ntp_state);
            if (state->ntp_state->status == NTP_KOD) {
                state->ntp_interval *= 2;
                CLOCK_DEBUG("NTP: backing off: new delay is %d minutes\r\n", state->ntp_interval);
            } else if (ntp_error_status != NTP_OK) {
                state->ntp_last_sync = local_epoch;
                CLOCK_DEBUG("NTP: get time failed with error %d\r\n", ntp_error_status);
                lcd_update_icons(state->lcd_states[0], state->watchdog_reset_error, state->ntp_state->error, WIFI_OK);
            } else {
                int drift = (int)state->ntp_time - (int)local_epoch;
                CLOCK_DEBUG("NTP sync at %s; drift = %ds\r\n", time_as_string(state->ntp_time, state), drift);
                state->ntp_last_sync = state->ntp_time;
                state->ntp_drift = drift;
                set_time_of_day(state);
            }
        }
        state->first_clock_tick = 1;
    }

    return 1; // Keep repeating
}

int config_store_handler(clock_config_t *config)
{
    config->magic_marker = CONFIG_MAGIC;
    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, (uint8_t *)config, sizeof(clock_config_t));
    restore_interrupts(interrupts);

    CLOCK_DEBUG("Stored new config in flash\r\n");
    return 0;
}

// static time_t get_atomic_time(clock_state_t *state)
// {
//     uint32_t interrupts = save_and_disable_interrupts();
//     time_t safe_time = state->ntp_time;
//     restore_interrupts(interrupts);
//     return safe_time;
// }

clock_state_t *clock_init(void)
{
    stdio_init_all();

    gpio_init(CONFIG_BUTTON_GPIO);
    gpio_set_dir(CONFIG_BUTTON_GPIO, GPIO_IN);
    gpio_pull_up(CONFIG_BUTTON_GPIO);

    clock_state_t *state = (clock_state_t *)calloc(1, sizeof(clock_state_t));
    if (state == NULL) {
        // Unrecoverable state and no chance to display status on the LCD
        on_clock_alloc_failed();
        printf("Failed to allocate clock state\r\n");
        return (clock_state_t *)0;
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
            on_lcd_init_failed(state, ii);
            return (clock_state_t *)0;
        }
        lcd_clear_screen(state->lcd_states[ii], BLACK);
    }

    state->cold_boot = (watchdog_caused_reboot() == 0);
    if (state->cold_boot) {
        memset(&persistent_state, 0, sizeof(persistent_state_t));
        persistent_state.magic_marker = CONFIG_MAGIC;
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

    CLOCK_DEBUG("Checking flash\r\n");
    clock_config_t *flash_config = (clock_config_t *)FLASH_CONFIG_ADDR;
    if (flash_config->magic_marker == CONFIG_MAGIC) {
        CLOCK_DEBUG("Valid config loaded from Flash\r\n");
        memcpy(&state->clock_config, flash_config, sizeof(clock_config_t));
    } else {
        CLOCK_DEBUG("Can't find valid config in Flash. Starting access point.\r\n");
        lcd_print_line(state->lcd_states[0], 3, RED, "Flash config corrupt");
        lcd_print_line(state->lcd_states[0], 4, RED, "Connect to Clock Wi-Fi");
        start_wifi_access_point(config_store_handler);
    }
    CLOCK_DEBUG("Checking flash done\r\n");

    return state;
}

int clock_main_loop(clock_state_t *state)
{
    watchdog_update();

    wifi_is_initialized = 0;
    wifi_error_t wifi_status = connect_to_wifi(WIFI_SSID, WIFI_PASSWORD);
    if (wifi_status == WIFI_OK) {
        if (state->cold_boot) {
            lcd_print_line(state->lcd_states[0], 3, GREEN, "Connected to WiFi");
        }
        lcd_update_icons(state->lcd_states[0], state->watchdog_reset_error, NTP_OK, WIFI_OK);
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
        if (state->cold_boot) {
            lcd_print_line(state->lcd_states[0], 4, GREEN, "NTP time sync OK");
        }
        lcd_update_icons(state->lcd_states[0], state->watchdog_reset_error, NTP_OK, WIFI_OK);
    } else {
        fatal_reset(state, ntp_status, WIFI_OK);
        // Never reached: reset happens
    }

    state->ntp_drift = 0;
    state->ntp_last_sync = state->ntp_time;
    state->ntp_interval = NTP_SYNC_INTERVAL_SEC;
    if (!state->cold_boot) {
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

    return 0;
}
