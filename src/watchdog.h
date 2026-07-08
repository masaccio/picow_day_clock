#pragma once

typedef enum
{
    WATCHDOG_OK,
    WATCHDOG_RESET,
    WATCHDOG_NTP,
    WATCHDOG_WIFI,
} watchdog_error_t;

extern const char *watchdog_error_to_string(watchdog_error_t status);
