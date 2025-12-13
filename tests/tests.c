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
int watchdog_reboot_called = 0;
char **log_buffer;

#define LOG_ERROR_WIDTH 40

static int run_test(test_func_t func, const char *test_name, const char **expected_log)
{
    log_buffer = (char **)calloc(sizeof(char *), LOG_BUFFER_SIZE);

    // Always init the test config
    memset(&test_config, 0, sizeof(test_config_t));
    log_buffer_size = 0;

    // Run test
    int status = func();

    if (status == 1) {
        printf("TEST %s: FAIL\n", test_name);
    } else {
        printf("TEST %s: OK\n", test_name);
    }

    if (expected_log == NULL) {
        return status;
    }

    int log_mismatch = 0;
    uint32_t expected_log_size = 0;
    for (unsigned int ii = 0; expected_log[ii] != NULL; ii++) {
        if (ii > expected_log_size) {
            expected_log_size = ii;
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
        for (size_t jj = test_len - 1; jj > 0; jj--) {
            if (log_buffer[ii][jj] == '\r' || log_buffer[ii][jj] == '\n') {
                log_buffer[ii][jj] = (char)0;
                test_len--;
            }
        }
        if (test_len != ref_len || strncmp(log_buffer[ii], expected_log[ii], max_len) != 0) {
            log_mismatch = 1;
        }
    }

    if (log_mismatch) {
        uint32_t heading_width = ((LOG_ERROR_WIDTH * 2) - (uint32_t)strlen(test_name) - 12) / 4;
        printf("%.*s [REF] %.*s %s %.*s [TEST] %.*s\n", heading_width, "================", heading_width,
               "================", test_name, heading_width, "================", heading_width, "================");
        for (unsigned int ii = 0; ii < expected_log_size; ii++) {
            size_t test_len = strlen(log_buffer[ii]);
            size_t ref_len = strlen(expected_log[ii]);
            size_t max_len = (ref_len > test_len) ? test_len : ref_len;
            int match = strncmp(log_buffer[ii], expected_log[ii], max_len) == 0 && test_len == max_len;
            printf("  %-*s %c %s\n", LOG_ERROR_WIDTH, expected_log[ii], match ? '|' : 'x',
                   (log_buffer_size >= ii) ? log_buffer[ii] : "");
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

static void set_localtime(int year, int mon, int mday, int hour, int min, int sec)
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
    mock_system_time_ms = (unsigned long long)t * 1000;
}

static clock_state_t *create_test_clock_state(repeating_timer_t *timer)
{
    clock_state_t *clock_state = (clock_state_t *)calloc(1, sizeof(clock_state_t));
    for (unsigned int ii = 0; ii < NUM_LCDS; ii++) {
        clock_state->lcd_states[ii] = lcd_init(0, 0, 0, 0, 0, 0, 0);
    }
    clock_state->ntp_state = ntp_init((void *)clock_state, ntp_timer_callback);
    clock_state->ntp_last_sync = mock_time(NULL);
    clock_state->init_done = 0;
    timer->user_data = clock_state;
    return clock_state;
}

static int test_dst(void)
{
    repeating_timer_t *timer = (repeating_timer_t *)calloc(1, sizeof(repeating_timer_t));
    clock_state_t *clock_state = create_test_clock_state(timer);

    // Coverage test for Zeller's congruence
    if (last_day_of_month(28, 2, 2025) != /* Friday */ 5) {
        return 1;
    }

    // Sun March 25, 2001 at 00:22 (just before clocks change)
    set_localtime(2001, 2, 25, 0, 22, 0);
    clock_state->ntp_last_sync = mock_time(NULL);
    clock_state->ntp_interval = NTP_SYNC_INTERVAL_SEC;
    (void)clock_timer_callback(timer);
    if (strncmp(clock_state->current_lcd_digits, "Sun0022", 7) != 0) {
        return 1;
    }
    time_t now = mock_time(NULL);
    if (strncmp(time_as_string(now), "00:22:00", 14) != 0) {
        return 1;
    }

    // Sun March 25, 2001 at 01:22 (just after clocks change)
    set_localtime(2001, 2, 25, 1, 22, 0);
    (void)clock_timer_callback(timer);
    if (strncmp(clock_state->current_lcd_digits, "Sun0222", 7) != 0) {
        return 1;
    }
    now = mock_time(NULL);
    if (strncmp(time_as_string(now), "02:22:00 (DST)", 14) != 0) {
        return 1;
    }

    // Thu August 23, 2001 at 23:55 (test day rollover in DST))
    set_localtime(2001, 7, 23, 23, 55, 0);
    mock_ntp_seconds = (mock_system_time_ms / 1000) + NTP_DELTA;
    (void)clock_timer_callback(timer);
    if (strncmp(clock_state->current_lcd_digits, "Fri0055", 7) != 0) {
        return 1;
    }
    now = mock_time(NULL);
    if (strncmp(time_as_string(now), "00:55:00 (DST)", 14) != 0) {
        return 1;
    }
    return 0;
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
    set_localtime(2001, 0, 9, 9, 28, 32);
    clock_state->ntp_last_sync = mock_time(NULL);
    clock_state->ntp_interval = NTP_SYNC_INTERVAL_SEC;
    mock_ntp_seconds = (mock_system_time_ms / 1000) + NTP_DELTA;

    // Run for 9 simulated days
    // +1d: normal NTP update
    // +2d: generate a KoD
    // +3d: should not update NTP
    // +4d: normal NTP update
    // +6d: create a DNS error
    // +8d: normal NTP update
    const int seconds_in_day = 24 * 60 * 60;
    int last_lcd_hour = -1;
    int last_lcd_min = -1;
    int drift = 50;
    int status = 0;
    for (unsigned int tick = 0; tick < (9 * seconds_in_day); tick++) {
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
        }
        if (tick == (8 * seconds_in_day)) {
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

    // Coverage on otherwise unused status debug values
    if ((strcmp(status_to_string(STATUS_WIFI_OK), "STATUS_WIFI_OK") != 0) ||
        (strcmp(status_to_string(STATUS_NTP_OK), "STATUS_NTP_OK") != 0) ||
        (strcmp(status_to_string(STATUS_WATCHDOG_RESET), "STATUS_WATCHDOG_RESET") != 0) ||
        (strcmp(status_to_string(STATUS_NONE), "STATUS_NONE") != 0) ||
        (strcmp(status_to_string((clock_status_t)0xff), "UNKNOWN_STATUS") != 0)) {
        return 1;
    }
    return 0;
}

int main(void)
{
    int status = 0;

    static const char *test_empty_log[] = {NULL};
    static const char *test_bad_ldc1_ref[] = {
        "Failed to allocate clock state",
        "LCD 1: failed to initialise",
        NULL,
    };
    status |= run_test(test_bad_lcd1, "LCD1 init error", test_bad_ldc1_ref);

    status |= run_test(test_dst, "Daylight savings", test_empty_log);

    static const char *test_ntp_recovery_ref[] = {
        "LCD: NTP_STATUS_DNS_ERROR=RED",
        NULL,
    };
    status |= run_test(test_ntp_time, "NTP time checks", test_ntp_recovery_ref);

    static const char *test_wifi_init_errors_ref[] = {
        // Wi-Fi init fails
        "LCD: LCD init successful",
        "LCD: STATUS_WIFI_INIT=RED",
        // Wi-Fi connect fails
        "LCD: LCD init successful",
        "LCD: STATUS_WIFI_CONNECT=RED",
        // Unknown Wi-Fi error
        "LCD: LCD init successful",
        "LCD: STATUS_WIFI_ERROR=RED",
        // Successful connection after a few timeouts
        "LCD: LCD init successful",
        "LCD: Connected to WiFi",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: NTP time sync OK",
        "LCD: STATUS_NTP_OK=GREEN",
        // Failed connection due to timeout
        "LCD: LCD init successful",
        "LCD: STATUS_WIFI_TIMEOUT=RED",
        NULL,
    };
    status |= run_test(test_wifi_init_errors, "Wi-Fi init error", test_wifi_init_errors_ref);

    static const char *test_wifi_auth_errors_ref[] = {
        // Wi-Fi auth retries succeed
        "LCD: LCD init successful",
        "LCD: Connected to WiFi",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: NTP time sync OK",
        "LCD: STATUS_NTP_OK=GREEN",
        // Wi-Fi auth retries too many
        "LCD: LCD init successful",
        "LCD: STATUS_WIFI_AUTH=RED",
        NULL,
    };
    status |= run_test(test_wifi_auth_errors, "Wi-Fi auth", test_wifi_auth_errors_ref);

    static const char *test_dns_lookup_ref[] = {
        // DNS lookup bad args
        "LCD: LCD init successful",
        "LCD: Connected to WiFi",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_DNS=RED",
        // DNS lookup failed
        "LCD: LCD init successful",
        "LCD: Connected to WiFi",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_DNS=RED",
        // DNS lookup times out
        "LCD: LCD init successful",
        "LCD: Connected to WiFi",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_TIMEOUT=RED",
        // DNS lookup OK after delays
        "LCD: LCD init successful",
        "LCD: Connected to WiFi",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: NTP time sync OK",
        "LCD: STATUS_NTP_OK=GREEN",
        NULL,
    };
    status |= run_test(test_dns_lookups, "DNS lookups", test_dns_lookup_ref);

    static const char *test_ntp_errors_ref[] = {
        // UDL invalid packets
        "LCD: LCD init successful",
        "LCD: Connected to WiFi",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_INVALID=RED",
        // UDP bad packet length
        "LCD: LCD init successful",
        "LCD: Connected to WiFi",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_INVALID=RED",
        // NTP wrong port
        "LCD: LCD init successful",
        "LCD: Connected to WiFi",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_INVALID=RED",
        // UDP bad IP type
        "LCD: LCD init successful",
        "LCD: Connected to WiFi",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_INIT=RED",
        // UDP memory alloc fails
        "LCD: LCD init successful",
        "LCD: Connected to WiFi",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_MEMORY=RED",
        // UDP sendto() fails
        "LCD: LCD init successful",
        "LCD: Connected to WiFi",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_INVALID=RED",
        // Last alloc fails
        "LCD: LCD init successful",
        "LCD: Connected to WiFi",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_INIT=RED",
        NULL,
    };
    status |= run_test(test_ntp_errors, "NTP errors", test_ntp_errors_ref);

    static const char *test_watchdog_ref[] = {
        // Test boot counter
        "LCD: LCD init successful",
        "LCD: Connected to WiFi",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: NTP time sync OK",
        "LCD: STATUS_NTP_OK=GREEN",
        // Test watchdog caused reset
        "LCD: STATUS_WATCHDOG_RESET=RED",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_OK=GREEN",
        // Restart clock after watchdog
        "LCD: STATUS_WATCHDOG_RESET=RED",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_OK=GREEN",
        // Normal start with Wi-Fi error
        "LCD: LCD init successful",
        "LCD: STATUS_WIFI_INIT=RED",
        // Restart clock after watchdog for NTP error
        "LCD: LCD init successful",
        "LCD: Connected to WiFi",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_DNS=RED",
        "LCD: STATUS_WATCHDOG_RESET_FOR_NTP=RED",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_OK=GREEN",
        NULL,
    };
    status |= run_test(test_watchdog, "Watchdog", test_watchdog_ref);

    return status;
}
