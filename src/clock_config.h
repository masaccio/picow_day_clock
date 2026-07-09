#pragma once

#include <stdint.h>

#include "config.h"
#include "ntp.h"
#include "watchdog.h"
#include "wifi.h"

// Helper to define enum to string functions
#define STATUS_CASE(STATUS)                                                                                            \
    case STATUS:                                                                                                       \
        return #STATUS;

#ifdef CLOCK_DEBUG_ENABLED
#ifdef TEST_MODE
extern int test_printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
#define CLOCK_DEBUG(...) test_printf(__VA_ARGS__)
#else
#define CLOCK_DEBUG(format, ...) printf("CLOCK: " format, ##__VA_ARGS__)
#endif // #if TEST_MODE
#else
#define CLOCK_DEBUG(...) ((void)0)
#endif

// Flash config is real flash on the Pico but we use a global pointer as the
// struct for storing configs (defined in mock.h) for testing on the host.
#define CONFIG_MAGIC ((uint32_t)'C' << 24 | (uint32_t)'L' << 16 | (uint32_t)'O' << 8 | (uint32_t)'K')
#define FLASH_TARGET_OFFSET (1024 * 1024)
#ifndef FLASH_CONFIG_ADDR
#define FLASH_CONFIG_ADDR XIP_BASE + FLASH_TARGET_OFFSET
#endif

typedef struct
{
    uint32_t magic_marker;
    uint32_t boot_count;
    watchdog_error_t watchdog_error;
    ntp_error_t ntp_error;
    wifi_error_t wifi_error;
} persistent_state_t;

typedef enum
{
    DST_RULE_NONE = 0,
    DST_RULE_NA,
    DST_RULE_EU,
    DST_RULE_AU,
    DST_RULE_NZ,
    DST_RULE_CL,
    DST_RULE_IL
} dst_rule_t;

typedef struct clock_config_t
{
    uint32_t magic_marker;
    char wifi_ssid[WIFI_SSID_MAX_LEN + 1];
    char wifi_password[WIFI_PASSWORD_MAX_LEN + 1];
    char ntp_server[HOSTNAME_MAX_LEN + 1];
    int16_t tz_offset_mins;
    dst_rule_t dst_rule;
    uint32_t ntp_timeout;
    uint16_t ntp_port;
    bool led_always_on;
} clock_config_t;

extern persistent_state_t persistent_state;

#ifndef TEST_MODE
#define NO_RETURN_FUNC __attribute__((noreturn))
#else
#define NO_RETURN_FUNC
#endif
