/*
 * SPDX-License-Identifier: MIT
 *
 * Much of this code is derived from the Waveshare 1.47inch LCD Module
 * driver (see https://www.waveshare.com/wiki/1.47inch_LCD_Module).
 *
 * (c) 2022 Waveshare team
 *
 * Additional code is
 *
 * (c) 2025 Jon Connell
 *
 */

#ifndef _LCD_H
#define _LCD_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/* Local includes */
#include "fb.h"
#include "fonts.h"

#define LCD_HEIGHT 172
#define LCD_WIDTH 320
#define BGCOLOR 0x00
#define FGCOLOR 0xff
#define TEXT_FONT Roboto_Medium24
#define TEXT_FONT_HEIGHT 24
#define DIGIT_FONT Roboto_Medium200

#define HORIZONTAL 0
#define VERTICAL 1

typedef struct
{
    /* Mapping for the LCD GPIO pins */
    uint16_t RST_gpio;
    uint16_t DC_gpio;
    uint16_t BL_gpio;
    uint16_t CS_gpio;
    uint16_t CLK_gpio;
    uint16_t MOSI_gpio;
    /* Additional state */
    uint16_t width;
    uint16_t height;
    uint8_t scan_dir;
} lcd_config_t;

void lcd_set_backlight(uint8_t level);

void lcd_init_hardware(lcd_config_t *config, bool reset);

typedef struct
{
    uint16_t WIDTH;
    uint16_t HEIGHT;
    uint8_t SCAN_DIR;
} LCD_1IN47_ATTRIBUTES;
extern LCD_1IN47_ATTRIBUTES LCD_1IN47;

void LCD_1IN47_Init(lcd_config_t *config, uint8_t Scan_dir, bool reset);
void LCD_1IN47_Clear(lcd_config_t *config, uint16_t Color);
void LCD_1IN47_DisplayPoint(lcd_config_t *config, uint16_t X, uint16_t Y, uint16_t Color);
void LCD_1IN47_Display_2Bit(lcd_config_t *config, void *Image);

typedef struct lcd_state_t
{
    uint8_t *frame_buffer_data;
    frame_buffer_t *frame_buffer;
    int y_offset;
    lcd_config_t *config;
} lcd_state_t;

extern lcd_state_t *lcd_init(lcd_config_t *config, int lcd_number);

extern void print_line(lcd_state_t *state, const char *format, ...);

extern void clear_screen(lcd_state_t *state);

void update_screen(lcd_state_t *state);

#endif /* _LCD_H */