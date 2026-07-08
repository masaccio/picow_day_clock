#pragma once

#include <stdint.h>

// --- LCD Hardware Configuration ---
#define LCD_GPIO_CS_BASE (uint16_t)2      // LCD 1 chip select
#define LCD1_GPIO_CS LCD_GPIO_CS_BASE     // LCD 1 chip select
#define LCD2_GPIO_CS LCD_GPIO_CS_BASE + 1 // LCD 2 chip select
#define LCD3_GPIO_CS LCD_GPIO_CS_BASE + 2 // LCD 3 chip select
#define LCD4_GPIO_CS LCD_GPIO_CS_BASE + 3 // LCD 4 chip select
#define LCD5_GPIO_CS LCD_GPIO_CS_BASE + 4 // LCD 5 chip select
#define LCD6_GPIO_CS LCD_GPIO_CS_BASE + 5 // LCD 6 chip select
#define LCD7_GPIO_CS LCD_GPIO_CS_BASE + 6 // LCD 7 chip select
#define LCD_GPIO_CLK (uint16_t)10         // Shared clock
#define LCD_GPIO_MOSI (uint16_t)11        // Shared MOSI (DIN)
#define LCD_GPIO_RST (uint16_t)12         // Shared LCD reset
#define LCD_GPIO_BL (uint16_t)13          // Shared LCD back light enable
#define LCD_GPIO_DC (uint16_t)14          // Shared data/command
#define LCD_COL_START 0                   // ST7789 column shift for 1.47" LCD
#define LCD_HEIGHT 320                    // Length of the short edge of the LCD in pixels
#define LCD_ROW_START 34                  // ST7789 row shift for 1.47" LCD
#define LCD_WIDTH 172                     // Length of the long edge of the LCD in pixels

#define LCD_COLOR_TABLE {0x0000 /* black */, 0xF800 /* red */, 0x07E0 /* green */, 0x07FF /* cyan */}

// Whilst the display is capable of many more colors, we limit our bitmaps to 4-bit
// color to save memory
typedef enum
{
    BLACK = 0x00,
    RED = 0x01,
    GREEN = 0x02,
    CYAN = 0x03
} color_t;

#define BG_COLOR BLACK
#define FG_COLOR GREEN

typedef struct lcd_state_t
{
    // GPIO config
    uint16_t RST_gpio;
    uint16_t DC_gpio;
    uint16_t BL_gpio;
    uint16_t CS_gpio;
    uint16_t CLK_gpio;
    uint16_t MOSI_gpio;
    // Additional config
    uint16_t width;
    uint16_t height;
} lcd_state_t;

typedef enum
{
    LCD_MSG_INIT_OK,
    LCD_MSG_FLASH_ERROR,
    LCD_MSG_WIFI_ERROR,
    LCD_MSG_WIFI_OK
} lcd_status_message_t;

#define LCD_MSG_STR_INIT_OK "LCD init successful"
#define LCD_MSG_STR_FLASH_ERROR "Flash config corrupt"
#define LCD_MSG_STR_WIFI_ERROR "Connect to Clock Wi-Fi"
#define LCD_MSG_STR_WIFI_OK "Connected to WiFi"
