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

// Local includes
#include "fb.h"
#include "font.h"

#define LCD_HEIGHT 172
#define LCD_WIDTH 320
#define NUM_LCDS 7

#define HORIZONTAL 0
#define VERTICAL 1

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
    uint8_t scan_dir;
    // Frame buffer
    frame_buffer_t *fb;
} lcd_state_t;

extern lcd_state_t *lcd_init(uint16_t RST_gpio, uint16_t DC_gpio, uint16_t BL_gpio, uint16_t CS_gpio, uint16_t CLK_gpio,
                             uint16_t MOSI_gpio, bool reset);

void lcd_set_backlight(lcd_state_t *state, uint8_t level);

void lcd_init_peripherals(lcd_state_t *state, bool reset);

extern void lcd_print_line(lcd_state_t *state, uint16_t line_num, color_t color, const char *format, ...);

extern void lcd_print_clock_digit(lcd_state_t *state, color_t color, const char ascii_char);

extern void lcd_clear_screen(lcd_state_t *state, color_t color);

void lcd_update_screen(lcd_state_t *state);

#endif // _LCD_H
