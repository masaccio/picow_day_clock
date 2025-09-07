#ifndef __WIFI_H
#define __WIFI_H

#include <stdbool.h>

#define WIFI_CONNECT_TIMEOUT_MS (10 * 1000) // Time to allow Wi-Fi driver to connect
#define WIFI_ABANDON_TIMEOUT_MS (60 * 1000) // How long to keep retrying connection
#define WIFI_BAD_AUTH_RETRY_COUNT 3         // How many bad auth errors to tolerate
#define WIFI_BAD_AUTH_RETRY_DELAY_MS 500    // Time to wait before retrying after auth error

typedef enum
{
    WIFI_STATUS_SUCCESS = 0,
    WIFI_STATUS_INIT_FAIL = -1,
    WIFI_STATUS_TIMEOUT = -2,
    WIFI_STATUS_BAD_AUTH = -3,
    WIFI_STATUS_CONNECT_FAILED = -4,
    WIFI_STATUS_UNKNOWN_ERROR = -5,
} wifi_status_t;

extern wifi_status_t connect_to_wifi(const char ssid[], const char password[]);

extern void disconnect_from_wifi(void);

#endif // __WIFI_H
