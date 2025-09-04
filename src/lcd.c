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

/* PICO SDK */
#include "hardware/pwm.h"
#include "pico/stdlib.h"

/* Local includes */
#include "fb.h"
#include "lcd.h"

/* How the 2-bit colours map to 16-bit colour */
static uint16_t color_table[] = {0x0000 /* black */, 0xF800 /* red */, 0x07E0 /* green */, 0xFFFF /* white */};

/* PWM slice attached to the backlight */
static uint slice_num;

static void lcd_reset(lcd_state_t *state);
static void st7789_init(lcd_state_t *state);

lcd_state_t *lcd_init(hal_t *hal, uint16_t RST_gpio, uint16_t DC_gpio, uint16_t BL_gpio, uint16_t CS_gpio,
                      uint16_t CLK_gpio, uint16_t MOSI_gpio, bool reset)

{
    lcd_state_t *state = (lcd_state_t *)calloc(1, sizeof(lcd_state_t));

    state->RST_gpio = RST_gpio;
    state->DC_gpio = DC_gpio;
    state->BL_gpio = BL_gpio;
    state->CS_gpio = CS_gpio;
    state->CLK_gpio = CLK_gpio;
    state->MOSI_gpio = MOSI_gpio;
    state->y_offset = 0;
    state->scan_dir = VERTICAL;
    state->hal = hal;

    lcd_init_peripherals(state, reset);
    lcd_set_backlight(state, 90);
    if (reset) {
        /* All displays share the same reset line */
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

void lcd_print_text(lcd_state_t *state, color_t color, const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    fb_write_string(state->fb, 0, state->y_offset, buffer, &text_font, /* fgcolor */ color, /* bgcolor */ BLACK);
    state->y_offset += text_font.height + 2;
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
    state->y_offset = 0;
    lcd_update_screen(state);
}

/* Initialise the Pico peripeherals we will use (SPI, GPIO, PWM) */
void lcd_init_peripherals(lcd_state_t *state, bool reset)
{
    state->hal->spi_init(10000 * 1000);
    state->hal->gpio_set_function(state->CLK_gpio, GPIO_FUNC_SPI);
    state->hal->gpio_set_function(state->MOSI_gpio, GPIO_FUNC_SPI);

    if (reset) {
        state->hal->gpio_init(state->RST_gpio);
        state->hal->gpio_set_dir(state->RST_gpio, GPIO_OUT);
    }
    state->hal->gpio_init(state->DC_gpio);
    state->hal->gpio_set_dir(state->DC_gpio, GPIO_OUT);
    state->hal->gpio_init(state->CS_gpio);
    state->hal->gpio_set_dir(state->CS_gpio, GPIO_OUT);
    state->hal->gpio_init(state->BL_gpio);
    state->hal->gpio_set_dir(state->BL_gpio, GPIO_OUT);

    state->hal->gpio_put(state->CS_gpio, 1);
    state->hal->gpio_put(state->DC_gpio, 0);
    state->hal->gpio_put(state->BL_gpio, 1);

    state->hal->gpio_set_function(state->BL_gpio, GPIO_FUNC_PWM);
    slice_num = state->hal->pwm_gpio_to_slice_num(state->BL_gpio);
    state->hal->pwm_set_wrap(slice_num, 100);
    state->hal->pwm_set_chan_level(slice_num, PWM_CHAN_B, 1);
    state->hal->pwm_set_clkdiv(slice_num, 50);
    state->hal->pwm_set_enabled(slice_num, true);
}

/* Use the PWM to set the backlight level for all displays */
void lcd_set_backlight(lcd_state_t *state, uint8_t level)
{
    if (level > 100) {
        level = 100;
    }
    state->hal->pwm_set_chan_level(slice_num, PWM_CHAN_B, level);
}

/* Cycle reset for all displays */
static void lcd_reset(lcd_state_t *state)
{
    state->hal->gpio_put(state->RST_gpio, 1);
    state->hal->sleep_ms(100);
    state->hal->gpio_put(state->RST_gpio, 0);
    state->hal->sleep_ms(100);
    state->hal->gpio_put(state->RST_gpio, 1);
    state->hal->sleep_ms(100);
}

/* Select an LCD and send a command byte */
static void st7789_command(lcd_state_t *state, uint8_t reg)
{
    state->hal->gpio_put(state->DC_gpio, 0);
    state->hal->gpio_put(state->CS_gpio, 0);
    state->hal->spi_write_blocking(&reg, 1);
    state->hal->gpio_put(state->CS_gpio, 1);
}

/* Select an LCD and send a data byte */
static void st7789_data_byte(lcd_state_t *state, uint8_t data)
{
    state->hal->gpio_put(state->DC_gpio, 1);
    state->hal->gpio_put(state->CS_gpio, 0);
    state->hal->spi_write_blocking(&data, 1);
    state->hal->gpio_put(state->CS_gpio, 1);
}

/* Select an LCD and send a data word (2 bytes) */
static void st7789_data_word(lcd_state_t *state, uint16_t data)
{
    state->hal->gpio_put(state->DC_gpio, 1);
    state->hal->gpio_put(state->CS_gpio, 0);
    state->hal->spi_write_blocking((uint8_t *)&data, 2);
    state->hal->gpio_put(state->CS_gpio, 1);
}

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
    state->hal->sleep_ms(120);
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
    state->hal->sleep_ms(120);
    st7789_command(state, 0x29);
}

/* Set the window address for the LCD update to be thw whole LCD */
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
static void LCD_1IN47_Clear(lcd_state_t *state, uint16_t Color)
{
    uint16_t j, i;
    uint16_t Image[state->width * state->height];

    Color = ((Color << 8) & 0xff00) | (Color >> 8);

    for (j = 0; j < state->height * state->width; j++) {
        Image[j] = Color;
    }

    st7789_set_command_windows(state);
    state->hal->gpio_put(state->DC_gpio, 1);
    state->hal->gpio_put(state->CS_gpio, 0);
    for (j = 0; j < state->height; j++) {
        state->hal->spi_write_blocking((uint8_t *)&Image[j * state->width], state->width * 2);
    }
    state->hal->gpio_put(state->CS_gpio, 1);
}

/* Copy the frame buffer to the LCD a line at a time converting the 2-bit
   frame buffer colour into the native 16-bit colour */
void lcd_update_screen(lcd_state_t *state)
{
    uint16_t j, x;
    st7789_set_command_windows(state);
    state->hal->gpio_put(state->DC_gpio, 1);
    state->hal->gpio_put(state->CS_gpio, 0);

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
        state->hal->spi_write_blocking((uint8_t *)linebuf, state->width * 2);
    }
    state->hal->gpio_put(state->CS_gpio, 1);
    st7789_command(state, 0x29);
}
