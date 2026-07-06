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
#undef free

typedef int (*test_func_t)(void);
static unsigned int test_verbose = 0;

mock_context_t mock_ctx = {0};
#define RESET_MOCK_CONFIG()                                                                                            \
    do {                                                                                                               \
        memset((void *)&mock_ctx.inject, 0, sizeof(mock_ctx.inject));                                                  \
        memset((void *)&mock_ctx.spy, 0, sizeof(mock_ctx.spy));                                                        \
        mock_ctx.sim.dns_pending = false;                                                                              \
        mock_ctx.sim.udp_pending = false;                                                                              \
        mock_ctx.sim.timer_active = false;                                                                             \
    } while (0)

#define STR(x) #x
#define TEST_AP_SSID "HomeNet"
#define TEST_AP_PASSWORD "s3cr3t"
#define TEST_NTP_SERVER "time.apple.com"
#define TEST_TZ_OFFSET 0
#define TEST_DST_RULE 0
#define TEST_TIMEOUT 10 * 1000
#define TEST_NTP_PORT 8123
#define TEST_CONFIG_URL(ssid, pwd, ntp, tz, dst) "ssid=" ssid "&pwd=" pwd "&ntp=" ntp "&tz=" STR(tz) "&dst=" STR(dst)

#define FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define ASSERT_WITH_MESSAGE(a, b, msg)                                                                                 \
    do {                                                                                                               \
        if ((a) != (b)) {                                                                                              \
            printf("ASSERT FAILED: %s:%d: %s\n", FILENAME, __LINE__, (msg));                                           \
            return 1;                                                                                                  \
        } else if (mock_ctx.config.test_verbose) {                                                                     \
            printf("ASSERT OK: %s:%d: %s\n", FILENAME, __LINE__, (msg));                                               \
        }                                                                                                              \
    } while (0)

#define CHECK_ICONS_OK()                                                                                               \
    do {                                                                                                               \
        ASSERT_WITH_MESSAGE(mock_ctx.spy.icon_state.watchdog, WATCHDOG_OK, "Watchdog OK");                             \
        ASSERT_WITH_MESSAGE(mock_ctx.spy.icon_state.ntp, NTP_OK, "NTP  OK");                                           \
        ASSERT_WITH_MESSAGE(mock_ctx.spy.icon_state.wifi, WIFI_OK, "Wi-Fi OK");                                        \
    } while (0)
#define CHECK_WATCHDOG_RESET_OK()                                                                                      \
    do {                                                                                                               \
        ASSERT_WITH_MESSAGE(mock_ctx.spy.icon_state.watchdog, WATCHDOG_RESET, "Watchdog reset");                       \
        ASSERT_WITH_MESSAGE(mock_ctx.spy.icon_state.ntp, NTP_OK, "NTP  OK");                                           \
        ASSERT_WITH_MESSAGE(mock_ctx.spy.icon_state.wifi, WIFI_OK, "Wi-Fi OK");                                        \
    } while (0)
#define CHECK_ICONS_FAIL(N, WATCHDOG, NTP, WIFI)                                                                       \
    do {                                                                                                               \
        ASSERT_WITH_MESSAGE(mock_ctx.spy.icon_state.watchdog, WATCHDOG, "Watchdog fail status");                       \
        ASSERT_WITH_MESSAGE(mock_ctx.spy.icon_state.ntp, NTP, "NTP  fail status");                                     \
        ASSERT_WITH_MESSAGE(mock_ctx.spy.icon_state.wifi, WIFI, "Wi-Fi fail status");                                  \
    } while (0)
#define EXPECT_FATAL_NTP_ERROR(ERROR)                                                                                  \
    do {                                                                                                               \
        CHECK_ICONS_FAIL(0, WATCHDOG_NTP, (ERROR), WIFI_OK);                                                           \
        ASSERT_WITH_MESSAGE(mock_ctx.spy.fatal_reset_caught, 1, "Fatal reset caught");                                 \
        ASSERT_WITH_MESSAGE(mock_ctx.spy.fatal_wifi_error, WIFI_OK, "Wi-Fi OK");                                       \
        ASSERT_WITH_MESSAGE(mock_ctx.spy.fatal_ntp_error, ERROR, "NTP error type");                                    \
    } while (0)
#define EXPECT_FATAL_WIFI_ERROR(ERROR)                                                                                 \
    do {                                                                                                               \
        CHECK_ICONS_FAIL(0, WATCHDOG_WIFI, NTP_OK, (ERROR));                                                           \
        ASSERT_WITH_MESSAGE(mock_ctx.spy.fatal_reset_caught, 1, "Fatal reset caught");                                 \
        ASSERT_WITH_MESSAGE(mock_ctx.spy.fatal_wifi_error, ERROR, "Wi-Fi error type");                                 \
    } while (0)

#define EXPECT_OK 0
#define EXPECT_FAIL 1
#define EXECUTE_TEST_WRAPPER(test_func, msg, status)                                                                   \
    do {                                                                                                               \
        if (test_func != status) {                                                                                     \
            test_printf("CHECK: TEST FAIL: " msg ", %s:%d\n", FILENAME, __LINE__);                                     \
            return 1;                                                                                                  \
        } else {                                                                                                       \
            test_printf("CHECK: TEST OK: " msg "\n");                                                                  \
        }                                                                                                              \
    } while (0)
#define EXECUTE_TEST(msg, status) EXECUTE_TEST_WRAPPER(test_main(), msg, status)
#define EXECUTE_AP_TEST(msg, status)                                                                                   \
    EXECUTE_TEST_WRAPPER(start_wifi_access_point(clock_state, mock_store_config), msg, status)

#define EXPECT_LCD_DIGITS(str)                                                                                         \
    do {                                                                                                               \
        if (strncmp(clock_state->current_lcd_digits, str, 7) != 0) {                                                   \
            printf("ASSERT FAILED: LCD state is %s not %s\n", clock_state->current_lcd_digits, str);                   \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

static int run_test(test_func_t func, const char *test_name)
{
    // Always init the shared test context.
    memset(&mock_ctx, 0, sizeof(mock_ctx));
    mock_ctx.logs.buffer = (char **)calloc(sizeof(char *), LOG_BUFFER_SIZE);
    mock_ctx.logs.buffer_size = 0;
    mock_ctx.config.test_verbose = test_verbose;
    mock_ctx.config.udp_port = TEST_NTP_PORT;

    test_printf("Starting test '%s'\n", test_name);

    // Run test
    int status = func();

    for (unsigned int ii = 0; ii <= mock_ctx.logs.buffer_size; ii++) {
        free(mock_ctx.logs.buffer[ii]);
    }
    free(mock_ctx.logs.buffer);

    if (mock_ctx.leak_checker.allocs != mock_ctx.leak_checker.frees) {
        printf("LEAK DETECTED: %u allocs vs %u frees in %s\n", mock_ctx.leak_checker.allocs,
               mock_ctx.leak_checker.frees, test_name);
        return 1;
    }

    printf("TEST %s: %s\n", test_name, (status == 1) ? "FAIL" : "OK");
    return status;
}

static int test_bad_lcd1(void)
{
    mock_ctx.spy.calloc_counter = 0;
    mock_ctx.inject.calloc_fail_at = 1;
    EXECUTE_TEST("LCD alloc (1)", EXPECT_FAIL);
    ASSERT_WITH_MESSAGE(mock_ctx.spy.clock_state_alloc_failed, 1, "Clock init failed");

    mock_ctx.spy.calloc_counter = 0;
    mock_ctx.inject.calloc_fail_at = 3;
    EXECUTE_TEST("LCD alloc (2)", EXPECT_FAIL);
    ASSERT_WITH_MESSAGE(mock_ctx.spy.lcd_init_failed, 1, "LCD init failed");

    mock_ctx.inject.calloc_fail_at = 0;
    return 0;
}

static int test_dns_lookups(void)
{
    RESET_MOCK_CONFIG();
    mock_ctx.inject.dns_bad_arg = 1;
    EXECUTE_TEST("DNS bad arg", EXPECT_FAIL);
    EXPECT_FATAL_NTP_ERROR(NTP_DNS_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.dns_lookup_fail = 1;
    EXECUTE_TEST("DNS lookup failure", EXPECT_FAIL);
    EXPECT_FATAL_NTP_ERROR(NTP_DNS_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.dns_latency_ms = TEST_TIMEOUT + 1000;
    mock_ctx.inject.dns_lookup_fail = 0;
    EXECUTE_TEST("DNS timeout", EXPECT_FAIL);
    EXPECT_FATAL_NTP_ERROR(NTP_TIMEOUT_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.dns_latency_ms = TEST_TIMEOUT - 1000;
    mock_ctx.inject.exit_on_ntp_success = 1;
    EXECUTE_TEST("DNS delay", EXPECT_OK);
    CHECK_ICONS_OK();

    return 0;
}

static int test_wifi_errors(void)
{
    RESET_MOCK_CONFIG();
    mock_ctx.inject.cyw43_arch_init_fail = 1;
    EXECUTE_TEST("Wi-Fi init failure", EXPECT_FAIL);
    EXPECT_FATAL_WIFI_ERROR(WIFI_INIT_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.cyw43_arch_wifi_connect_status = PICO_ERROR_CONNECT_FAILED;
    EXECUTE_TEST("Wi-Fi connect failure", EXPECT_FAIL);
    EXPECT_FATAL_WIFI_ERROR(WIFI_CONNECT_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.cyw43_arch_wifi_connect_status = -99;
    EXECUTE_TEST("Wi-Fi status failure", EXPECT_FAIL);
    EXPECT_FATAL_WIFI_ERROR(WIFI_UNKNOWN_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.cyw43_auth_error_count = WIFI_BAD_AUTH_RETRY_COUNT;
    EXECUTE_TEST("Wi-Fi excess auth errors", EXPECT_FAIL);
    EXPECT_FATAL_WIFI_ERROR(WIFI_AUTH_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.cyw43_auth_error_count = WIFI_BAD_AUTH_RETRY_COUNT - 1;
    mock_ctx.inject.exit_on_ntp_success = 1;
    EXECUTE_TEST("Wi-Fi auth errors OK", EXPECT_OK);
    CHECK_ICONS_OK();

    RESET_MOCK_CONFIG();
    mock_ctx.inject.cyw43_auth_timeout_count = 99;
    EXECUTE_TEST("Wi-Fi excess timeout", EXPECT_FAIL);
    EXPECT_FATAL_WIFI_ERROR(WIFI_TIMEOUT_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.cyw43_auth_timeout_count = (WIFI_ABANDON_TIMEOUT_MS / WIFI_CONNECT_TIMEOUT_MS) - 1;
    mock_ctx.inject.exit_on_ntp_success = 1;
    EXECUTE_TEST("Wi-Fi OK timeout", EXPECT_OK);
    CHECK_ICONS_OK();

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

    clock_state->ntp_last_sync = t;
    clock_state->ntp_interval = NTP_SYNC_INTERVAL_SEC;
    mock_ctx.spy.system_time_ms = t * 1000;
    mock_ctx.spy.ntp_seconds = (mock_ctx.spy.system_time_ms / 1000) + NTP_DELTA;
    mock_ctx.sim.timer_next_fire = mock_ctx.spy.system_time_ms + 1000;
}

static clock_state_t *create_test_clock_state(repeating_timer_t *timer, clock_config_t *clock_config)
{
    clock_state_t *clock_state = (clock_state_t *)calloc(1, sizeof(clock_state_t));
    for (uint16_t ii = 0; ii < NUM_LCDS; ii++) {
        clock_state->lcd_states[ii] = lcd_init(ii, 0);
    }
    clock_state->ntp_state = ntp_init((void *)clock_state, ntp_timer_callback);
    clock_state->ntp_last_sync = mock_time(NULL);
    clock_state->ntp_time = clock_state->ntp_last_sync;
    clock_state->ntp_state->ntp_port = TEST_NTP_PORT;
    clock_state->first_clock_tick = 0;
    timer->user_data = clock_state;

    memcpy(&clock_state->clock_config, clock_config, sizeof(clock_config_t));
    memcpy(flash_clock_config, clock_config, sizeof(clock_config_t));

    add_repeating_timer_ms(1000, clock_timer_callback, clock_state, timer);

    return clock_state;
}

static void free_test_clock_state(clock_state_t *state)
{
    // LCD/NTP state is allocated inside the clock so the leak counters are active
    mock_ctx.leak_checker.frees++;
    free(state->ntp_state);
    for (unsigned int ii = 0; ii < NUM_LCDS; ii++)
        if (state->lcd_states[ii]) {
            mock_ctx.leak_checker.frees++;
            free(state->lcd_states[ii]);
        }
    free(state);
}

static clock_config_t *create_clock_config(void)
{
    static clock_config_t clock_config = {.magic_marker = CONFIG_MAGIC,
                                          .wifi_ssid = TEST_AP_SSID,
                                          .wifi_password = TEST_AP_PASSWORD,
                                          .tz_offset_mins = TEST_TZ_OFFSET,
                                          .dst_rule = (dst_rule_t)TEST_DST_RULE,
                                          .ntp_timeout = TEST_TIMEOUT,
                                          .ntp_port = TEST_NTP_PORT};
    return &clock_config;
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
    clock_state_t *clock_state = create_test_clock_state(timer, create_clock_config());

    // Timezone Offset Tests (DST_RULE_NONE)`
    clock_state->ntp_last_sync = mock_time(NULL);
    clock_state->ntp_interval = NTP_SYNC_INTERVAL_SEC;

    // Test Positive Fractional Offset: India (+5:30 -> 330 mins)
    clock_state->clock_config.tz_offset_mins = 330;
    set_localtime(clock_state, 2024, 0, 1, 12, 0, 0); // Jan 1, 12:00 UTC
    (void)clock_timer_callback(timer);
    clock_task(clock_state);
    EXPECT_LCD_DIGITS("Mon1730");
    CHECK_ICONS_OK();

    // Test Negative Offset: New York (-5:00 -> -300 mins)
    clock_state->clock_config.tz_offset_mins = -300;
    set_localtime(clock_state, 2024, 0, 1, 12, 0, 0); // Jan 1, 12:00 UTC
    (void)clock_timer_callback(timer);
    clock_task(clock_state);
    EXPECT_LCD_DIGITS("Mon0700");
    CHECK_ICONS_OK();

    // Display Rollover Tests
    clock_state->clock_config.tz_offset_mins = 0; // Reset to UK base
    clock_state->clock_config.dst_rule = DST_RULE_EU;

    // Sun March 25, 2001 at 00:22 (just before EU clocks change)
    set_localtime(clock_state, 2001, 2, 25, 0, 22, 0);
    (void)clock_timer_callback(timer);
    clock_task(clock_state);
    EXPECT_LCD_DIGITS("Sun0022");
    CHECK_ICONS_OK();

    // Sun March 25, 2001 at 01:22 (just after EU clocks change)
    set_localtime(clock_state, 2001, 2, 25, 1, 22, 0);
    (void)clock_timer_callback(timer);
    clock_task(clock_state);
    EXPECT_LCD_DIGITS("Sun0222");
    CHECK_ICONS_OK();

    // Thu August 23, 2001 at 23:55 (test day rollover in DST)
    set_localtime(clock_state, 2001, 7, 23, 23, 55, 0);
    (void)clock_timer_callback(timer);
    clock_task(clock_state);
    EXPECT_LCD_DIGITS("Fri0055");
    CHECK_ICONS_OK();

    const char *test_time = time_as_string(mock_time(NULL), clock_state);
    ASSERT_WITH_MESSAGE(strncmp(test_time, "00:55:00 (DST)", 12), 0, "Check time as string");

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

    // NZ: Last Sun Sep (02:00) to 1st Sun Apr (03:00) -> 2024: Sep 29, Apr 7
    TEST_DST_BOUND(2024, 8, 29, 1, 59, DST_RULE_NZ, 0); // Spring: Before Sep start
    TEST_DST_BOUND(2024, 8, 29, 2, 0, DST_RULE_NZ, 1);  // Spring: DST starts
    TEST_DST_BOUND(2024, 3, 7, 2, 59, DST_RULE_NZ, 1);  // Autumn: Before Apr end
    TEST_DST_BOUND(2024, 3, 7, 3, 0, DST_RULE_NZ, 0);   // Autumn: DST ends

    // --- No DST ---
    TEST_DST_BOUND(2024, 0, 15, 12, 0, DST_RULE_NONE, 0); // January 15th (Mid-winter/summer)
    TEST_DST_BOUND(2024, 5, 15, 12, 0, DST_RULE_NONE, 0); // June 15th (Mid-summer/winter)

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

    free(timer);
    free_test_clock_state(clock_state);

    return 0; // All paths passed
}

static int lcd_digits_to_int(const char *digits)
{
    return ((*digits - '0') * 10) + (*(digits + 1) - '0');
}

static int test_ntp_time(void)
{
    repeating_timer_t *timer = (repeating_timer_t *)calloc(1, sizeof(repeating_timer_t));
    clock_state_t *clock_state = create_test_clock_state(timer, create_clock_config());

    // Tue January 9, 2001 at 09:28:32
    set_localtime(clock_state, 2001, 0, 9, 9, 28, 32);

    // Run for 12 simulated days
    // +1d: normal NTP update
    // +2d: generate a KoD
    // +3d: should not update NTP
    // +4d: normal NTP update
    // +6d: create a DNS error
    // +7d: watchdog error should clear
    // +8d: normal NTP update (NTP error should clear)
    for (int day = 0; day <= 12; day++) {
        switch (day) {
            case 2:
                mock_ctx.inject.udp_response_type = UDP_NTP_KOD;
                break;
            case 3:
                mock_ctx.inject.udp_response_type = UDP_NTP_OK;
                break;
            case 6:
                mock_ctx.inject.dns_lookup_fail = 1;
                mock_ctx.inject.fatal_reset_no_longjmp = 1;
                clock_state->watchdog_reset_error = WATCHDOG_NTP;
                clock_state->last_watchdog_error_time =
                    (time_t)(mock_ctx.spy.system_time_ms / 1000) + NTP_SYNC_INTERVAL_SEC / 2;
                break;
            case 7:
                mock_ctx.inject.dns_lookup_fail = 0;
                break;
            default:
                break;
        }

        for (int second = 0; second < 24 * 60 * 60; second++) {
            sleep_ms(1000);

            time_t now = (time_t)(mock_ctx.spy.system_time_ms / 1000);
            struct tm *test_time = gmtime(&now);
            clock_task(clock_state);

            if (clock_state->current_lcd_digits[3]) {
                // Only check once the LCD has started getting time updates
                int lcd_hour = lcd_digits_to_int(&clock_state->current_lcd_digits[3]);
                int lcd_min = lcd_digits_to_int(&clock_state->current_lcd_digits[5]);

                if (test_time->tm_hour != lcd_hour || test_time->tm_min != lcd_min) {
                    test_printf("FAILED clock state: time=%02d:%02d, lcd=%02d:%02d\n", test_time->tm_hour,
                                test_time->tm_min, lcd_hour, lcd_min);
                }
            }

            mock_ctx.spy.ntp_seconds++;
        }

        switch (day) {
            case 1:
                ASSERT_WITH_MESSAGE(mock_ctx.spy.ntp_packet_sent, 1, "NTP updated");
                mock_ctx.spy.ntp_packet_sent = 0;
                break;
            case 2:
                ASSERT_WITH_MESSAGE(mock_ctx.spy.ntp_packet_sent, 1, "NTP updated");
                ASSERT_WITH_MESSAGE(clock_state->ntp_interval, NTP_SYNC_INTERVAL_SEC * 2, "Interval doubled via KoD");
                mock_ctx.spy.ntp_packet_sent = 0;
                break;
            case 3:
                ASSERT_WITH_MESSAGE(mock_ctx.spy.ntp_packet_sent, 0, "NTP not updated");
                break;
            case 6:
                ASSERT_WITH_MESSAGE(mock_ctx.spy.icon_state.watchdog, WATCHDOG_NTP, "Watchdog fired for NTP");
                break;
            case 7:
                ASSERT_WITH_MESSAGE(clock_state->watchdog_reset_error, WATCHDOG_OK, "watchdog cleared");
                ASSERT_WITH_MESSAGE(mock_ctx.spy.icon_state.ntp, NTP_DNS_ERROR, "NTP  OK");
                break;
            default:
                // CHECK_ICONS_OK();
                break;
        }

        // Simulate the Pico's hardware clock drifting from reality
        if (day == 7) {
            mock_ctx.spy.ntp_seconds += 100;
        }
    }

    mock_ctx.inject.udp_response_type = UDP_NTP_OK;

    // Thu February 7, 2036 at 06:28:59 (NTP Epoch 1)
    set_localtime(clock_state, 2036, 2, 7, 6, 28, 59);
    clock_state->ntp_last_sync = (time_t)(mock_ctx.spy.system_time_ms / 1000) - clock_state->ntp_interval;
    sleep_ms(1000);
    clock_task(clock_state);
    EXPECT_LCD_DIGITS("Fri0629");

    free(timer);
    free_test_clock_state(clock_state);

    return 0;
}

static int test_ntp_errors(void)
{
    RESET_MOCK_CONFIG();
    mock_ctx.inject.udp_response_type = UDP_NTP_INVALID;
    EXECUTE_TEST("NTP UDP invalid", EXPECT_FAIL);
    EXPECT_FATAL_NTP_ERROR(NTP_PROTOCOL_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.udp_response_type = UDP_NTP_BAD_LEN;
    EXECUTE_TEST("NTP UDP bad length failure", EXPECT_FAIL);
    EXPECT_FATAL_NTP_ERROR(NTP_PROTOCOL_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.udp_response_type = UDP_NTP_BAD_PORT;
    EXECUTE_TEST("NTP UDP bad port failure", EXPECT_FAIL);
    EXPECT_FATAL_NTP_ERROR(NTP_PROTOCOL_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.udp_response_type = UDP_NTP_LEAP3;
    EXECUTE_TEST("NTP unsynchronized", EXPECT_FAIL);
    EXPECT_FATAL_NTP_ERROR(NTP_PROTOCOL_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.udp_new_ip_type_fail = 1;
    EXECUTE_TEST("NTP UDP new IP failure", EXPECT_FAIL);
    EXPECT_FATAL_NTP_ERROR(NTP_INIT_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.pbuf_alloc_fail_at = 1;
    EXECUTE_TEST("NTP pbuf alloc failure", EXPECT_FAIL);
    EXPECT_FATAL_NTP_ERROR(NTP_MEMORY_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.udp_sendto_fail = 1;
    EXECUTE_TEST("NTP sendto failure", EXPECT_FAIL);
    EXPECT_FATAL_NTP_ERROR(NTP_PROTOCOL_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.udp_invalid_addr = 1;
    EXECUTE_TEST("NTP invalid address", EXPECT_FAIL);
    EXPECT_FATAL_NTP_ERROR(NTP_DNS_ERROR);

    RESET_MOCK_CONFIG();
    mock_ctx.inject.calloc_fail_at = 9; // Clock, 7x LCD, fail on NTP
    EXECUTE_TEST("NTP calloc failure", EXPECT_FAIL);
    EXPECT_FATAL_NTP_ERROR(NTP_INIT_ERROR);

    return 0;
}

static int test_watchdog(void)
{
    RESET_MOCK_CONFIG();
    mock_ctx.inject.exit_on_ntp_success = 1;
    EXECUTE_TEST("Watchdog init OK", EXPECT_OK);
    CHECK_ICONS_OK();

    RESET_MOCK_CONFIG();
    mock_ctx.inject.watchdog_caused_reboot = 1;
    mock_ctx.inject.exit_on_ntp_success = 1;
    EXECUTE_TEST("Watchdog reboot OK", EXPECT_OK);
    CHECK_WATCHDOG_RESET_OK();

    // RESET_MOCK_CONFIG();
    // EXECUTE_TEST("Watchdog boot count", 0 || persistent_state.boot_count != 2);
    // CHECK_WATCHDOG_RESET_OK();

    // RESET_MOCK_CONFIG();
    // mock_ctx.inject.cyw43_arch_init_fail = 1;
    // EXECUTE_TEST("Watchdog reboot on Wi-Fi", 1 || !mock_ctx.spy.watchdog_reboot_called);
    // EXPECT_FATAL_WIFI_ERROR(WIFI_INIT_ERROR);

    // RESET_MOCK_CONFIG();
    // mock_ctx.inject.dns_lookup_fail = 1;
    // EXECUTE_TEST("Watchdog reboot on DNS", 1 || !mock_ctx.spy.watchdog_reboot_called);
    // EXPECT_FATAL_NTP_ERROR(NTP_DNS_ERROR);

    // RESET_MOCK_CONFIG();
    // mock_ctx.inject.watchdog_caused_reboot = 1;
    // EXECUTE_TEST("Watchdog DNS OK", EXPECT_OK);
    // CHECK_WATCHDOG_RESET_OK();

    // Coverage on otherwise invalid status debug values
    if ((strcmp(watchdog_error_to_string((watchdog_error_t)0xff), "UNKNOWN_STATUS") != 0) ||
        (strcmp(wifi_error_to_string((wifi_error_t)0xff), "UNKNOWN_STATUS") != 0) ||
        (strcmp(ntp_error_to_string((ntp_error_t)0xff), "UNKNOWN_STATUS") != 0)) {
        return 1;
    }
    return 0;
}

static clock_config_t last_parsed_config;
static int config_save_count = 0;

static int mock_store_config(clock_config_t *config, int invalidate)
{
    (void)invalidate;
    memcpy(&last_parsed_config, config, sizeof(clock_config_t));
    config_save_count++;
    return 0;
}

static int mock_store_config_failure(clock_config_t *config)
{
    (void)config;
    return 1;
}

static int simulate_form_post(void *con_state, struct tcp_pcb *pcb, const char *post_body)
{
    config_save_count = 0;
    clear_test_config(con_state);
    memset(&last_parsed_config, 0, sizeof(clock_config_t));

    char headers[256];
    snprintf(headers, sizeof(headers), "POST / HTTP/1.1\r\nContent-Length: %zu\r\n\r\n", strlen(post_body));

    struct pbuf p_head = {0};
    p_head.tot_len = (u16_t)strlen(headers);
    memcpy(p_head.payload, headers, p_head.tot_len);
    tcp_server_recv(con_state, pcb, &p_head, 0);

    struct pbuf p_body = {0};
    p_body.tot_len = (u16_t)strlen(post_body);
    memcpy(p_body.payload, post_body, p_body.tot_len);
    tcp_server_recv(con_state, pcb, &p_body, 0);

    return config_save_count;
}

static int test_wifi_ap_config(void)
{
    repeating_timer_t *timer = (repeating_timer_t *)calloc(1, sizeof(repeating_timer_t));
    clock_state_t *clock_state = create_test_clock_state(timer, create_clock_config());

    RESET_MOCK_CONFIG();
    mock_ctx.inject.calloc_fail_at = 1;
    EXECUTE_AP_TEST("Wi-Fi AP alloc failure", EXPECT_FAIL);

    // RESET_MOCK_CONFIG();
    // mock_ctx.inject.tcp_open_fail = 1;
    // EXECUTE_AP_TEST("Wi-Fi TCP open failure", EXPECT_FAIL);

    RESET_MOCK_CONFIG();
    void *con_state = create_test_config((void *)mock_store_config_failure);
    struct tcp_pcb client_pcb = {0};

    // Flash failure test
    const char *url = TEST_CONFIG_URL("SSID", "password", TEST_NTP_SERVER, TEST_TZ_OFFSET, TEST_DST_RULE);
    if (simulate_form_post(con_state, &client_pcb, url) != 0) {
        test_printf("FAILED: flash failure form failed to submit");
        return 1;
    }
    if (strstr(mock_ctx.inject.tcp_write_buffer, "Failed to save to Flash") == (char *)0) {
        test_printf("FAILED response: got '%s'\n", mock_ctx.inject.tcp_write_buffer);
    }
    free_test_config(con_state);

    // Test happy path (standard fully-populated form)
    con_state = create_test_config((void *)mock_store_config);
    const char *happy_path =
        TEST_CONFIG_URL(TEST_AP_SSID, TEST_AP_PASSWORD, TEST_NTP_SERVER, TEST_TZ_OFFSET, TEST_DST_RULE);
    if (simulate_form_post(con_state, &client_pcb, happy_path) != 1) {
        test_printf("FAILED AP test: Happy path form failed to submit");
        return 1;
    }
    if (strstr(mock_ctx.inject.tcp_write_buffer, "Saved! Rebooting...") == (char *)0) {
        test_printf("FAILED response: got '%s'\n", mock_ctx.inject.tcp_write_buffer);
    }

    ASSERT_WITH_MESSAGE(strcmp(last_parsed_config.wifi_ssid, TEST_AP_SSID), 0, "URL decode SSID");
    ASSERT_WITH_MESSAGE(strcmp(last_parsed_config.wifi_password, TEST_AP_PASSWORD), 0, "URL decode password");
    ASSERT_WITH_MESSAGE(strcmp(last_parsed_config.ntp_server, TEST_NTP_SERVER), 0, "URL NTP server");
    ASSERT_WITH_MESSAGE(last_parsed_config.tz_offset_mins, TEST_TZ_OFFSET, "URL TZ offset");
    ASSERT_WITH_MESSAGE(last_parsed_config.dst_rule, TEST_DST_RULE, "URL DST rule");
    ASSERT_WITH_MESSAGE(last_parsed_config.ntp_port, NTP_DEFAULT_PORT, "URL decode NTP port");

    // Stress URL decode
    // ssid: "My Wi-Fi!" (Testing + and %21)
    // pwd:  "a&b=c%d"   (Testing %26, %3D, %25)
    const char *url_encoded = TEST_CONFIG_URL("My+Wi-Fi%21", "a%26b%3Dc%25d", TEST_NTP_SERVER, 0, 0);
    simulate_form_post(con_state, &client_pcb, url_encoded);

    ASSERT_WITH_MESSAGE(strcmp(last_parsed_config.wifi_ssid, "My Wi-Fi!"), 0, "URL decode SSID stress");
    ASSERT_WITH_MESSAGE(strcmp(last_parsed_config.wifi_password, "a&b=c%d"), 0, "URL decode password stress");

    // Buffer overflow tests
    const char *overflow = "ssid=1234567890123456789012345678901234567890"                                // 40 chars
                           "&pwd=1234567890123456789012345678901234567890123456789012345678901234567890"; // 70 chars

    simulate_form_post(con_state, &client_pcb, overflow);

    // Assert strictly null-terminated and truncated correctly
    if (strlen(last_parsed_config.wifi_ssid) > 32)
        return 1;
    if (strlen(last_parsed_config.wifi_password) > 63)
        return 1;
    if (strncmp(last_parsed_config.wifi_ssid, "12345678901234567890123456789012", 32) != 0)
        return 1;

    // Exhaustive DST Enumeration Mapping
    simulate_form_post(con_state, &client_pcb, "dst=EU");
    if (last_parsed_config.dst_rule != DST_RULE_EU)
        return 1;
    simulate_form_post(con_state, &client_pcb, "dst=AU");
    if (last_parsed_config.dst_rule != DST_RULE_AU)
        return 1;
    simulate_form_post(con_state, &client_pcb, "dst=NZ");
    if (last_parsed_config.dst_rule != DST_RULE_NZ)
        return 1;
    simulate_form_post(con_state, &client_pcb, "dst=CL");
    if (last_parsed_config.dst_rule != DST_RULE_CL)
        return 1;
    simulate_form_post(con_state, &client_pcb, "dst=IL");
    if (last_parsed_config.dst_rule != DST_RULE_IL)
        return 1;

    // Invalid / Catch-all cases must default to NONE
    simulate_form_post(con_state, &client_pcb, "dst=GARBAGE");
    if (last_parsed_config.dst_rule != DST_RULE_NONE)
        return 1;
    simulate_form_post(con_state, &client_pcb, "dst=");
    if (last_parsed_config.dst_rule != DST_RULE_NONE)
        return 1;

    // Malformed data and missing fields
    simulate_form_post(con_state, &client_pcb, "ssid=&pwd=&tz=");
    if (strlen(last_parsed_config.wifi_ssid) != 0)
        return 1;
    if (last_parsed_config.tz_offset_mins != 0)
        return 1;

    free(timer);
    free_test_clock_state(clock_state);
    free_test_config(con_state);

    return 0;
}

static int test_wifi_ap_limits(void)
{
    // 1. Initialize the mock server state
    tcp_server_t mock_server = {0};

    // 2. Simulate 5 parallel TCP handshakes from a smartphone browser
    struct tcp_pcb mock_pcbs[TCP_IP_MAX_CONNECTIONS];
    int accepted_count = 0;
    int rejected_count = 0;

    for (int i = 0; i < TCP_IP_MAX_CONNECTIONS + 2; i++) {
        // Assume tcp_server_accept is exposed or tested directly
        err_t result = tcp_server_accept(&mock_server, &mock_pcbs[i], ERR_OK);
        if (result == ERR_OK) {
            mock_free(mock_ctx.sim.tcp_arg);
            accepted_count++;
        } else if (result == ERR_ABRT) {
            rejected_count++;
        }
    }

    // Assert that the gatekeeper protected the heap
    ASSERT_WITH_MESSAGE(accepted_count, TCP_IP_MAX_CONNECTIONS, "Gatekeeper failed: allowed too many connections!");
    ASSERT_WITH_MESSAGE(rejected_count, 2, "Gatekeeper failed: did not abort excess connections!");

    tcp_connect_state_t con_state = {0};

    // Create a massive fake pbuf chain (e.g., 3000 bytes of 'A')
    struct pbuf mock_pbuf;
    char massive_payload[TCP_IP_BUFFER_SIZE * 2];
    memset(massive_payload, 'A', sizeof(massive_payload));
    massive_payload[TCP_IP_BUFFER_SIZE * 2 - 1] = '\0';

    mock_pbuf.tot_len = TCP_IP_BUFFER_SIZE * 2;
    memcpy(mock_pbuf.payload, massive_payload, TCP_IP_BUFFER_SIZE);

    err_t result = tcp_server_recv(&con_state, /*pcb=*/NULL, &mock_pbuf, ERR_OK);

    ASSERT_WITH_MESSAGE(result, ERR_OK, "Large payload handled");

    // Assert that the buffer capped at our new 1024 byte limit (minus null terminator)
    ASSERT_WITH_MESSAGE(strlen(con_state.headers), TCP_IP_BUFFER_SIZE - 1,
                        "Buffer overflowed or truncated   incorrectly!");

    return 0;
}

// ============================================================================
// Wi-Fi & AP TCP-IP Mock Config & State
// ============================================================================

static int store_calls = 0;
static clock_config_t last_config;

static int mock_store_config_success(clock_config_t *config, int invalidate)
{
    (void)invalidate;
    store_calls++;
    last_config = *config;
    return 0; // Return success
}

static int mock_store_config_fail_then_success(clock_config_t *config, int invalidate)
{
    (void)invalidate;
    store_calls++;
    if (store_calls == 1) {
        return -1; // Simulate a Flash write failure on first try
    }
    last_config = *config;
    return 0; // Succeed on subsequent tries
}

static void reset_wifi_test_state(void)
{
    RESET_MOCK_CONFIG();
    store_calls = 0;
    memset(&last_config, 0, sizeof(last_config));
    mock_clear_tcp_payloads();
}

// ============================================================================
// Access Point (AP) & TCP Server Tests
// ============================================================================

static int test_ap_init_fails_cyw43(void)
{
    reset_wifi_test_state();
    mock_ctx.inject.cyw43_arch_init_fail = 1;
    clock_state_t state = {0};

    wifi_error_t err = start_wifi_access_point(&state, mock_store_config_success);
    ASSERT_WITH_MESSAGE(err, WIFI_INIT_ERROR, "Should fail when CYW43 init fails");
    return 0;
}

static int test_ap_init_fails_calloc(void)
{
    reset_wifi_test_state();
    // Fail the first calloc which allocates the tcp_server_t state
    mock_ctx.inject.calloc_fail_at = 1;
    clock_state_t state = {0};

    wifi_error_t err = start_wifi_access_point(&state, mock_store_config_success);
    ASSERT_WITH_MESSAGE(err, WIFI_INIT_ERROR, "Should fail when state calloc fails");
    return 0;
}

static int test_ap_wifi_already_initialized(void)
{
    reset_wifi_test_state();
    mock_queue_tcp_payload("POST / HTTP/1.1\r\nContent-Length: 10\r\n\r\nssid=Hello");

    clock_state_t state = {0};
    state.wifi_is_initialized = 1;

    // If it attempts to initialize cyw43 again, it would fail.
    // This verifies the short-circuit logic works.
    mock_ctx.inject.cyw43_arch_init_fail = 1;

    wifi_error_t err = start_wifi_access_point(&state, mock_store_config_success);
    ASSERT_WITH_MESSAGE(err, WIFI_OK, "Should bypass init if already initialized");
    return 0;
}

static int test_ap_success_full_post(void)
{
    reset_wifi_test_state();
    const char *post_req = "POST / HTTP/1.1\r\n"
                           "Content-Length: 75\r\n"
                           "\r\n"
                           "ssid=MyNetwork&pwd=MyPassword&ntp=pool.ntp.org&tz=-420&dst=NA&cto=5000&port=123";

    mock_queue_tcp_payload(post_req);

    clock_state_t state = {0};
    wifi_error_t err = start_wifi_access_point(&state, mock_store_config_success);

    ASSERT_WITH_MESSAGE(err, WIFI_OK, "AP should start successfully");
    ASSERT_WITH_MESSAGE(store_calls, 1, "Config should be stored exactly once");
    ASSERT_WITH_MESSAGE(strcmp(last_config.wifi_ssid, "MyNetwork"), 0, "SSID string parsing failed");
    ASSERT_WITH_MESSAGE(strcmp(last_config.wifi_password, "MyPassword"), 0, "Password string parsing failed");
    ASSERT_WITH_MESSAGE(strcmp(last_config.ntp_server, "pool.ntp.org"), 0, "NTP string parsing failed");
    ASSERT_WITH_MESSAGE(last_config.tz_offset_mins, -420, "Timezone integer cast failed");
    ASSERT_WITH_MESSAGE(last_config.dst_rule, DST_RULE_NA, "DST rule mapping failed");
    ASSERT_WITH_MESSAGE(last_config.ntp_timeout, 5000, "Timeout integer cast failed");
    ASSERT_WITH_MESSAGE(last_config.ntp_port, 123, "Port integer cast failed");

    return 0;
}

static int test_ap_url_decoding(void)
{
    reset_wifi_test_state();

    // The + should decode to a space, %20 should decode to a space, %2E to a dot
    const char *post_req = "POST / HTTP/1.1\r\n"
                           "Content-Length: 46\r\n"
                           "\r\n"
                           "ssid=My+Network&pwd=My%20Password&ntp=a%2Eb.com";

    mock_queue_tcp_payload(post_req);

    clock_state_t state = {0};
    wifi_error_t err = start_wifi_access_point(&state, mock_store_config_success);

    ASSERT_WITH_MESSAGE(err, WIFI_OK, "AP should handle decoding successfully");
    ASSERT_WITH_MESSAGE(strcmp(last_config.wifi_ssid, "My Network"), 0, "Failed to decode + as space");
    ASSERT_WITH_MESSAGE(strcmp(last_config.wifi_password, "My Password"), 0, "Failed to decode %20 as space");
    ASSERT_WITH_MESSAGE(strcmp(last_config.ntp_server, "a.b.com"), 0, "Failed to decode %2E as dot");

    return 0;
}

// static int test_ap_url_decode_malformed(void)
// {
//     reset_wifi_test_state();
//     // %ZZ is invalid hex. `hex_to_int` returns 0, terminating the string early.
//     mock_queue_tcp_payload("POST / HTTP/1.1\r\nContent-Length: 13\r\n\r\nssid=Abc%ZZd");

//     clock_state_t state = {0};
//     wifi_error_t err = start_wifi_access_point(&state, mock_store_config_success);

//     ASSERT_WITH_MESSAGE(err, WIFI_OK, "AP should handle malformed hex strings");
//     ASSERT_WITH_MESSAGE(strcmp(last_config.wifi_ssid, "Abc"), 0, "Should terminate safely at invalid hex");

//     return 0;
// }

static int test_ap_dst_rule_fallback(void)
{
    reset_wifi_test_state();
    // Providing an unknown DST rule string should fall back to DST_RULE_NONE
    mock_queue_tcp_payload("POST / HTTP/1.1\r\nContent-Length: 14\r\n\r\nssid=A&dst=FOO");
    clock_state_t state = {0};
    wifi_error_t err = start_wifi_access_point(&state, mock_store_config_success);

    ASSERT_WITH_MESSAGE(err, WIFI_OK, "AP should handle unknown DST gracefully");
    ASSERT_WITH_MESSAGE(last_config.dst_rule, DST_RULE_NONE, "Should fallback to DST_RULE_NONE");

    return 0;
}

// static int test_ap_get_config_page(void)
// {
//     reset_wifi_test_state();

//     // 1. GET request for config page
//     mock_queue_tcp_payload("GET /config HTTP/1.1\r\n\r\n");
//     // 2. Queue a valid POST to subsequently break the infinite loop
//     mock_queue_tcp_payload("POST / HTTP/1.1\r\nContent-Length: 9\r\n\r\nssid=Exit");

//     clock_state_t state = {0};
//     wifi_error_t err = start_wifi_access_point(&state, mock_store_config_success);

//     ASSERT_WITH_MESSAGE(err, WIFI_OK, "Should survive GET request and exit on POST");
//     return 0;
// }

// static int test_ap_get_redirect(void)
// {
//     reset_wifi_test_state();

//     // 1. GET request for unknown page (Route C - Captive Portal Redirect)
//     mock_queue_tcp_payload("GET /generate_204 HTTP/1.1\r\n\r\n");
//     // 2. Queue valid POST to exit loop
//     mock_queue_tcp_payload("POST / HTTP/1.1\r\nContent-Length: 9\r\n\r\nssid=Exit");

//     clock_state_t state = {0};
//     wifi_error_t err = start_wifi_access_point(&state, mock_store_config_success);

//     ASSERT_WITH_MESSAGE(err, WIFI_OK, "Should survive redirect request and exit on POST");
//     return 0;
// }

static int test_ap_post_save_fails(void)
{
    reset_wifi_test_state();

    // 1. Valid POST, but we will mock a Flash write failure
    mock_queue_tcp_payload("POST / HTTP/1.1\r\nContent-Length: 9\r\n\r\nssid=Fail");
    // 2. Second attempt succeeds
    mock_queue_tcp_payload("POST / HTTP/1.1\r\nContent-Length: 9\r\n\r\nssid=Pass");

    clock_state_t state = {0};
    wifi_error_t err = start_wifi_access_point(&state, mock_store_config_fail_then_success);

    ASSERT_WITH_MESSAGE(err, WIFI_OK, "Should recover from a failed config store");
    ASSERT_WITH_MESSAGE(store_calls, 2, "Store config should have been called twice");
    // ASSERT_WITH_MESSAGE(strcmp(last_config.wifi_ssid, "Pass"), 0, "Last config stored should be the passing one");

    return 0;
}

// static int test_ap_incomplete_requests(void)
// {
//     reset_wifi_test_state();

//     // 1. Missing double \r\n boundary (Headers incomplete)
//     mock_queue_tcp_payload("GET / HTTP/1.1");
//     // 2. POST with missing body payload
//     mock_queue_tcp_payload("POST / HTTP/1.1\r\nContent-Length: 100\r\n\r\nshort");
//     // 3. Valid POST to exit the loop
//     mock_queue_tcp_payload("POST / HTTP/1.1\r\nContent-Length: 9\r\n\r\nssid=Pass");

//     clock_state_t state = {0};
//     wifi_error_t err = start_wifi_access_point(&state, mock_store_config_success);

//     ASSERT_WITH_MESSAGE(err, WIFI_OK, "Should ignore incomplete packets and wait for valid one");
//     ASSERT_WITH_MESSAGE(store_calls, 1, "Should only store the valid payload");

//     return 0;
// }

static int test_ap_accept_calloc_fails(void)
{
    reset_wifi_test_state();

    // Fail the second calloc attempt (tcp_connect_state_t in tcp_server_accept)
    mock_ctx.inject.calloc_fail_at = 2;

    // Request rejected by tcp_server_accept due to ERR_MEM
    mock_queue_tcp_payload("POST / HTTP/1.1\r\nContent-Length: 9\r\n\r\nssid=Fail");
    // Next payload succeeds (calloc_fail_at resets to 0)
    mock_queue_tcp_payload("POST / HTTP/1.1\r\nContent-Length: 9\r\n\r\nssid=Pass");

    clock_state_t state = {0};
    wifi_error_t err = start_wifi_access_point(&state, mock_store_config_success);

    ASSERT_WITH_MESSAGE(err, WIFI_OK, "Should handle accept ERR_MEM gracefully");
    ASSERT_WITH_MESSAGE(store_calls, 1, "Only the second payload should have triggered a store");

    return 0;
}

static int test_ap_tcp_error_handling(void)
{
    reset_wifi_test_state();

    // Inject a TCP connection reset (ERR_RST) into the event loop at T+50ms
    mock_inject_tcp_error(ERR_RST, 50);
    // Send valid POST that fires after the error to finish the test gracefully
    mock_queue_tcp_payload("POST / HTTP/1.1\r\nContent-Length: 9\r\n\r\nssid=Pass");

    clock_state_t state = {0};
    wifi_error_t err = start_wifi_access_point(&state, mock_store_config_success);

    ASSERT_WITH_MESSAGE(err, WIFI_OK, "tcp_server_err should clean up cleanly without crashing");
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

    status |= run_test(test_bad_lcd1, "LCD1 init error");

    status |= run_test(test_dst, "Daylight savings");

    status |= run_test(test_ntp_time, "NTP time checks");

    status |= run_test(test_wifi_errors, "Wi-Fi init error");

    status |= run_test(test_dns_lookups, "DNS lookups");

    status |= run_test(test_ntp_errors, "NTP errors");

    status |= run_test(test_watchdog, "Watchdog");

    status |= run_test(test_wifi_ap_config, "Wi-Fi AP Config");

    status |= run_test(test_wifi_ap_limits, "Wi-Fi AP limits");

    // ... inside main()

    status |= run_test(test_ap_init_fails_cyw43, "Wi-Fi AP: CYW43 init fails");
    status |= run_test(test_ap_init_fails_calloc, "Wi-Fi AP: Calloc fails during init");
    status |= run_test(test_ap_wifi_already_initialized, "Wi-Fi AP: Short-circuit already initialized");
    status |= run_test(test_ap_success_full_post, "Wi-Fi AP: Successful POST payload processing");
    status |= run_test(test_ap_url_decoding, "Wi-Fi AP: URL encoded payload logic");
    // status |= run_test(test_ap_url_decode_malformed, "Wi-Fi AP: Malformed URL hex decoding");
    status |= run_test(test_ap_dst_rule_fallback, "Wi-Fi AP: DST Rule fallback mapping");
    // status |= run_test(test_ap_get_config_page, "Wi-Fi AP: Handle GET config requests");
    // status |= run_test(test_ap_get_redirect, "Wi-Fi AP: Handle captive portal redirects");
    status |= run_test(test_ap_post_save_fails, "Wi-Fi AP: Flash write failure recovery");
    // status |= run_test(test_ap_incomplete_requests, "Wi-Fi AP: Ignore incomplete TCP streams");
    status |= run_test(test_ap_accept_calloc_fails, "Wi-Fi AP: Recover from tcp_accept ERR_MEM");
    status |= run_test(test_ap_tcp_error_handling, "Wi-Fi AP: Handle TCP reset/abort events");

    return status;
}
