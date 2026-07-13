#pragma once

#define FACTORY_RESET_GPIO (uint16_t)9        // Factory reset enable (Wi-Fi AP)
#define DIAG_GREEN_LED_GPIO (uint16_t)15      // LED driver for normal operation
#define DIAG_RED_LED_GPIO (uint16_t)16        // LED driver for fatal errors
#define AMBIENT_LIGHT_GPIO (uint16_t)26       // ADC pin for photo-resistor light sensor
#define AMBIENT_LIGHT_ADC_CH (uint16_t)0      // ADC pin 26 is ADC channel 0
#define AMBIENT_LIGHT_DARK (uint16_t)50       // ADC reading when the room is pitch black
#define AMBIENT_LIGHT_BRIGHT (uint16_t)3000   // ADC reading when the room lights are on
#define FACTORY_RESET_HOLD_TIME_MS 3000       // How long the factory reset button must be held for
#define HTTP_POLL_TIME_SEC 5                  // How long to allow a connection to idle
#define HTTP_TCP_PORT 80                      // Found in the SDK, but define here for portability to the host tests
#define NTP_DELTA 2208988800                  // Seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_MSG_LEN 48                        // NTP message size (fixed by lwIP)
#define NTP_DEFAULT_TIMEOUT_MS (10 * 1000)    // Time to allow for an NTP response in milliseconds
#define NTP_SYNC_INTERVAL_SEC (60 * 60 * 24)  // Sync to NTP once per day
#define NTP_DEFAULT_PORT 123                  // Default NTP port
#define NTP_DEFAULT_SERVER "pool.ntp.org"     // Default NPT server
#define NUM_LCDS 7                            // Number of LCDs (3 for day of week, 4 for time)
#define TCP_IP_BUFFER_SIZE 8192               // Needs to be large enough for the most bloated of POST responses
#define TCP_IP_MAX_CONNECTIONS 4              // Keep low to avoid RAM exhaustion from browsers making many requests
#define WATCHDOG_ICON_INTERVAL (60 * 60 * 24) // How long to hold a watchdog error icon
#define WATCHDOG_TIMEOUT_MS (5 * 1000)        // Watchdog timeout in milliseconds (HW max is ~8s)
#define WIFI_ABANDON_TIMEOUT_MS (30 * 1000)   // How long to keep retrying connection in milliseconds
#define WIFI_BAD_AUTH_RETRY_COUNT 3           // How many bad auth errors to tolerate
#define WIFI_BAD_AUTH_RETRY_DELAY_MS 500      // Time to wait before retrying after auth error in milliseconds
#define WIFI_CONNECT_TIMEOUT_MS (10 * 1000)   // Time to allow Wi-Fi driver to connect in milliseconds

#define HTTP_RESPONSE_REDIRECT_HEADER                                                                                  \
    "HTTP/1.1 302 Found\r\n"                                                                                           \
    "Location: http://%s" HTTP_CONFIG_URL "\r\n"                                                                       \
    "Connection: close\r\n"                                                                                            \
    "\r\n"
#define HTTP_RESPONSE_OK_HEADER                                                                                        \
    "HTTP/1.1 200 OK\r\n"                                                                                              \
    "Content-Type: text/html; charset=utf-8\r\n"                                                                       \
    "Connection: close\r\n"                                                                                            \
    "\r\n"
#define HTTP_CONFIG_URL "/clock-config"

#define WIFI_AP_SSID_PREFIX "Pico-Day-Clock-Setup" // Prefix will have a partial MAC address appended
#define WIFI_SSID_MAX_LEN 32                       // IEEE 802.11 standard limits
#define WIFI_PASSWORD_MAX_LEN 63                   // IEEE 802.11 standard limits
#define HOSTNAME_MAX_LEN 64                        // Maximum length for Linux FQDN
