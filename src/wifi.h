#ifndef __WIFI_H
#define __WIFI_H

#define WIFI_CONNECT_TIMEOUT_MS (10 * 1000)
#define WIFI_ABANDON_TIMEOUT_MS (60 * 1000)
#define WIFI_BAD_AUTH_RETRY_COUNT 3
#define WIFI_BAD_AUTH_RETRY_DELAY_MS 500

extern bool connect_to_wifi(const char ssid[], const char password[]);

extern void disconnect_from_wifi();

#endif /* __WIFI_H */
