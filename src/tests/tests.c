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
char **log_buffer;

static int run_test(test_func_t func, const char *test_name, const char **expected_log)
{
    printf("run_test=%s\n", test_name);
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

    for (unsigned int ii = 0; expected_log[ii] != NULL; ii++) {
        if (ii >= log_buffer_size) {
            printf("TEST %s: ERROR: log truncated at offset #%d\n", test_name, ii);
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
            printf("TEST %s: ERROR    : log mismatch position #%d\n", test_name, ii);
            printf("TEST %s: EXPECTED : >>%s<<\n", test_name, expected_log[ii]);
            printf("TEST %s: FOUND    : >>%s<<\n", test_name, log_buffer[ii]);
        }
    }
    for (unsigned int ii = 0; ii <= log_buffer_size; ii++) {
        free(log_buffer[ii]);
    }
    free(log_buffer);
    return status;
}

test_config_t test_config = {
    .cyw43_arch_init_fail = false,
    .udp_new_ip_type_fail = false,
};

int test_bad_lcd1(void)
{
    calloc_fail_at = 1;
    int status = test_main();
    calloc_fail_at = 0;
    return (status == 1) ? 0 : 1;
}

int test_cy43_init_errors(void)
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

int test_cy43_auth_errors(void)
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

int test_bad_udp_alloc(void)
{
    test_config.udp_new_ip_type_fail = true;
    return (test_main() == 1) ? 0 : 1;
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

int test_dst(void)
{
    repeating_timer_t *timer = calloc(1, sizeof(repeating_timer_t));
    clock_state_t *clock_state = create_test_clock_state(timer);

    // Sun March 25, 2001 at 00:22 (just before clocks change)
    set_localtime(2001, 2, 25, 0, 22, 0);
    clock_state->ntp_last_sync = mock_time(NULL);
    (void)clock_timer_callback(timer);
    if (strncmp(clock_state->current_lcd_digits, "SUN0022", 7) != 0) {
        return 1;
    }
    // Sun March 25, 2001 at 01:22 (just after clocks change)
    set_localtime(2001, 2, 25, 1, 22, 0);
    (void)clock_timer_callback(timer);
    if (strncmp(clock_state->current_lcd_digits, "SUN0222", 7) != 0) {
        return 1;
    }
    return 0;
}

int lcd_digits_to_int(const char *digits)
{
    return ((*digits - '0') * 10) + (*(digits + 1) - '0');
}

int test_ntp_drift(void)
{
    repeating_timer_t *timer = calloc(1, sizeof(repeating_timer_t));
    clock_state_t *clock_state = create_test_clock_state(timer);

    // Tue January 9, 2001 at 09:28:32
    set_localtime(2001, 0, 9, 9, 28, 32);
    clock_state->ntp_last_sync = mock_time(NULL);
    mock_ntp_seconds = (mock_system_time_ms / 1000) + NTP_DELTA;
    // Run for 3 simulated days
    int last_lcd_hour = -1;
    int last_lcd_min = -1;
    int drift = 50;
    int status = 0;
    for (unsigned int ii = 0; ii < (3 * 24 * 60 * 60); ii++) {
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
        if (ii > 0 && (ii % (24 * 60 * 60)) == 0) {
            mock_ntp_seconds += drift;
            drift = -drift;
        } else {
            mock_ntp_seconds++;
        }
    }
    return status;
}

int main(void)
{
    int status = 0;
    printf("Hello testing!\n");

    static const char *test_bad_ldc1_ref[] = {
        "LCD 1: failed to initialise",
        NULL,
    };
    status |= run_test(test_bad_lcd1, "LCD1 init error", test_bad_ldc1_ref);

    status |= run_test(test_dst, "Daylight savings", NULL);

    status |= run_test(test_ntp_drift, "NTP drift", NULL);

    static const char *test_cy43_init_errors_ref[] = {
        // CY43 init fails
        "LCD: LCD init OK",
        "LCD: Wi-Fi init error",
        // CY43 connection failure
        "LCD: LCD init OK",
        "LCD: Wi-Fi connect error",
        // CY43 unknown failure
        "LCD: LCD init OK",
        "LCD: Wi-Fi unknown error",
        // Timeout OK
        "LCD: LCD init OK",
        "LCD: Wi-Fi connect OK",
        "LCD: NTP init OK",
        "LCD: NTP time OK",
        // Timeout
        "LCD: LCD init OK",
        "LCD: Wi-Fi timeout error",
        NULL,
    };
    status |= run_test(test_cy43_init_errors, "cyw43_arch_init error", test_cy43_init_errors_ref);

    static const char *test_cy43_auth_errors_ref[] = {
        // Bad auth within retry count
        "LCD: LCD init OK",
        "LCD: Wi-Fi connect OK",
        "LCD: NTP init OK",
        "LCD: NTP time OK",
        // Bad auth exceeds retry count
        "LCD: LCD init OK",
        "LCD: Wi-Fi auth error",
        NULL,
    };
    status |= run_test(test_cy43_auth_errors, "Wi-Fi auth", test_cy43_auth_errors_ref);

    static const char *test_bad_udp_alloc_ref[] = {
        "LCD: LCD init OK",
        "LCD: Wi-Fi connect OK",
        "LCD: NTP init error",
        NULL,
    };
    status |= run_test(test_bad_udp_alloc, "udp_new_ip_type error", test_bad_udp_alloc_ref);

    return status;
}
