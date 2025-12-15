#ifndef CONFIG_H
#define CONFIG_H

#define LCD1_GPIO_DC (uint16_t)6
#define LCD1_GPIO_CS (uint16_t)7
#define LCD2_GPIO_DC (uint16_t)8
#define LCD2_GPIO_CS (uint16_t)9
#define LCD3_GPIO_DC (uint16_t)2
#define LCD3_GPIO_CS (uint16_t)3
#define LCD4_GPIO_DC (uint16_t)4
#define LCD4_GPIO_CS (uint16_t)5
#define LCD5_GPIO_DC (uint16_t)14
#define LCD5_GPIO_CS (uint16_t)15
#define LCD6_GPIO_DC (uint16_t)16
#define LCD6_GPIO_CS (uint16_t)17
#define LCD7_GPIO_DC (uint16_t)18
#define LCD7_GPIO_CS (uint16_t)19
#define LCD_GPIO_RST (uint16_t)12
#define LCD_GPIO_BL (uint16_t)13
#define LCD_GPIO_CLK (uint16_t)10
#define LCD_GPIO_MOSI (uint16_t)11

#define WATCHDOG_TIMEOUT_MS (20 * 1000)     // Watchdog timeout in milliseconds (needs to be longer than timeouts)
#define WIFI_CONNECT_TIMEOUT_MS (10 * 1000) // Time to allow Wi-Fi driver to connect in milliseconds
#define WIFI_ABANDON_TIMEOUT_MS (60 * 1000) // How long to keep retrying connection in milliseconds
#define WIFI_BAD_AUTH_RETRY_COUNT 3         // How many bad auth errors to tolerate
#define WIFI_BAD_AUTH_RETRY_DELAY_MS 500    // Time to wait before retrying after auth error in milliseconds

#ifndef NTP_SERVER
#define NTP_SERVER "pool.ntp.org" // Default NTP server
#endif

#define NTP_MSG_LEN 48                       // NTP message size (fixed by lwIP)
#define NTP_PORT 123                         // Default NTP port
#define NTP_DELTA 2208988800                 // Seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_TIMEOUT_MS (10 * 1000)           // How long to wait for a single NTP response
#define NTP_SYNC_INTERVAL_SEC (60 * 60 * 24) // Sync to NTP once per day

#define WATCHDOG_ICON_INTERVAL NTP_SYNC_INTERVAL_SEC // How long to hold a watchdog error icon

#define LCD_COL_START 0  // ST7789 column shift for 1.47" LCD
#define LCD_ROW_START 34 // ST7789 row shift for 1.47" LCD
#define LCD_HEIGHT 320   // Length of the short edge of the LCD in pixels
#define LCD_WIDTH 172    // Length of the long edge of the LCD in pixels
#define NUM_LCDS 7       // Number of LCDs (3 for day of week, 4 for time)

#define LCD_COLOR_TABLE {0x0000 /* black */, 0xF800 /* red */, 0x07E0 /* green */, 0x2E9C /* cyan */}

#endif // CONFIG_H
