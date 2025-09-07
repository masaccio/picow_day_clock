#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

typedef int (*test_func_t)(void);

unsigned int log_buffer_size = 0;
unsigned int calloc_fail_at = 0;
char **log_buffer;

static void run_test(test_func_t func, const char *test_name, bool expect_fail, const char **expected_log)
{
    /* Always init the test config */
    memset(&test_config, 0, sizeof(test_config_t));
    log_buffer_size = 0;

    /* Run test */
    int status = func();

    if (status == 1 && expect_fail) {
        printf("TEST %s: OK (expected failure)\n", test_name);
    } else if (status == 0 && expect_fail) {
        printf("TEST %s: FAIL (unexpected success)\n", test_name);
    } else if (status == 1) {
        printf("TEST %s: FAIL\n", test_name);
    } else {
        printf("TEST %s: OK\n", test_name);
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
    return status == 1;
}

int test_bad_cy43_init(void)
{
    test_config.cyw43_arch_init_fail = true;
    return test_main() == 1;
}

int test_bad_udp_alloc(void)
{
    test_config.udp_new_ip_type_fail = true;
    return test_main() == 1;
}

int main(void)
{
    log_buffer = calloc(sizeof(char *), LOG_BUFFER_SIZE);

    static const char *test_bad_ldc1_ref[] = {
        "LCD 1: failed to initialise",
        NULL,
    };
    run_test(test_bad_lcd1, "LCD1 init error", true, test_bad_ldc1_ref);

    // static const char *test_bad_cy43_init_ref[] = {
    //     "Wi-Fi: failed to initialise CYW43",
    //     NULL,
    // };
    // run_test(test_bad_cy43_init, "cyw43_arch_init error", true, test_bad_cy43_init_ref);

    // static const char *test_bad_udp_alloc_ref[] = {
    //     "Wi-Fi: connected to my-test-ssid",
    //     "Failed to allocate UDP buffer",
    //     "NTP: failed to initialise",
    //     NULL,
    // };
    // run_test(test_bad_udp_alloc, "udp_new_ip_type error", true, test_bad_udp_alloc_ref);

    return 0;
}
