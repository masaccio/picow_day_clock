#pragma once

#define LCD_GPIO_CS_BASE (uint16_t)2      // LCD 1 chip select
#define LCD1_GPIO_CS LCD_GPIO_CS_BASE     // LCD 1 chip select
#define LCD2_GPIO_CS LCD_GPIO_CS_BASE + 1 // LCD 2 chip select
#define LCD3_GPIO_CS LCD_GPIO_CS_BASE + 2 // LCD 3 chip select
#define LCD4_GPIO_CS LCD_GPIO_CS_BASE + 3 // LCD 4 chip select
#define LCD5_GPIO_CS LCD_GPIO_CS_BASE + 4 // LCD 5 chip select
#define LCD6_GPIO_CS LCD_GPIO_CS_BASE + 5 // LCD 6 chip select
#define LCD7_GPIO_CS LCD_GPIO_CS_BASE + 6 // LCD 7 chip select
#define FACTORY_RESET_GPIO (uint16_t)9    // Factor reset enable (Wi-Fi AP)
#define LCD_GPIO_CLK (uint16_t)10         // Shared clock
#define LCD_GPIO_MOSI (uint16_t)11        // Shared MOSI (DIN)
#define LCD_GPIO_RST (uint16_t)12         // Shared LCD reset
#define LCD_GPIO_BL (uint16_t)13          // Shared LCD back light enable
#define LCD_GPIO_DC (uint16_t)14          // Shared data/command

#define LCD_COLOR_TABLE {0x0000 /* black */, 0xF800 /* red */, 0x07E0 /* green */, 0x2E9C /* cyan */}

#define FACTORY_RESET_HOLD_TIME_MS 3000       // How long the factory reset button must be held for
#define HTTP_POLL_TIME_SEC 5                  // How long to allow a connection to idle
#define HTTP_TCP_PORT 80                      // Found in the SDK, but define here for portability to the host tests
#define LCD_COL_START 0                       // ST7789 column shift for 1.47" LCD
#define LCD_HEIGHT 320                        // Length of the short edge of the LCD in pixels
#define LCD_ROW_START 34                      // ST7789 row shift for 1.47" LCD
#define LCD_WIDTH 172                         // Length of the long edge of the LCD in pixels
#define NTP_DELTA 2208988800                  // Seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_MSG_LEN 48                        // NTP message size (fixed by lwIP)
#define NTP_PORT 123                          // Default NTP port
#define NTP_SYNC_INTERVAL_SEC (60 * 60 * 24)  // Sync to NTP once per day
#define NUM_LCDS 7                            // Number of LCDs (3 for day of week, 4 for time)
#define TCP_IP_BUFFER_SIZE 8192               // Needs to be large enough for the most bloated of POST responses
#define TCP_IP_MAX_CONNECTIONS 4              // Keep low to avoid RAM exhaustion from browsers making many requests
#define WATCHDOG_ICON_INTERVAL (60 * 60 * 24) // How long to hold a watchdog error icon
#define WATCHDOG_TIMEOUT_MS (5 * 1000)        // Watchdog timeout in milliseconds (HW max is ~8s)
#define WIFI_ABANDON_TIMEOUT_MS (60 * 1000)   // How long to keep retrying connection in milliseconds
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
