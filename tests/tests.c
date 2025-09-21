#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "clock.h"
#include "test.h"
#include "wifi.h"

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
unsigned int pbuf_alloc_fail_at = 0;
char **log_buffer;

static int run_test(test_func_t func, const char *test_name, const char **expected_log)
{
    log_buffer = calloc(sizeof(char *), LOG_BUFFER_SIZE);

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

    bool log_mismatch = false;
    uint32_t max_log_width = 0;
    uint32_t expected_log_size = 0;
    for (unsigned int ii = 0; ii < log_buffer_size; ii++) {
        if (strlen(log_buffer[ii]) > max_log_width) {
            max_log_width = strlen(log_buffer[ii]);
        }
    }
    for (unsigned int ii = 0; expected_log[ii] != NULL; ii++) {
        if (ii > expected_log_size) {
            expected_log_size = ii;
        }
        if (strlen(expected_log[ii]) > expected_log_size) {
            max_log_width = strlen(expected_log[ii]);
        }
    }
    for (unsigned int ii = 0; expected_log[ii] != NULL; ii++) {
        if (ii >= log_buffer_size) {
            log_mismatch = true;
            break;
        }
        unsigned int test_len = strlen(log_buffer[ii]);
        unsigned int ref_len = strlen(expected_log[ii]);
        unsigned int max_len = (ref_len > test_len) ? test_len : ref_len;
        for (int jj = test_len - 1; jj >= 0; jj--) {
            if (log_buffer[ii][jj] == '\r' || log_buffer[ii][jj] == '\n') {
                log_buffer[ii][jj] = (char)0;
                test_len--;
            }
        }
        if (test_len != ref_len || strncmp(log_buffer[ii], expected_log[ii], max_len) != 0) {
            log_mismatch = true;
        }
    }

    if (log_mismatch) {
        printf("===== [REF] ======= %s ===== [TEST] =======\n", test_name);
        for (unsigned int ii = 0; ii < expected_log_size; ii++) {
            printf("  %-*s | %s\n", max_log_width, expected_log[ii], (log_buffer_size >= ii) ? log_buffer[ii] : "");
        }
        for (unsigned int ii = expected_log_size; ii < log_buffer_size; ii++) {
            printf("  %-*s | %s\n", max_log_width, "", log_buffer[ii]);
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
    .cyw43_arch_init_fail = false,
    .udp_new_ip_type_fail = false,
    .udp_sendto_fail = false,
    .dns_lookup_delay = 0,
    .dns_lookup_fail = false,
    .watchdog_caused_reboot = false,
};

int test_bad_lcd1(void)
{
    calloc_fail_at = 2;
    int status = test_main();
    calloc_fail_at = 0;
    return (status == 1) ? 0 : 1;
}

int test_dns_lookups(void)
{
    test_config.dns_bad_arg = true;
    if (test_main() != 1) {
        return 1;
    }
    test_config.dns_lookup_fail = true;
    if (test_main() != 1) {
        return 1;
    }
    // DNS poll loops every 500ms for 30s
    test_config.dns_lookup_delay = 61;
    test_config.dns_lookup_fail = false;
    if (test_main() != 1) {
        return 1;
    }
    // DNS poll loops every 500ms for 30s
    test_config.dns_lookup_delay = 59;
    if (test_main() != 0) {
        return 1;
    }
    return 0;
}

int test_wifi_init_errors(void)
{
    test_config.cyw43_arch_init_fail = true;
    if (test_main() != 1) {
        return 1;
    }
    test_config.cyw43_arch_init_fail = false;
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

int test_wifi_auth_errors(void)
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

extern bool clock_timer_callback(repeating_timer_t *);
extern unsigned long long mock_system_time_ms;

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
    mock_system_time_ms = t * 1000;
}

extern void ntp_timer_callback(void *state, time_t *ntp_time);
extern unsigned mock_ntp_seconds;

static clock_state_t *create_test_clock_state(repeating_timer_t *timer)
{
    clock_state_t *clock_state = (clock_state_t *)calloc(1, sizeof(clock_state_t));
    for (unsigned int ii = 0; ii < NUM_LCDS; ii++) {
        clock_state->lcd_states[ii] = lcd_init(0, 0, 0, 0, 0, 0, false);
    }
    clock_state->ntp_state = ntp_init((void *)clock_state, ntp_timer_callback);
    clock_state->ntp_last_sync = mock_time(NULL);
    clock_state->init_done = false;
    timer->user_data = clock_state;
    return clock_state;
}

extern int last_day_of_month(int day, int month, int year);

int test_dst(void)
{
    repeating_timer_t *timer = calloc(1, sizeof(repeating_timer_t));
    clock_state_t *clock_state = create_test_clock_state(timer);

    // Coverage test for Zeller's congruence
    if (last_day_of_month(28, 2, 2025) != /* Friday */ 5) {
        return 1;
    }

    // Sun March 25, 2001 at 00:22 (just before clocks change)
    set_localtime(2001, 2, 25, 0, 22, 0);
    clock_state->ntp_last_sync = mock_time(NULL);
    (void)clock_timer_callback(timer);
    if (strncmp(clock_state->current_lcd_digits, "SUN0022", 7) != 0) {
        return 1;
    }
    time_t now = mock_time(NULL);
    if (strncmp(time_as_string(now), "00:22:00", 14) != 0) {
        return 1;
    }

    // Sun March 25, 2001 at 01:22 (just after clocks change)
    set_localtime(2001, 2, 25, 1, 22, 0);
    (void)clock_timer_callback(timer);
    if (strncmp(clock_state->current_lcd_digits, "SUN0222", 7) != 0) {
        return 1;
    }
    now = mock_time(NULL);
    if (strncmp(time_as_string(now), "02:22:00 (DST)", 14) != 0) {
        return 1;
    }
    return 0;
}

int lcd_digits_to_int(const char *digits)
{
    return ((*digits - '0') * 10) + (*(digits + 1) - '0');
}

int test_ntp_time(void)
{
    repeating_timer_t *timer = calloc(1, sizeof(repeating_timer_t));
    clock_state_t *clock_state = create_test_clock_state(timer);

    // Tue January 9, 2001 at 09:28:32
    set_localtime(2001, 0, 9, 9, 28, 32);
    clock_state->ntp_last_sync = mock_time(NULL);
    clock_state->ntp_interval = NTP_SYNC_INTERVAL_SEC;
    mock_ntp_seconds = (mock_system_time_ms / 1000) + NTP_DELTA;
    // Run for 5 simulated days
    int last_lcd_hour = -1;
    int last_lcd_min = -1;
    int drift = 50;
    int status = 0;
    for (unsigned int tick = 0; tick < (5 * 24 * 60 * 60); tick++) {
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
        if (tick == (2 * 24 * 60 * 60)) {
            test_config.udp_ntp_kod = true;
        }
        if (tick == (3 * 24 * 60 * 60) && clock_state->ntp_interval != (NTP_SYNC_INTERVAL_SEC * 2)) {
            return 1;
        }
        if (tick > 0 && (tick % (24 * 60 * 60)) == 0) {
            mock_ntp_seconds += drift;
            drift = -drift;
        } else {
            mock_ntp_seconds++;
        }
    }
    return status;
}

int test_ntp_errors(void)
{
    test_config.udp_invalid_response = true;
    if (test_main() != 1) {
        return 1;
    }
    test_config.udp_invalid_response = false;
    test_config.udp_new_ip_type_fail = true;
    if (test_main() != 1) {
        return 1;
    }
    test_config.udp_new_ip_type_fail = false;
    pbuf_alloc_fail_at = 1;
    if (test_main() != 1) {
        return 1;
    }
    pbuf_alloc_fail_at = 0;
    test_config.udp_sendto_fail = true;
    if (test_main() != 1) {
        return 1;
    }
    return 0;
}

int main(void)
{
    int status = 0;

    static const char *test_bad_ldc1_ref[] = {
        "LCD 1: failed to initialise",
        NULL,
    };
    status |= run_test(test_bad_lcd1, "LCD1 init error", test_bad_ldc1_ref);

    status |= run_test(test_dst, "Daylight savings", NULL);

    status |= run_test(test_ntp_time, "NTP time checks", NULL);

    static const char *test_wifi_init_errors_ref[] = {
        "LCD: STATUS_WIFI_INIT=RED",
        "LCD: STATUS_WIFI_CONNECT=RED",
        "LCD: STATUS_WIFI_ERROR=RED",
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_OK=GREEN",
        "LCD: STATUS_WIFI_TIMEOUT=RED",
        NULL,
    };
    status |= run_test(test_wifi_init_errors, "Wi-Fi init error", test_wifi_init_errors_ref);

    static const char *test_wifi_auth_errors_ref[] = {
        "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_OK=GREEN",
        "LCD: STATUS_WIFI_AUTH=RED",
        NULL,
    };
    status |= run_test(test_wifi_auth_errors, "Wi-Fi auth", test_wifi_auth_errors_ref);

    static const char *test_dns_lookup_ref[] = {
        "LCD: STATUS_WIFI_OK=GREEN", "LCD: STATUS_NTP_DNS=RED",   "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_DNS=RED",   "LCD: STATUS_WIFI_OK=GREEN", "LCD: STATUS_NTP_TIMEOUT=RED",
        "LCD: STATUS_WIFI_OK=GREEN", "LCD: STATUS_NTP_OK=GREEN",  NULL,
    };
    status |= run_test(test_dns_lookups, "DNS lookups", test_dns_lookup_ref);

    static const char *test_ntp_errors_ref[] = {
        "LCD: STATUS_WIFI_OK=GREEN", "LCD: STATUS_NTP_INVALID=RED", "LCD: STATUS_WIFI_OK=GREEN",
        "LCD: STATUS_NTP_INIT=RED",  "LCD: STATUS_WIFI_OK=GREEN",   "LCD: STATUS_NTP_MEMORY=RED",
        "LCD: STATUS_WIFI_OK=GREEN", "LCD: STATUS_NTP_INVALID=RED", NULL,
    };
    status |= run_test(test_ntp_errors, "NTP errors", test_ntp_errors_ref);
    return status;
}
