#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

// Pico SDK
#ifndef TEST_MODE
#include "hardware/adc.h"
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
#endif

// Local includes
#include "bitmap.h"
#include "clock.h"
#include "config.h"

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

static time_t get_atomic_time(clock_state_t *state)
{
    uint32_t interrupts = save_and_disable_interrupts();
    time_t safe_time = state->ntp_time;
    restore_interrupts(interrupts);
    return safe_time;
}

void system_reboot(void)
{
    CLOCK_DEBUG("Rebooting\r\n");
    watchdog_reboot((uint32_t)0, SRAM_END, (uint32_t)0 /* delay_ms */);
#ifndef TEST_MODE
    while (1)
        __wfi(); // hang until reset
#endif
}

// Callback from NTP called when an NTP request is successful
void ntp_timer_callback(void *state, time_t *ntp_time)
{
    clock_state_t *clock_state = (clock_state_t *)state;

    // Store time atomically so main loop doesn't read a torn 64-bit value
    uint32_t ints = save_and_disable_interrupts();
    clock_state->ntp_time = *ntp_time;
    restore_interrupts(ints);

    // Signal the task loop that new NTP data is ready to process
    clock_state->ntp_time_updated = 1;
}

int day_of_week(int day, int month, int year)
{
    if (month < 3) {
        month += 12;
        year -= 1;
    }
    int K = year % 100;
    int J = year / 100;
    int h = (day + (13 * (month + 1)) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;
    return (h + 6) % 7;
}

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

time_t tm_to_epoch(struct tm *tm)
{
#if defined(__APPLE__) || defined(__linux__)
    return timegm(tm);
#else
    return mktime(tm);
#endif
}

static void get_dst_bounds(dst_rule_t rule, int year, time_t *start, time_t *end)
{
    struct tm ts = {0}, te = {0};
    ts.tm_year = year - 1900;
    te.tm_year = year - 1900;

    switch (rule) {
        case DST_RULE_NA:
            ts.tm_mon = 2;
            ts.tm_mday = nth_weekday(2, 0, 3, year);
            ts.tm_hour = 2;
            te.tm_mon = 10;
            te.tm_mday = nth_weekday(1, 0, 11, year);
            te.tm_hour = 2;
            break;
        case DST_RULE_EU:
            ts.tm_mon = 2;
            ts.tm_mday = nth_weekday(-1, 0, 3, year);
            ts.tm_hour = 1;
            te.tm_mon = 9;
            te.tm_mday = nth_weekday(-1, 0, 10, year);
            te.tm_hour = 1;
            break;
        case DST_RULE_AU:
            ts.tm_mon = 9;
            ts.tm_mday = nth_weekday(1, 0, 10, year);
            ts.tm_hour = 2;
            te.tm_mon = 3;
            te.tm_mday = nth_weekday(1, 0, 4, year);
            te.tm_hour = 3;
            break;
        case DST_RULE_NZ:
            ts.tm_mon = 8;
            ts.tm_mday = nth_weekday(-1, 0, 9, year);
            ts.tm_hour = 2;
            te.tm_mon = 3;
            te.tm_mday = nth_weekday(1, 0, 4, year);
            te.tm_hour = 3;
            break;
        case DST_RULE_CL:
            ts.tm_mon = 8;
            ts.tm_mday = nth_weekday(1, 6, 9, year);
            ts.tm_hour = 23;
            ts.tm_min = 59;
            te.tm_mon = 3;
            te.tm_mday = nth_weekday(1, 6, 4, year);
            te.tm_hour = 23;
            te.tm_min = 59;
            break;
        case DST_RULE_IL:
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

int time_is_dst(time_t utc_now, clock_state_t *state)
{
    dst_rule_t dst_rule = state->flash_config.dst_rule;
    if (dst_rule == DST_RULE_NONE)
        return 0;

    struct tm tm_local;
    struct tm *utc_tm = gmtime_r(&utc_now, &tm_local);
    int year = utc_tm->tm_year + 1900;
    time_t start_time, end_time;

    get_dst_bounds(dst_rule, year, &start_time, &end_time);

    if (start_time < end_time) {
        return (utc_now >= start_time && utc_now < end_time);
    } else {
        return (utc_now >= start_time || utc_now < end_time);
    }
}

static time_t calculate_local_time(time_t utc_now, clock_state_t *state)
{
    time_t local_time = utc_now + (state->flash_config.tz_offset_mins * 60);
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

    const char *suffix = time_is_dst(t, state) ? " (DST)" : "";
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d%s", tm_local.tm_hour, tm_local.tm_min, tm_local.tm_sec, suffix);
    return buffer;
}

static void set_time_of_day(clock_state_t *state)
{
    struct timeval tv;
    tv.tv_sec = get_atomic_time(state);
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
}

bool clock_timer_callback(repeating_timer_t *t)
{
    clock_state_t *state = (clock_state_t *)t->user_data;
    static uint8_t button_hold_seconds = 0;
    if (gpio_get(FACTORY_RESET_GPIO) == 0) {
        button_hold_seconds++;
        if (button_hold_seconds >= (FACTORY_RESET_HOLD_TIME_MS / 1000)) {
            button_hold_seconds = 0;
            state->ap_mode_triggered = 1;
        }
    } else {
        button_hold_seconds = 0;
    }

    // Flag the main loop to process UI and Networking
    state->clock_tick_updated = true;
    return true; // Keep repeating
}

bool config_store_handler(struct flash_config_t *config, bool invalidate)
{
    config->magic_marker = invalidate ? 0xffffffff : CONFIG_MAGIC;

    uint8_t flash_buf[FLASH_PAGE_SIZE];
    memset(flash_buf, 0xFF, FLASH_PAGE_SIZE);
    memcpy(flash_buf, config, sizeof(flash_config_t));

    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, flash_buf, FLASH_PAGE_SIZE);
    restore_interrupts(interrupts);

    if (invalidate)
        system_reboot(); // will not return

    CLOCK_DEBUG("Stored new config in flash\r\n");
    return true;
}

static bool backlight_timer_callback(struct repeating_timer *t)
{
    lcd_state_t *lcd_state = (lcd_state_t *)t->user_data;

    adc_select_input(AMBIENT_LIGHT_ADC_CH);
    uint16_t raw_adc = adc_read();

    // Blend the new reading (1/8th) with the old reading (7/8ths)
    if (lcd_state->smoothed_adc == 0) {
        lcd_state->smoothed_adc = raw_adc;
    } else {
        lcd_state->smoothed_adc = ((lcd_state->smoothed_adc * 7) + raw_adc) / 8;
    }

    uint8_t bl_percent;
    if (lcd_state->smoothed_adc <= AMBIENT_LIGHT_DARK) {
        bl_percent = 5; // Minimum backlight level
    } else if (lcd_state->smoothed_adc >= AMBIENT_LIGHT_BRIGHT) {
        bl_percent = 100; // Maximum brightness
    } else {
        // Normalize to between 0.0 and 1.0
        float normalized_input =
            (float)(lcd_state->smoothed_adc - AMBIENT_LIGHT_DARK) / (float)(AMBIENT_LIGHT_BRIGHT - AMBIENT_LIGHT_DARK);
        // Apply perception correction
        bl_percent = (uint8_t)(sqrtf(normalized_input) * 100.0f);
    }
    lcd_set_backlight(lcd_state, bl_percent);

    return true; // Keep repeating
}

void clock_init(clock_state_t *state)
{
    stdio_init_all();

    gpio_init(FACTORY_RESET_GPIO);
    gpio_set_dir(FACTORY_RESET_GPIO, GPIO_IN);
    gpio_pull_up(FACTORY_RESET_GPIO);

    adc_init();
    adc_gpio_init(AMBIENT_LIGHT_GPIO);

    memset(state, 0, sizeof(clock_state_t));
    for (unsigned int ii = 0; ii < NUM_LCDS; ii++) {
        lcd_init(&state->lcd_states[ii], (uint16_t)ii, /* reset */ ii == 0);
        lcd_clear_screen(&state->lcd_states[ii], BLACK);
    }

    int factory_reset = 1;
    for (int ii = 0; ii < (FACTORY_RESET_HOLD_TIME_MS / 100); ii++) {
        if (gpio_get(FACTORY_RESET_GPIO) != 0) {
            factory_reset = 0;
            break;
        }
        sleep_ms(100);
    }
    state->cold_boot = (watchdog_caused_reboot() == 0);

    if (factory_reset) {
        CLOCK_DEBUG("Erasing flash configuration.\r\n");
        flash_config_t config = {0};
        config_store_handler(&config, 1 /* invalidate */);
    } else if (state->cold_boot) {
        memset(&persistent_state, 0, sizeof(persistent_state_t));
        persistent_state.magic_marker = CONFIG_MAGIC;
        lcd_print_line(&state->lcd_states[0], GREEN, LCD_MSG_INIT_OK);
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
        lcd_update_icons(&state->lcd_states[0], watchdog_error, persistent_state.ntp_error,
                         persistent_state.wifi_error);
        state->watchdog_reset_error = watchdog_error;
        state->ntp_reset_error = persistent_state.ntp_error;
        state->wifi_reset_error = persistent_state.wifi_error;
        persistent_state.ntp_error = NTP_OK;
        persistent_state.wifi_error = WIFI_OK;
    }

    CLOCK_DEBUG("Checking flash\r\n");
    flash_config_t *flash_config = (flash_config_t *)(FLASH_CONFIG_ADDR);
    if (flash_config->magic_marker == CONFIG_MAGIC) {
        CLOCK_DEBUG("Valid config loaded from Flash\r\n");
        memcpy(&state->flash_config, flash_config, sizeof(flash_config_t));
    } else {
        CLOCK_DEBUG("Can't find valid config in Flash. Starting access point.\r\n");
        lcd_print_line(&state->lcd_states[0], RED, LCD_MSG_FLASH_ERROR);
        lcd_print_line(&state->lcd_states[0], RED, LCD_MSG_WIFI_ERROR);
        start_wifi_access_point(NULL, config_store_handler, &state->wifi_initialized);
        system_reboot();
    }
    CLOCK_DEBUG("Checking flash done\r\n");

    state->ntp_last_sync = state->ntp_time;
    state->ntp_interval = NTP_SYNC_INTERVAL_SEC;
    state->first_clock_tick = true;
}

void clock_start(clock_state_t *state)
{
    wifi_error_t wifi_status =
        connect_to_wifi(state->flash_config.wifi_ssid, state->flash_config.wifi_password, &state->wifi_initialized);
    if (wifi_status == WIFI_OK) {
        if (state->cold_boot) {
            lcd_print_line(&state->lcd_states[0], GREEN, LCD_MSG_WIFI_OK);
        }
        lcd_update_icons(&state->lcd_states[0], state->watchdog_reset_error, NTP_OK, WIFI_OK);
    } else {
        fatal_reset(state, NTP_OK, wifi_status);
    }

    if (!ntp_init(&state->ntp_state, (void *)state, ntp_timer_callback))
        fatal_reset(state, NTP_INIT_ERROR, WIFI_OK);
    state->ntp_state.ntp_port = state->flash_config.ntp_port;

    ntp_error_t ntp_status = ntp_request_async(&state->ntp_state);
    if (ntp_status != NTP_OK)
        fatal_reset(state, ntp_status, WIFI_OK);

    watchdog_enable(WATCHDOG_TIMEOUT_MS, /* pause_on_debug */ 1);
    sleep_ms(500);
    if (!add_repeating_timer_ms(1000, clock_timer_callback, state, &state->led_timer_state)) {
        CLOCK_DEBUG("Failed to init clock timer callback\r\n");
        fatal_reset(state, NTP_INIT_ERROR, WIFI_OK);
    }
    if (!add_repeating_timer_ms(BACKLIGHT_CALLBACK_TIME_MS, backlight_timer_callback, &state->lcd_states[0],
                                &state->backlight_timer_state)) {
        CLOCK_DEBUG("Failed to init backlight timer callback\r\n");
        fatal_reset(state, NTP_INIT_ERROR, WIFI_OK);
    }
}

void clock_task(clock_state_t *state)
{
    // Process async NTP responses
    if (state->ntp_time_updated) {
        state->ntp_time_updated = 0;
        set_time_of_day(state);
        state->ntp_last_sync = time(NULL);
        CLOCK_DEBUG("NTP sync complete\r\n");
    }

    if (!state->clock_tick_updated) {
        return;
    }
    state->clock_tick_updated = false;

    watchdog_update();

    time_t utc_now = time(NULL);
    time_t local_epoch = calculate_local_time(utc_now, state);
    struct tm local_time;
    gmtime_r(&local_epoch, &local_time);

    // Clear any watchdog error icon after it's been displayed a while
    if (local_epoch > (state->last_watchdog_error_time + WATCHDOG_ICON_INTERVAL)) {
        state->watchdog_reset_error = WATCHDOG_OK;
    }

    char lcd_digits[NUM_LCDS + 1];
    static char day_of_week_str[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    strncpy(lcd_digits, day_of_week_str[local_time.tm_wday], 3);
    lcd_digits[3] = '0' + (char)(local_time.tm_hour / 10);
    lcd_digits[4] = '0' + (char)(local_time.tm_hour % 10);
    lcd_digits[5] = '0' + (char)(local_time.tm_min / 10);
    lcd_digits[6] = '0' + (char)(local_time.tm_min % 10);
    lcd_digits[7] = '\0';

    bool digits_changed = false;
    for (unsigned int ii = 0; ii < NUM_LCDS; ii++) {
        if (state->current_lcd_digits[ii] != lcd_digits[ii] || state->first_clock_tick) {
            digits_changed = true;
            break;
        }
    }

    if (digits_changed) {
        CLOCK_DEBUG("%s, timestamp=%llu, boot_count=%lu, ntp_reset_error=%d, wifi_reset_error=%d, NTP=%s\r\n",
                    time_as_string(utc_now, state), local_epoch, persistent_state.boot_count, state->ntp_reset_error,
                    state->wifi_reset_error, ntp_error_to_string(state->ntp_state.error));

        for (unsigned int ii = 0; ii < NUM_LCDS; ii++) {
            if (state->current_lcd_digits[ii] != lcd_digits[ii] || state->first_clock_tick) {
                lcd_clear_screen(&state->lcd_states[ii], BLACK);
                lcd_print_clock_digit(&state->lcd_states[ii], (ii < 3) ? CYAN : GREEN, lcd_digits[ii]);
            }
            if (ii == 0) {
                lcd_update_icons(&state->lcd_states[0], state->watchdog_reset_error, state->ntp_state.error, WIFI_OK);
            }
            state->current_lcd_digits[ii] = lcd_digits[ii];
        }
    }

    if (state->ntp_state.status == NTP_PENDING) {
        uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        if ((now_ms - state->ntp_state.request_start_ms) > state->flash_config.ntp_timeout) {
            CLOCK_DEBUG("NTP Async Timeout!\r\n");
            fatal_reset(state, NTP_TIMEOUT_ERROR, WIFI_OK);
        }
    } else if (state->ntp_state.status == NTP_FAILED) {
        CLOCK_DEBUG("NTP Async Failed with error %s\r\n", ntp_error_to_string(state->ntp_state.error));
        fatal_reset(state, state->ntp_state.error, WIFI_OK);
    } else if (state->ntp_state.status == NTP_KOD) {
        state->ntp_interval *= 2;
        state->ntp_state.status = NTP_IDLE;
        CLOCK_DEBUG("NTP: KoD received, doubling interval to %d seconds\r\n", state->ntp_interval);
    } else if (state->ntp_state.status == NTP_SUCCESS) {
        state->ntp_state.status = NTP_IDLE;
    }

    if (state->ntp_state.status == NTP_IDLE) {
        if ((utc_now - state->ntp_last_sync) >= state->ntp_interval) {
            CLOCK_DEBUG("NTP starting new check\r\n");
            state->ntp_last_sync = utc_now;

            ntp_error_t ntp_status = ntp_request_async(&state->ntp_state);
            if (ntp_status != NTP_OK)
                fatal_reset(state, ntp_status, WIFI_OK);
        }
    }
    state->first_clock_tick = false;
}
