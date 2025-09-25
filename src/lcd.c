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
#include "mock.h"
#endif

// Local includes
#include "bitmap.h"
#include "clock.h"
#include "config.h"

// How the 2-bit colours map to 16-bit colour
static uint16_t color_table[] = LCD_COLOR_TABLE;

// PWM slice attached to the backlight
static uint slice_num;

static void st7789_init(lcd_state_t *state);

static void lcd_reset(lcd_state_t *state);

void lcd_write_image(lcd_state_t *state, const uint8_t *image, uint16_t x_start, uint16_t y_start, uint16_t image_width,
                     uint16_t image_height, color_t fgcolor);

int lcd_write_char(lcd_state_t *state, uint16_t x_point, uint16_t y_point, const char ascii_char, font_t *font,
                   color_t fgcolor, color_t bgcolor);

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

    lcd_init_peripherals(state, reset);
    lcd_set_backlight(state, 90);
    if (reset) {
        // All displays share the same reset line
        lcd_reset(state);
    }

    st7789_init(state);
    lcd_set_backlight(state, 0);
    lcd_clear_screen(state, BGCOLOR);
    lcd_set_backlight(state, 100);

    return state;
}

void lcd_print_line(lcd_state_t *state, uint16_t line_num, color_t color, const char *buffer)
{
    uint16_t y_start = (line_num * text_font.height) + 2;
    uint16_t x_point = 0;
    uint16_t y_point = y_start;
    int width = 0;
    while (*buffer != '\0') {
        // Wrap end of line
        if ((x_point + width) > LCD_WIDTH) {
            x_point = 0;
            y_point += text_font.height;
        }

        // Wrap end of screen
        if ((y_point + text_font.height) > LCD_HEIGHT) {
            x_point = 0;
            y_point = 0;
        }
        width = lcd_write_char(state, x_point, y_point, *buffer, &text_font, color, BGCOLOR);
        x_point += width;
        buffer++;
    }
}

void lcd_print_clock_digit(lcd_state_t *state, color_t color, const char ascii_char)
{
    const font_glyph_t entry = clock_digit_font.table[ascii_char - ' '];
    uint16_t x_point = (LCD_WIDTH - entry.width) / 2;
    (void)lcd_write_char(state, x_point, 0, ascii_char, &clock_digit_font, color, BLACK);
}

#define ICON_CASE(STATUS, ICON, OFFSET)                                                                                \
    case STATUS:                                                                                                       \
        icon = ICON;                                                                                                   \
        offset = OFFSET;                                                                                               \
        break;

void lcd_update_icon(lcd_state_t *state, clock_status_t status, bool is_error)
{
    const uint8_t *icon;
    int offset;
    switch (status) {
        ICON_CASE(STATUS_WIFI_OK, wifi_icon, -1)
        ICON_CASE(STATUS_WIFI_INIT, wifi_init_icon, -1)
        ICON_CASE(STATUS_WIFI_TIMEOUT, wifi_timeout_icon, -1)
        ICON_CASE(STATUS_WIFI_AUTH, wifi_password_icon, -1)
        ICON_CASE(STATUS_WIFI_CONNECT, wifi_connect_icon, -1)
        ICON_CASE(STATUS_WIFI_ERROR, wifi_error_icon, -1)

        ICON_CASE(STATUS_NTP_OK, ntp_icon, -2)
        ICON_CASE(STATUS_NTP_INIT, ntp_init_icon, -2)
        ICON_CASE(STATUS_NTP_DNS, ntp_dns_icon, -2)
        ICON_CASE(STATUS_NTP_TIMEOUT, ntp_timeout_icon, -2)
        ICON_CASE(STATUS_NTP_MEMORY, ntp_memory_icon, -2)
        ICON_CASE(STATUS_NTP_INVALID, ntp_error_icon, -2)

        ICON_CASE(STATUS_WATCHDOG_RESET, watchdog_icon, -3)

        default:
            return;
    }

    color_t color = is_error ? RED : GREEN;
    // Position last icon 5 pixels from edge to handle rounded corner
    uint16_t x_start = LCD_WIDTH - 5 + ICON_SIZE * offset;
    lcd_write_image(state, icon, x_start, 0, ICON_SIZE, ICON_SIZE, color);
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

static void st7789_init(lcd_state_t *state)
{

    st7789_command(state, 0x11); // SLPOUT (Sleep Out)

    sleep_ms(120);

    st7789_command(state, 0x36); // MADCTL (Memory Data Access Control)
    st7789_data_byte(state, /* MADCTL_MX */ 0x40 | /* MADCTL_MY */ 0x80);

    st7789_command(state, 0x3A); // COLMOD (Interface Pixel Format)
    st7789_data_byte(state, 0x05);

    st7789_command(state, 0xB2); // PORCTRL (Porch Setting)
    st7789_data_byte(state, 0x0C);
    st7789_data_byte(state, 0x0C);
    st7789_data_byte(state, 0x00);
    st7789_data_byte(state, 0x33);
    st7789_data_byte(state, 0x33);

    st7789_command(state, 0xB7); // GCTRL (Gate Control)
    st7789_data_byte(state, 0x35);

    st7789_command(state, 0xBB); // VCOMS (VCOM Setting)
    st7789_data_byte(state, 0x35);

    st7789_command(state, 0xC0); // LCMCTRL (LCM Control)
    st7789_data_byte(state, 0x2C);

    st7789_command(state, 0xC2); // VDVVRHEN (VDV and VRH Command Enable)
    st7789_data_byte(state, 0x01);

    st7789_command(state, 0xC3); // VRHS (VRH Set)
    st7789_data_byte(state, 0x13);

    st7789_command(state, 0xC4); // VDVS (VDV Set)
    st7789_data_byte(state, 0x20);

    st7789_command(state, 0xC6); // FRCTRL2 (Frame Rate Control in Normal Mode)
    st7789_data_byte(state, 0x0F);

    st7789_command(state, 0xD0); // PWCTRL1 (Power Control 1)
    st7789_data_byte(state, 0xA4);
    st7789_data_byte(state, 0xA1);

    st7789_command(state, 0xD6); // !!! Non-standard command
    st7789_data_byte(state, 0xA1);

    st7789_command(state, 0xE0); // PVGAMCTRL (Positive Voltage Gamma Control)
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

    st7789_command(state, 0xE1); // NVGAMCTRL (Negative Voltage Gamma Control)
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

    st7789_command(state, 0x21); // INVON (Display Inversion On)

    st7789_command(state, 0x11); // SLPOUT (Sleep Out)

    sleep_ms(120);

    st7789_command(state, 0x29); // DISPON (Display On)
}

void st7789_set_command_windows(lcd_state_t *state, uint16_t x_start, uint16_t y_start, uint16_t width, uint16_t height)
{

    // X-cordinates are based on portrait mode
    st7789_command(state, 0x2A); // CASET (Column address set)
    st7789_data_byte(state, (x_start + LCD_ROW_START) >> 8);
    st7789_data_byte(state, (x_start + LCD_ROW_START) & 0xff);
    st7789_data_byte(state, (x_start + width + LCD_ROW_START - 1) >> 8);
    st7789_data_byte(state, (x_start + width + LCD_ROW_START - 1) & 0xff);

    // Set the Y coordinates
    st7789_command(state, 0x2B); // RASET (Row address set)
    st7789_data_byte(state, (y_start + LCD_COL_START) >> 8);
    st7789_data_byte(state, (y_start + LCD_COL_START) & 0xff);
    st7789_data_byte(state, (y_start + height + LCD_COL_START - 1) >> 8);
    st7789_data_byte(state, (y_start + height + LCD_COL_START - 1) & 0xff);

    st7789_command(state, 0x2C); // RAMWR (Memory Write)
}

void lcd_draw_rectangle(lcd_state_t *state, uint16_t x_start, uint16_t y_start, uint16_t width, uint16_t height,
                        uint16_t color)
{
    st7789_set_command_windows(state, x_start, y_start, width, height);
    gpio_put(state->DC_gpio, 1);
    gpio_put(state->CS_gpio, 0);

    uint8_t linebuf[width * 2];
    for (uint16_t x = 0; x < width; x++) {
        linebuf[x * 2] = color_table[color % 4] >> 8;
        linebuf[x * 2 + 1] = color_table[color % 4] & 0xFF;
    }

    for (uint16_t j = 0; j < height; j++) {
        spi_write_blocking(spi1, (uint8_t *)linebuf, width * 2);
    }
    gpio_put(state->CS_gpio, 1);
    st7789_command(state, 0x29); // DISPON (Display On)
}

void lcd_clear_screen(lcd_state_t *state, uint16_t color)
{
    lcd_draw_rectangle(state, 0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

int lcd_write_char(lcd_state_t *state, uint16_t x_point, uint16_t y_point, const char ascii_char, font_t *font,
                   color_t fgcolor, color_t bgcolor)
{
    uint8_t linebuf[LCD_WIDTH * 2];
    const font_glyph_t *entry = &font->table[ascii_char - ' '];
    const uint8_t *ptr = entry->table;
    uint16_t font_width = entry->width;

    st7789_set_command_windows(state, x_point, y_point, font_width, font->height);
    gpio_put(state->DC_gpio, 1);
    gpio_put(state->CS_gpio, 0);

    for (uint16_t glyph_row = 0; glyph_row < font->height; glyph_row++) {
        for (uint16_t glyph_column = 0; glyph_column < font_width; glyph_column++) {
            char glyph_pixels = *(ptr + (glyph_column / 8));
            if (glyph_pixels & (0x80 >> (glyph_column % 8))) {
                linebuf[glyph_column * 2] = color_table[fgcolor % 4] >> 8;
                linebuf[glyph_column * 2 + 1] = color_table[fgcolor % 4] & 0xFF;
            } else {
                linebuf[glyph_column * 2] = color_table[bgcolor % 4] >> 8;
                linebuf[glyph_column * 2 + 1] = color_table[bgcolor % 4] & 0xFF;
            }
        }
        spi_write_blocking(spi1, (uint8_t *)linebuf, font_width * 2);
        ptr += font->byte_width;
    }

    gpio_put(state->CS_gpio, 1);
    st7789_command(state, 0x29); // DISPON (Display On)

    return font_width;
}

void lcd_write_image(lcd_state_t *state, const uint8_t *image, uint16_t x_start, uint16_t y_start, uint16_t image_width,
                     uint16_t image_height, color_t fgcolor)
{
    uint8_t linebuf[LCD_WIDTH * 2];

    st7789_set_command_windows(state, x_start, y_start, image_width, image_height);
    gpio_put(state->DC_gpio, 1);
    gpio_put(state->CS_gpio, 0);

    int bytes_per_row = (image_width + 7) / 8;

    for (int yy = 0; yy < image_height; yy++) {
        for (int byte_idx = 0; byte_idx < bytes_per_row; byte_idx++) {
            uint8_t byte = *image++;
            for (int bit = 0; bit < 8; bit++) {
                int x = x_start + (byte_idx * 8) + bit;
                if (x < x_start + image_width) {
                    // Most significant bit is left-most pixel
                    linebuf[(x - x_start) * 2] = color_table[(byte & (1 << (7 - bit))) ? fgcolor : 0] >> 8;
                    linebuf[(x - x_start) * 2 + 1] = color_table[(byte & (1 << (7 - bit))) ? fgcolor : 0] & 0xFF;
                }
            }
        }
        spi_write_blocking(spi1, (uint8_t *)linebuf, image_width * 2);
    }
    gpio_put(state->CS_gpio, 1);
    st7789_command(state, 0x29); // DISPON (Display On)
}
