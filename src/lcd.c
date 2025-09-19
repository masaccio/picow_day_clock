/*
 * SPDX-License-Identifier: MIT
 *
 * Much of this code is derived from the Waveshare 1.47inch LCD Module
 * driver (see https://www.waveshare.com/wiki/1.47inch_LCD_Module).
 *
 * (c) 2022 Waveshare team
 *
 * Additional code:
 *
 * (c) 2025 Jon Connell
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// PICO SDK
#ifndef TEST_MODE
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#else
#include "tests/mock.h"
#endif

// Local includes
#include "bitmap.h"
#include "clock.h"
#include "fb.h"
#include "lcd.h"
#include "status.h"

// How the 2-bit colours map to 16-bit colour
static uint16_t color_table[] = {0x0000 /* black */, 0xF800 /* red */, 0x07E0 /* green */, 0xFFFF /* white */};

// PWM slice attached to the backlight
static uint slice_num;

static void lcd_reset(lcd_state_t *state);
static void st7789_init(lcd_state_t *state);

lcd_state_t *lcd_init(uint16_t RST_gpio, uint16_t DC_gpio, uint16_t BL_gpio, uint16_t CS_gpio, uint16_t CLK_gpio,
                      uint16_t MOSI_gpio, bool reset)

{
    lcd_state_t *state = (lcd_state_t *)calloc(1, sizeof(lcd_state_t));
    if (!state) {
        return NULL;
    }

    state->RST_gpio = RST_gpio;
    state->DC_gpio = DC_gpio;
    state->BL_gpio = BL_gpio;
    state->CS_gpio = CS_gpio;
    state->CLK_gpio = CLK_gpio;
    state->MOSI_gpio = MOSI_gpio;
    state->scan_dir = VERTICAL;

    lcd_init_peripherals(state, reset);
    lcd_set_backlight(state, 90);
    if (reset) {
        // All displays share the same reset line
        lcd_reset(state);
    }

    st7789_init(state);
    lcd_set_backlight(state, 0);

    state->fb = fb_create(LCD_WIDTH, LCD_HEIGHT, 0);
    fb_clear(state->fb, BGCOLOR);
    fb_rotate(state->fb, ROTATE_90);

    lcd_set_backlight(state, 100);

    return state;
}

void lcd_print_line(lcd_state_t *state, uint16_t line_num, color_t color, const char *buffer)
{
    uint16_t y_offset = (line_num * text_font.height) + 2;
    fb_write_string(state->fb, 0, y_offset, buffer, &text_font, /* fgcolor */ color, /* bgcolor */ BLACK);
}

void lcd_print_clock_digit(lcd_state_t *state, color_t color, const char ascii_char)
{
    const font_glyph_t entry = clock_digit_font.table[ascii_char - ' '];
    uint16_t x_point = (state->fb->width - entry.width) / 2;
    (void)fb_write_char(state->fb, x_point, 0, ascii_char, &clock_digit_font, color, BLACK);
}

void lcd_clear_screen(lcd_state_t *state, color_t color)
{
    fb_clear(state->fb, color);
    lcd_update_screen(state);
}

void lcd_update_icon(lcd_state_t *state, clock_status_t status, bool is_error)
{
    const uint8_t *icon;
    int offset;
    color_t color = is_error ? RED : GREEN;
    switch (status) {
        case STATUS_WIFI_OK:
            icon = wifi_icon;
            offset = -1;
            break;

        case STATUS_WIFI_INIT:
            icon = wifi_init_icon;
            offset = -1;
            break;

        case STATUS_WIFI_TIMEOUT:
            icon = wifi_timeout_icon;
            offset = -1;
            break;

        case STATUS_WIFI_AUTH:
            icon = wifi_password_icon;
            offset = -1;
            break;

        case STATUS_WIFI_CONNECT:
            icon = wifi_connect_icon;
            offset = -1;

        case STATUS_WIFI_ERROR:
            icon = wifi_error_icon;
            offset = -1;
            break;

        case STATUS_NTP_OK:
            icon = ntp_icon;
            offset = -2;
            break;

        case STATUS_NTP_INIT:
            icon = ntp_init_icon;
            offset = -2;
            break;

        case STATUS_NTP_DNS:
            icon = ntp_dns_icon;
            offset = -2;
            break;

        case STATUS_NTP_TIMEOUT:
            icon = ntp_timeout_icon;
            offset = -2;
            break;

        case STATUS_NTP_MEMORY:
            icon = ntp_memory_icon;
            offset = -2;
            break;

        case STATUS_NTP_INVALID:
            icon = ntp_error_icon;
            offset = -2;
            break;

        case STATUS_WATCHDOG_RESET:
            icon = watchdog_icon;
            offset = -3;
            break;

        case STATUS_NONE:
            return;
    }

    // Position last icon 5 pixels from edge to handle rounded corner and position all icons 5 pixels apart
    uint16_t x_start = LCD_HEIGHT - 5;
    x_start += ICON_SIZE * offset;
    fb_copy_image(state->fb, icon, x_start, 0, ICON_SIZE, ICON_SIZE, color);
}

// Initialise the Pico peripeherals we will use (SPI, GPIO, PWM)
void lcd_init_peripherals(lcd_state_t *state, bool reset)
{
    spi_init(spi1, 10000 * 1000);
    gpio_set_function(state->CLK_gpio, GPIO_FUNC_SPI);
    gpio_set_function(state->MOSI_gpio, GPIO_FUNC_SPI);

    if (reset) {
        gpio_init(state->RST_gpio);
        gpio_set_dir(state->RST_gpio, GPIO_OUT);
    }
    gpio_init(state->DC_gpio);
    gpio_set_dir(state->DC_gpio, GPIO_OUT);
    gpio_init(state->CS_gpio);
    gpio_set_dir(state->CS_gpio, GPIO_OUT);
    gpio_init(state->BL_gpio);
    gpio_set_dir(state->BL_gpio, GPIO_OUT);

    gpio_put(state->CS_gpio, 1);
    gpio_put(state->DC_gpio, 0);
    gpio_put(state->BL_gpio, 1);

    gpio_set_function(state->BL_gpio, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(state->BL_gpio);
    pwm_set_wrap(slice_num, 100);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 1);
    pwm_set_clkdiv(slice_num, 50);
    pwm_set_enabled(slice_num, true);
}

// Use the PWM to set the backlight level for all displays
void lcd_set_backlight(lcd_state_t *state, uint8_t level)
{
    (void)state;
    if (level > 100) {
        level = 100;
    }
    pwm_set_chan_level(slice_num, PWM_CHAN_B, level);
}

// Cycle reset for all displays
static void lcd_reset(lcd_state_t *state)
{
    gpio_put(state->RST_gpio, 1);
    sleep_ms(100);
    gpio_put(state->RST_gpio, 0);
    sleep_ms(100);
    gpio_put(state->RST_gpio, 1);
    sleep_ms(100);
}

// Select an LCD and send a command byte
static void st7789_command(lcd_state_t *state, uint8_t reg)
{
    gpio_put(state->DC_gpio, 0);
    gpio_put(state->CS_gpio, 0);
    spi_write_blocking(spi1, &reg, 1);
    gpio_put(state->CS_gpio, 1);
}

// Select an LCD and send a data byte
static void st7789_data_byte(lcd_state_t *state, uint8_t data)
{
    gpio_put(state->DC_gpio, 1);
    gpio_put(state->CS_gpio, 0);
    spi_write_blocking(spi1, &data, 1);
    gpio_put(state->CS_gpio, 1);
}

#if 0
// Select an LCD and send a data word (2 bytes)
static void st7789_data_word(lcd_state_t *state, uint16_t data)
{
    gpio_put(state->DC_gpio, 1);
    gpio_put(state->CS_gpio, 0);
    spi_write_blocking(spi1, (uint8_t *)&data, 2);
    gpio_put(state->CS_gpio, 1);
}
#endif

static void st7789_init(lcd_state_t *state)
{
    uint8_t MemoryAccessReg = 0x00;

    if (state->scan_dir == HORIZONTAL) {
        state->height = LCD_WIDTH;
        state->width = LCD_HEIGHT;
        MemoryAccessReg = 0X78;
    } else {
        state->height = LCD_HEIGHT;
        state->width = LCD_WIDTH;
        MemoryAccessReg = 0X00;
    }
    st7789_command(state, 0x36);              // MX, MY, RGB mode
    st7789_data_byte(state, MemoryAccessReg); // 0x08 set RGB
    st7789_command(state, 0x11);
    sleep_ms(120);
    st7789_command(state, 0x36);
    if (state->scan_dir == HORIZONTAL)
        st7789_data_byte(state, 0x00);
    else
        st7789_data_byte(state, 0x70);

    st7789_command(state, 0x3A);
    st7789_data_byte(state, 0x05);

    st7789_command(state, 0xB2);
    st7789_data_byte(state, 0x0C);
    st7789_data_byte(state, 0x0C);
    st7789_data_byte(state, 0x00);
    st7789_data_byte(state, 0x33);
    st7789_data_byte(state, 0x33);

    st7789_command(state, 0xB7);
    st7789_data_byte(state, 0x35);

    st7789_command(state, 0xBB);
    st7789_data_byte(state, 0x35);

    st7789_command(state, 0xC0);
    st7789_data_byte(state, 0x2C);

    st7789_command(state, 0xC2);
    st7789_data_byte(state, 0x01);

    st7789_command(state, 0xC3);
    st7789_data_byte(state, 0x13);

    st7789_command(state, 0xC4);
    st7789_data_byte(state, 0x20);

    st7789_command(state, 0xC6);
    st7789_data_byte(state, 0x0F);

    st7789_command(state, 0xD0);
    st7789_data_byte(state, 0xA4);
    st7789_data_byte(state, 0xA1);

    st7789_command(state, 0xD6);
    st7789_data_byte(state, 0xA1);

    st7789_command(state, 0xE0);
    st7789_data_byte(state, 0xF0);
    st7789_data_byte(state, 0x00);
    st7789_data_byte(state, 0x04);
    st7789_data_byte(state, 0x04);
    st7789_data_byte(state, 0x04);
    st7789_data_byte(state, 0x05);
    st7789_data_byte(state, 0x29);
    st7789_data_byte(state, 0x33);
    st7789_data_byte(state, 0x3E);
    st7789_data_byte(state, 0x38);
    st7789_data_byte(state, 0x12);
    st7789_data_byte(state, 0x12);
    st7789_data_byte(state, 0x28);
    st7789_data_byte(state, 0x30);

    st7789_command(state, 0xE1);
    st7789_data_byte(state, 0xF0);
    st7789_data_byte(state, 0x07);
    st7789_data_byte(state, 0x0A);
    st7789_data_byte(state, 0x0D);
    st7789_data_byte(state, 0x0B);
    st7789_data_byte(state, 0x07);
    st7789_data_byte(state, 0x28);
    st7789_data_byte(state, 0x33);
    st7789_data_byte(state, 0x3E);
    st7789_data_byte(state, 0x36);
    st7789_data_byte(state, 0x14);
    st7789_data_byte(state, 0x14);
    st7789_data_byte(state, 0x29);
    st7789_data_byte(state, 0x32);

    st7789_command(state, 0x21);

    st7789_command(state, 0x11);
    sleep_ms(120);
    st7789_command(state, 0x29);
}

// Set the window address for the LCD update to be thw whole LCD
void st7789_set_command_windows(lcd_state_t *state)
{
    if (state->scan_dir == HORIZONTAL) { // set the X coordinates
        st7789_command(state, 0x2A);
        st7789_data_byte(state, 0x00);
        st7789_data_byte(state, 0x22);
        st7789_data_byte(state, 0x00);
        st7789_data_byte(state, 0xcd);

        // set the Y coordinates
        st7789_command(state, 0x2B);
        st7789_data_byte(state, 0x00);
        st7789_data_byte(state, 0x00);
        st7789_data_byte(state, 0x01);
        st7789_data_byte(state, 0x3f);
    } else {
        // set the X coordinates
        st7789_command(state, 0x2A);
        st7789_data_byte(state, 0x00);
        st7789_data_byte(state, 0x00);
        st7789_data_byte(state, 0x01);
        st7789_data_byte(state, 0x3f);

        // set the Y coordinates
        st7789_command(state, 0x2B);
        st7789_data_byte(state, 0x00);
        st7789_data_byte(state, 0x22);
        st7789_data_byte(state, 0x00);
        st7789_data_byte(state, 0xcd);
    }

    st7789_command(state, 0x2C);
}

/* Clear the screen with a temporary frame buffer.
   TODO: this is a future frame-buffer-free way of driving the LCD */
#if 0
static void LCD_1IN47_Clear(lcd_state_t *state, uint16_t Color)
{
    uint16_t j;
    uint16_t Image[state->width * state->height];

    Color = ((Color << 8) & 0xff00) | (Color >> 8);

    for (j = 0; j < state->height * state->width; j++) {
        Image[j] = Color;
    }

    st7789_set_command_windows(state);
    gpio_put(state->DC_gpio, 1);
    gpio_put(state->CS_gpio, 0);
    for (j = 0; j < state->height; j++) {
        spi_write_blocking(spi1, (uint8_t *)&Image[j * state->width], state->width * 2);
    }
    gpio_put(state->CS_gpio, 1);
}
#endif

/* Copy the frame buffer to the LCD a line at a time converting the 2-bit
   frame buffer colour into the native 16-bit colour */
void lcd_update_screen(lcd_state_t *state)
{
    uint16_t j, x;
    st7789_set_command_windows(state);
    gpio_put(state->DC_gpio, 1);
    gpio_put(state->CS_gpio, 0);

    uint8_t linebuf[state->width * 2];

    for (j = 0; j < state->height; j++) {
        uint8_t *src = (uint8_t *)state->fb->data + (j * ((state->width + 3) / 4));
        for (x = 0; x < state->width; x++) {
            int byte = x / 4;
            int shift = 6 - 2 * (x % 4);
            uint8_t val = (src[byte] >> shift) & 0x3;
            uint16_t color = color_table[val];

            linebuf[x * 2] = (color >> 8) & 0xFF;
            linebuf[x * 2 + 1] = color & 0xFF;
        }
        spi_write_blocking(spi1, (uint8_t *)linebuf, state->width * 2);
    }
    gpio_put(state->CS_gpio, 1);
    st7789_command(state, 0x29);
}
