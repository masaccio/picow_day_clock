#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "clock.h"
#include "config.h"
#include "test.h"

// These system calls have been redefined in mock.h so undef them here
// so that we can call into the system libraries as needed.
#undef printf
#undef calloc
#undef time
#undef localtime
#undef settimeofday

typedef int (*test_func_t)(void);

unsigned int log_buffer_size = 0;
unsigned int calloc_fail_at = 0;
unsigned int calloc_counter = 0;
unsigned int pbuf_alloc_fail_at = 0;
unsigned int test_verbose = 0;
int watchdog_reboot_called = 0;
char **log_buffer;

#define LOG_ERROR_WIDTH 45

static int run_test(test_func_t func, const char *test_name, const char **expected_log)
{
    log_buffer = (char **)calloc(sizeof(char *), LOG_BUFFER_SIZE);

    // Always init the test config
    memset(&test_config, 0, sizeof(test_config_t));
    log_buffer_size = 0;

    if (test_verbose) {
        printf("DEBUG: Starting test '%s'\n", test_name);
    }

    // Only happens once on the target
    // TODO: this really should be state in the Wi-Fi code
    wifi_is_initialized = 0;

    // Run test
    int status = func();

    if (status == 1) {
        printf("TEST %s: FAIL\n", test_name);
    } else {
        printf("TEST %s: OK\n", test_name);
    }

    int log_mismatch = 0;
    uint32_t expected_log_size = 0;
    for (unsigned int ii = 0; expected_log[ii] != NULL; ii++) {
        if (ii >= expected_log_size) {
            expected_log_size = ii + 1;
        }
    }
    if (expected_log[0] == NULL && log_buffer_size > 0) {
        log_mismatch = 1;
    }
    for (unsigned int ii = 0; expected_log[ii] != NULL; ii++) {
        if (ii >= log_buffer_size) {
            log_mismatch = 1;
            break;
        }
        size_t test_len = strlen(log_buffer[ii]);
        size_t ref_len = strlen(expected_log[ii]);
        size_t max_len = (ref_len > test_len) ? test_len : ref_len;
        if (test_len != ref_len || strncmp(log_buffer[ii], expected_log[ii], max_len) != 0) {
            log_mismatch = 1;
        }
    }

    if (log_mismatch) {
        uint32_t heading_width = ((LOG_ERROR_WIDTH * 2) - (uint32_t)strlen(test_name) - 12) / 4;
        printf("%.*s [REF] %.*s %s %.*s [TEST] %.*s\n", heading_width, "================", heading_width,
               "================", test_name, heading_width, "================", heading_width, "================");
        for (unsigned int ii = 0; ii < expected_log_size; ii++) {
            if (log_buffer_size > ii) {
                size_t test_len = strlen(log_buffer[ii]);
                size_t ref_len = strlen(expected_log[ii]);
                size_t max_len = (ref_len > test_len) ? test_len : ref_len;
                int match = strncmp(log_buffer[ii], expected_log[ii], max_len) == 0 && test_len == max_len;
                printf("  %-*s %c %s\n", LOG_ERROR_WIDTH, expected_log[ii], match ? '|' : 'x', log_buffer[ii]);
            } else {
                printf("  %-*s x \n", LOG_ERROR_WIDTH, expected_log[ii]);
            }
        }
        for (unsigned int ii = expected_log_size; ii < log_buffer_size; ii++) {
            printf("  %-*s | %s\n", LOG_ERROR_WIDTH, "", log_buffer[ii]);
        }
    }
    for (unsigned int ii = 0; ii <= log_buffer_size; ii++) {
        free(log_buffer[ii]);
    }
    free(log_buffer);
    return status;
}

test_config_t test_config = {
    .cyw43_auth_error_count = 0,
    .cyw43_arch_wifi_connect_status = 0,
    .cyw43_auth_timeout_count = 0,
    .cyw43_arch_init_fail = 0,
    .udp_new_ip_type_fail = 0,
    .udp_sendto_fail = 0,
    .dns_lookup_delay = 0,
    .dns_lookup_fail = 0,
    .watchdog_caused_reboot = 0,
};

static int test_bad_lcd1(void)
{
    calloc_counter = 0;
    calloc_fail_at = 1;
    if (test_main() != 1) {
        return 1;
    }
    calloc_counter = 0;
    calloc_fail_at = 2;
    if (test_main() != 1) {
        return 1;
    }
    calloc_fail_at = 0;
    return 0;
}

static int test_dns_lookups(void)
{
    test_config.dns_bad_arg = 1;
    if (test_main() != 1) {
        return 1;
    }
    test_config.dns_lookup_fail = 1;
    if (test_main() != 1) {
        return 1;
    }
    // DNS poll loops every 500ms for 10s
    test_config.dns_lookup_delay = 21;
    test_config.dns_lookup_fail = 0;
    if (test_main() != 1) {
        return 1;
    }
    // DNS poll loops every 500ms for 10s
    test_config.dns_lookup_delay = 20;
    if (test_main() != 0) {
        return 1;
    }
    return 0;
}

static int test_wifi_init_errors(void)
{
    test_config.cyw43_arch_init_fail = 1;
    if (test_main() != 1) {
        return 1;
    }
    test_config.cyw43_arch_init_fail = 0;
    test_config.cyw43_arch_wifi_connect_status = PICO_ERROR_CONNECT_FAILED;
    if (test_main() != 1) {
        return 1;
    }
    test_config.cyw43_arch_wifi_connect_status = -99;
    if (test_main() != 1) {
        return 1;
    }
    // Try two batches of timeouts: the first one should not quite timeout enough
    test_config.cyw43_arch_wifi_connect_status = 0;
    test_config.cyw43_auth_timeout_count = 4;
    if (test_main() != 0) {
        return 1;
    }
    test_config.cyw43_arch_wifi_connect_status = 0;
    test_config.cyw43_auth_timeout_count = 6;
    if (test_main() != 1) {
        return 1;
    }
    return 0;
}

static int test_wifi_auth_errors(void)
{
    test_config.cyw43_auth_error_count = WIFI_BAD_AUTH_RETRY_COUNT - 1;
    if (test_main() != 0) {
        return 1;
    }

    test_config.cyw43_auth_error_count = WIFI_BAD_AUTH_RETRY_COUNT;
    if (test_main() != 1) {
        return 1;
    }
    return 0;
}

static void set_localtime(clock_state_t *clock_state, int year, int mon, int mday, int hour, int min, int sec)
{

    struct tm tm_val = {0};
    tm_val.tm_year = year - 1900;
    tm_val.tm_mon = mon;
    tm_val.tm_mday = mday;
    tm_val.tm_hour = hour;
    tm_val.tm_min = min;
    tm_val.tm_sec = sec;
    tm_val.tm_isdst = 0;
    time_t t = tm_to_epoch(&tm_val);

    clock_state->ntp_drift = 0;
    clock_state->ntp_last_sync = t;
    clock_state->first_clock_tick = 0;
    mock_system_time_ms = (unsigned long long)t * 1000;
    mock_ntp_seconds = (unsigned long long)t;
}

static clock_state_t *create_test_clock_state(repeating_timer_t *timer)
{
    clock_state_t *clock_state = (clock_state_t *)calloc(1, sizeof(clock_state_t));
    for (unsigned int ii = 0; ii < NUM_LCDS; ii++) {
        clock_state->lcd_states[ii] = lcd_init(0, 0, 0, 0, 0, 0, 0);
    }
    clock_state->ntp_state = ntp_init((void *)clock_state, ntp_timer_callback);
    clock_state->ntp_last_sync = mock_time(NULL);
    clock_state->first_clock_tick = 0;
    timer->user_data = clock_state;
    return clock_state;
}

// Note: 'mo' is 0-indexed (2 = March, 9 = October, etc.)
#define TEST_DST_BOUND(yr, mo, dy, hr, mn, rule, expected)                                                             \
    do {                                                                                                               \
        clock_state->clock_config.dst_rule = rule;                                                                     \
        tm_val.tm_year = (yr) - 1900;                                                                                  \
        tm_val.tm_mon = (mo);                                                                                          \
        tm_val.tm_mday = (dy);                                                                                         \
        tm_val.tm_hour = (hr);                                                                                         \
        tm_val.tm_min = (mn);                                                                                          \
        tm_val.tm_sec = 0;                                                                                             \
        t = tm_to_epoch(&tm_val);                                                                                      \
        if (time_is_dst(t, clock_state) != (expected)) {                                                               \
            printf("DST Test Failed: %04d-%02d-%02d %02d:%02d\n", yr, mo, dy, hr, mn);                                 \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

static int test_dst(void)
{
    repeating_timer_t *timer = (repeating_timer_t *)calloc(1, sizeof(repeating_timer_t));
    clock_state_t *clock_state = create_test_clock_state(timer);
    time_t now;

    // Coverage test for Zeller's congruence
    if (last_weekday_of_month(28, 2, 2025) != /* Friday */ 5) {
        return 1;
    }

    // Timezone Offset Tests (DST_RULE_NONE)`
    clock_state->ntp_last_sync = mock_time(NULL);
    clock_state->ntp_interval = NTP_SYNC_INTERVAL_SEC;
    clock_state->clock_config.dst_rule = DST_RULE_NONE;

    // Test Positive Fractional Offset: India (+5:30 -> 330 mins)
    clock_state->clock_config.tz_offset_mins = 330;
    set_localtime(clock_state, 2024, 0, 1, 12, 0, 0); // Jan 1, 12:00 UTC
    (void)clock_timer_callback(timer);
    now = mock_time(NULL);
    if (strncmp(time_as_string(now, clock_state), "17:30:00", 8) != 0)
        return 1;

    // Test Negative Offset: New York (-5:00 -> -300 mins)
    clock_state->clock_config.tz_offset_mins = -300;
    set_localtime(clock_state, 2024, 0, 1, 12, 0, 0); // Jan 1, 12:00 UTC
    (void)clock_timer_callback(timer);
    now = mock_time(NULL);
    if (strncmp(time_as_string(now, clock_state), "07:00:00", 8) != 0)
        return 1;

    // Display Rollover Tests
    clock_state->clock_config.tz_offset_mins = 0; // Reset to UK base
    clock_state->clock_config.dst_rule = DST_RULE_EU;

    // Sun March 25, 2001 at 00:22 (just before EU clocks change)
    set_localtime(clock_state, 2001, 2, 25, 0, 22, 0);
    (void)clock_timer_callback(timer);
    if (strncmp(clock_state->current_lcd_digits, "Sun0022", 7) != 0)
        return 1;
    now = mock_time(NULL);
    if (strncmp(time_as_string(now, clock_state), "00:22:00", 14) != 0)
        return 1;

    // Sun March 25, 2001 at 01:22 (just after EU clocks change)
    set_localtime(clock_state, 2001, 2, 25, 1, 22, 0);
    (void)clock_timer_callback(timer);
    if (strncmp(clock_state->current_lcd_digits, "Sun0222", 7) != 0)
        return 1;
    now = mock_time(NULL);
    if (strncmp(time_as_string(now, clock_state), "02:22:00 (DST)", 14) != 0)
        return 1;

    // Thu August 23, 2001 at 23:55 (test day rollover in DST)
    set_localtime(clock_state, 2001, 7, 23, 23, 55, 0);
    mock_ntp_seconds = (mock_system_time_ms / 1000) + NTP_DELTA;
    (void)clock_timer_callback(timer);
    if (strncmp(clock_state->current_lcd_digits, "Fri0055", 7) != 0)
        return 1;
    now = mock_time(NULL);
    if (strncmp(time_as_string(now, clock_state), "00:55:00 (DST)", 14) != 0)
        return 1;

    // Exhaustive DST Boundary Math Tests
    struct tm tm_val = {0};
    time_t t;

    // --- Northern Hemisphere ---

    // NA: 2nd Sun Mar (02:00) to 1st Sun Nov (02:00) -> 2024: Mar 10, Nov 3
    TEST_DST_BOUND(2024, 2, 10, 1, 59, DST_RULE_NA, 0); // Before spring forward
    TEST_DST_BOUND(2024, 2, 10, 2, 0, DST_RULE_NA, 1);  // Spring forward triggers
    TEST_DST_BOUND(2024, 10, 3, 1, 59, DST_RULE_NA, 1); // Before fall back
    TEST_DST_BOUND(2024, 10, 3, 2, 0, DST_RULE_NA, 0);  // Fall back triggers

    // EU: Last Sun Mar (01:00) to Last Sun Oct (01:00) -> 2024: Mar 31, Oct 27
    TEST_DST_BOUND(2024, 2, 31, 0, 59, DST_RULE_EU, 0);
    TEST_DST_BOUND(2024, 2, 31, 1, 0, DST_RULE_EU, 1);
    TEST_DST_BOUND(2024, 9, 27, 0, 59, DST_RULE_EU, 1);
    TEST_DST_BOUND(2024, 9, 27, 1, 0, DST_RULE_EU, 0);

    // --- Southern Hemisphere (Wraps across New Year) ---

    // AU: 1st Sun Oct (02:00) to 1st Sun Apr (03:00) -> 2024: Oct 6, Apr 7
    TEST_DST_BOUND(2024, 9, 6, 1, 59, DST_RULE_AU, 0);   // Spring: Before Oct start
    TEST_DST_BOUND(2024, 9, 6, 2, 0, DST_RULE_AU, 1);    // Spring: DST starts
    TEST_DST_BOUND(2024, 11, 25, 12, 0, DST_RULE_AU, 1); // Peak Summer: Dec is DST!
    TEST_DST_BOUND(2024, 3, 7, 2, 59, DST_RULE_AU, 1);   // Autumn: Before Apr end
    TEST_DST_BOUND(2024, 3, 7, 3, 0, DST_RULE_AU, 0);    // Autumn: DST ends (Winter begins)
    TEST_DST_BOUND(2024, 6, 15, 12, 0, DST_RULE_AU, 0);  // Deep Winter: July is Standard Time

    // --- Complex Edge Cases ---

    // Chile: 1st Sat Sep (24:00) to 1st Sat Apr (24:00) -> 2024: Sep 7, Apr 6
    // Note: 24:00 is mathematically 23:59:59 transition
    TEST_DST_BOUND(2024, 8, 7, 23, 58, DST_RULE_CL, 0);
    TEST_DST_BOUND(2024, 8, 7, 23, 59, DST_RULE_CL, 1);
    TEST_DST_BOUND(2024, 3, 6, 23, 58, DST_RULE_CL, 1);
    TEST_DST_BOUND(2024, 3, 6, 23, 59, DST_RULE_CL, 0);

    // Israel: Fri before last Sun Mar to Last Sun Oct -> 2024: Mar 29, Oct 27
    TEST_DST_BOUND(2024, 2, 29, 1, 59, DST_RULE_IL, 0);
    TEST_DST_BOUND(2024, 2, 29, 2, 0, DST_RULE_IL, 1);
    TEST_DST_BOUND(2024, 9, 27, 1, 59, DST_RULE_IL, 1);
    TEST_DST_BOUND(2024, 9, 27, 2, 0, DST_RULE_IL, 0);

    return 0; // All paths passed
}

static int lcd_digits_to_int(const char *digits)
{
    return ((*digits - '0') * 10) + (*(digits + 1) - '0');
}

static int test_ntp_time(void)
{
    repeating_timer_t *timer = (repeating_timer_t *)calloc(1, sizeof(repeating_timer_t));
    clock_state_t *clock_state = create_test_clock_state(timer);

    // Tue January 9, 2001 at 09:28:32
    set_localtime(clock_state, 2001, 0, 9, 9, 28, 32);
    clock_state->ntp_last_sync = mock_time(NULL);
    clock_state->ntp_interval = NTP_SYNC_INTERVAL_SEC;
    mock_ntp_seconds = (mock_system_time_ms / 1000) + NTP_DELTA;

    // Run for 10 simulated days
    // +1d: normal NTP update
    // +2d: generate a KoD
    // +3d: should not update NTP
    // +4d: normal NTP update
    // +6d: create a DNS error
    // +7d: watchdog error should clear
    // +8d: normal NTP update (NTP error should clear)
    const int seconds_in_day = 24 * 60 * 60;
    int last_lcd_hour = -1;
    int last_lcd_min = -1;
    int drift = 50;
    int status = 0;
    for (unsigned int tick = 0; tick < (10 * seconds_in_day); tick++) {
        (void)clock_timer_callback(timer);
        int lcd_hour = lcd_digits_to_int(&clock_state->current_lcd_digits[3]);
        int lcd_min = lcd_digits_to_int(&clock_state->current_lcd_digits[5]);
        if (last_lcd_hour >= 0 && last_lcd_hour != lcd_hour && last_lcd_min != lcd_min) {
            time_t now = mock_system_time_ms / 1000;
            struct tm *current_time = gmtime(&now);
            if (current_time->tm_hour != lcd_hour || current_time->tm_min != lcd_min) {
                status = 1;
                printf("TIME ERROR: %02d:%02d != %02d:%02d\n", current_time->tm_hour, current_time->tm_min, lcd_hour,
                       lcd_min);
            }
        }
        last_lcd_hour = lcd_hour;
        last_lcd_min = lcd_min;
        mock_system_time_ms += 1000;

        if (tick == (2 * seconds_in_day)) {
            test_config.udp_response_type = UDP_NTP_KOD;
        }
        if (tick == (3 * seconds_in_day) && clock_state->ntp_interval != (NTP_SYNC_INTERVAL_SEC * 2)) {
            // Interval was not doubled via KoD
            return 1;
        }
        if (tick == (6 * seconds_in_day)) {
            test_config.dns_lookup_fail = 1;
            clock_state->watchdog_reset_error = WATCHDOG_NTP;
            clock_state->last_watchdog_error = (time_t)(mock_system_time_ms / 1000) + NTP_SYNC_INTERVAL_SEC / 2;
        }
        if (tick == (8 * seconds_in_day)) {
            if (clock_state->watchdog_reset_error != WATCHDOG_OK) {
                status = 1;
                printf("WATCHDOG FAILED TO CLEAR\n");
            }
            test_config.dns_lookup_fail = 0;
        }

        // Each day gets a different drift
        if (tick > 0 && (tick % seconds_in_day) == 0) {
            mock_ntp_seconds = (unsigned long long)((long long)mock_ntp_seconds + drift);
            drift = -drift;
        } else {
            mock_ntp_seconds++;
        }
    }
    test_config.udp_response_type = UDP_NTP_OK;
    return status;
}

static int test_ntp_errors(void)
{
    test_config.udp_response_type = UDP_NTP_INVALID;
    if (test_main() != 1) {
        return 1;
    }
    test_config.udp_response_type = UDP_NTP_BAD_LEN;
    if (test_main() != 1) {
        return 1;
    }
    test_config.udp_response_type = UDP_NTP_BAD_PORT;
    if (test_main() != 1) {
        return 1;
    }
    test_config.udp_new_ip_type_fail = 1;
    if (test_main() != 1) {
        return 1;
    }
    test_config.udp_new_ip_type_fail = 0;
    pbuf_alloc_fail_at = 1;
    if (test_main() != 1) {
        return 1;
    }
    pbuf_alloc_fail_at = 0;
    test_config.udp_sendto_fail = 1;
    if (test_main() != 1) {
        return 1;
    }
    test_config.udp_sendto_fail = 0;
    test_config.udp_invalid_addr = 1;
    if (test_main() != 1) {
        return 1;
    }
    test_config.udp_invalid_addr = 0;
    calloc_counter = 0;
    calloc_fail_at = 9; // Clock, 7x LCD, fail on NTP
    if (test_main() != 1) {
        return 1;
    }
    return 0;
}

static int test_watchdog(void)
{
    if (test_main() != 0) {
        return 1;
    }
    test_config.watchdog_caused_reboot = 1;
    if (test_main() != 0) {
        return 1;
    }
    if (test_main() != 0 || persistent_state.boot_count != 2) {
        return 1;
    }
    test_config.watchdog_caused_reboot = 0;
    test_config.cyw43_arch_init_fail = 1;
    if (test_main() != 1 || !watchdog_reboot_called) {
        return 1;
    }
    watchdog_reboot_called = 0;
    test_config.cyw43_arch_init_fail = 0;
    test_config.dns_lookup_fail = 1;
    if (test_main() != 1 || !watchdog_reboot_called) {
        return 1;
    }
    test_config.watchdog_caused_reboot = 1;
    test_config.dns_lookup_fail = 0;
    if (test_main() != 0) {
        return 1;
    }

    // Coverage on otherwise invalid status debug values
    if ((strcmp(watchdog_error_to_string((watchdog_error_t)0xff), "UNKNOWN_STATUS") != 0) ||
        (strcmp(wifi_error_to_string((wifi_error_t)0xff), "UNKNOWN_STATUS") != 0) ||
        (strcmp(ntp_error_to_string((ntp_error_t)0xff), "UNKNOWN_STATUS") != 0)) {
        return 1;
    }
    return 0;
}

int main(const int argc, const char *argv[])
{
    int status = 0;
    if (argc > 1) {
        if (strcmp(argv[1], "--verbose") == 0) {
            test_verbose = 1;
        } else {
            fprintf(stderr, "Warning: unknown command-line argument %s\n", argv[1]);
        }
    }

    static const char *test_bad_ldc1_ref[] = {
        "Test init", "Failed to allocate clock state", "Test init", "LCD 1: failed to initialise", NULL,
    };
    status |= run_test(test_bad_lcd1, "LCD1 init error", test_bad_ldc1_ref);

    static const char *test_dst_ref[] = {
        "Icons: OK OK OK",
        NULL,
    };
    status |= run_test(test_dst, "Daylight savings", test_dst_ref);

    static const char *test_ntp_recovery_ref[] = {
        "Icons: NTP OK OK", "Icons: NTP DNS_ERROR OK", "Icons: OK DNS_ERROR OK", "Icons: OK OK OK", NULL,
    };
    status |= run_test(test_ntp_time, "NTP time checks", test_ntp_recovery_ref);

    static const char *test_wifi_init_errors_ref[] = {
        // Wi-Fi init fails
        "Test init",
        "LCD init successful",
        "Watchdog: NTP_OK/WIFI_INIT_ERROR",
        "Reboot: NTP_OK/WIFI_INIT_ERROR",
        "Icons: WIFI OK INIT_ERROR",
        // Wi-Fi connect fails
        "Test init",
        "LCD init successful",
        "Watchdog: NTP_OK/WIFI_CONNECT_ERROR",
        "Reboot: NTP_OK/WIFI_CONNECT_ERROR",
        "Icons: WIFI OK CONNECT_ERROR",
        // Unknown Wi-Fi error
        "Test init",
        "LCD init successful",
        "Watchdog: NTP_OK/WIFI_UNKNOWN_ERROR",
        "Reboot: NTP_OK/WIFI_UNKNOWN_ERROR",
        "Icons: WIFI OK UNKNOWN_ERROR",
        // Successful connection after a few timeouts
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "NTP time sync OK",
        // Failed connection due to timeout
        "Test init",
        "LCD init successful",
        "Watchdog: NTP_OK/WIFI_TIMEOUT_ERROR",
        "Reboot: NTP_OK/WIFI_TIMEOUT_ERROR",
        "Icons: WIFI OK TIMEOUT_ERROR",
        NULL,
    };
    status |= run_test(test_wifi_init_errors, "Wi-Fi init error", test_wifi_init_errors_ref);

    static const char *test_wifi_auth_errors_ref[] = {
        // Wi-Fi auth retries succeed
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "NTP time sync OK",
        // Wi-Fi auth retries too many
        "Test init",
        "LCD init successful",
        "Watchdog: NTP_OK/WIFI_AUTH_ERROR",
        "Reboot: NTP_OK/WIFI_AUTH_ERROR",
        "Icons: WIFI OK AUTH_ERROR",
        NULL,
    };
    status |= run_test(test_wifi_auth_errors, "Wi-Fi auth", test_wifi_auth_errors_ref);

    static const char *test_dns_lookup_ref[] = {
        // DNS lookup bad args
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "Watchdog: NTP_DNS_ERROR/WIFI_OK",
        "Reboot: NTP_DNS_ERROR/WIFI_OK",
        "Icons: NTP DNS_ERROR OK",
        // DNS lookup failed
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "Watchdog: NTP_DNS_ERROR/WIFI_OK",
        "Reboot: NTP_DNS_ERROR/WIFI_OK",
        "Icons: NTP DNS_ERROR OK",
        // DNS lookup times out
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "Watchdog: NTP_TIMEOUT_ERROR/WIFI_OK",
        "Reboot: NTP_TIMEOUT_ERROR/WIFI_OK",
        "Icons: NTP TIMEOUT_ERROR OK",
        // DNS lookup OK after delays
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "NTP time sync OK",
        NULL,
    };
    status |= run_test(test_dns_lookups, "DNS lookups", test_dns_lookup_ref);

    static const char *test_ntp_errors_ref[] = {
        // UDL invalid packets
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "Watchdog: NTP_PROTOCOL_ERROR/WIFI_OK",
        "Reboot: NTP_PROTOCOL_ERROR/WIFI_OK",
        "Icons: NTP PROTOCOL_ERROR OK",
        // UDP bad packet length
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "Watchdog: NTP_PROTOCOL_ERROR/WIFI_OK",
        "Reboot: NTP_PROTOCOL_ERROR/WIFI_OK",
        "Icons: NTP PROTOCOL_ERROR OK",
        // NTP wrong port
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "Watchdog: NTP_PROTOCOL_ERROR/WIFI_OK",
        "Reboot: NTP_PROTOCOL_ERROR/WIFI_OK",
        "Icons: NTP PROTOCOL_ERROR OK",
        // UDP bad IP type
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "Watchdog: NTP_INIT_ERROR/WIFI_OK",
        "Reboot: NTP_INIT_ERROR/WIFI_OK",
        "Icons: NTP INIT_ERROR OK",
        // UDP memory alloc fails
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "Watchdog: NTP_MEMORY_ERROR/WIFI_OK",
        "Reboot: NTP_MEMORY_ERROR/WIFI_OK",
        "Icons: NTP MEMORY_ERROR OK",
        // UDP sendto() fails
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "Watchdog: NTP_PROTOCOL_ERROR/WIFI_OK",
        "Reboot: NTP_PROTOCOL_ERROR/WIFI_OK",
        "Icons: NTP PROTOCOL_ERROR OK",
        // Return address invalid
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "Watchdog: NTP_PROTOCOL_ERROR/WIFI_OK",
        "Reboot: NTP_PROTOCOL_ERROR/WIFI_OK",
        "Icons: NTP PROTOCOL_ERROR OK",
        // Last alloc fails
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "Watchdog: NTP_INIT_ERROR/WIFI_OK",
        "Reboot: NTP_INIT_ERROR/WIFI_OK",
        "Icons: NTP INIT_ERROR OK",
        NULL,
    };
    status |= run_test(test_ntp_errors, "NTP errors", test_ntp_errors_ref);

    static const char *test_watchdog_ref[] = {
        // Test boot counter
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "NTP time sync OK",
        // Test watchdog caused reset
        "Test init",
        "Icons: RESET OK OK",
        "Icons: OK OK OK",
        // Restart clock after watchdog
        "Test init",
        "Icons: RESET OK OK",
        "Icons: OK OK OK",
        // Normal start with Wi-Fi error
        "Test init",
        "LCD init successful",
        "Watchdog: NTP_OK/WIFI_INIT_ERROR",
        "Reboot: NTP_OK/WIFI_INIT_ERROR",
        "Icons: WIFI OK INIT_ERROR",
        // Restart clock after watchdog for NTP error
        "Test init",
        "LCD init successful",
        "Connected to WiFi",
        "Icons: OK OK OK",
        "Watchdog: NTP_DNS_ERROR/WIFI_OK",
        "Reboot: NTP_DNS_ERROR/WIFI_OK",
        "Icons: NTP DNS_ERROR OK",
        // Fake a watchdog reset for unknown cause
        "Test init",
        "Icons: RESET OK OK",
        "Icons: OK OK OK",
        NULL,
    };
    status |= run_test(test_watchdog, "Watchdog", test_watchdog_ref);

    return status;
}
