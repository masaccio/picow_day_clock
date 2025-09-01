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
#include "hardware/spi.h"
#include "pico/stdlib.h"

/* Local includes */
#include "fb.h"
#include "lcd.h"

/* Frame buffers for a 2-bit per pixel display */
static uint8_t lcd1_fb_data[(LCD_HEIGHT + 1) * (LCD_WIDTH / 4)];
static uint8_t lcd2_fb_data[(LCD_HEIGHT + 1) * (LCD_WIDTH / 4)];

lcd_state_t *lcd_init(lcd_config_t *config, int lcd_number)
{
    lcd_state_t *state = (lcd_state_t *)calloc(1, sizeof(lcd_state_t));

    sleep_ms(100); /* TODO: is this required? */

    lcd_init_hardware(config, lcd_number == 1);

    state->frame_buffer_data = (lcd_number == 1 ? lcd1_fb_data : lcd2_fb_data);
    state->y_offset = 0;
    state->config = config;

    LCD_1IN47_Init(config, VERTICAL, /* reset */ lcd_number == 1);
    lcd_set_backlight(0);

    state->frame_buffer = fb_create(state->frame_buffer_data, LCD_WIDTH, LCD_HEIGHT, 0, BGCOLOR);
    fb_clear(state->frame_buffer, BGCOLOR);
    fb_rotate(state->frame_buffer, ROTATE_90);

    lcd_set_backlight(100);

    return state;
}

void print_line(lcd_state_t *state, const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    fb_write_string(state->frame_buffer, 0, state->y_offset, buffer, &TEXT_FONT, FGCOLOR, BGCOLOR);
    state->y_offset += TEXT_FONT_HEIGHT + 2;
}

void clear_screen(lcd_state_t *state)
{
    fb_clear(state->frame_buffer, BGCOLOR);
    state->y_offset = 0;
    LCD_1IN47_Display_2Bit(state->config, state->frame_buffer);
}

void update_screen(lcd_state_t *state)
{
    LCD_1IN47_Display_2Bit(state->config, state->frame_buffer);
}

LCD_1IN47_ATTRIBUTES LCD_1IN47;

static uint16_t color_table[] = {0x0000 /* black */, 0xF800 /* red */, 0x07E0 /* green */, 0xFFFF /* white */};

static uint slice_num;

void lcd_init_hardware(lcd_config_t *config, bool reset)
{
    spi_init(spi1, 10000 * 1000);
    gpio_set_function(config->CLK_gpio, GPIO_FUNC_SPI);
    gpio_set_function(config->MOSI_gpio, GPIO_FUNC_SPI);

    if (reset) {
        gpio_init(config->RST_gpio);
        gpio_set_dir(config->RST_gpio, GPIO_OUT);
    }
    gpio_init(config->DC_gpio);
    gpio_set_dir(config->DC_gpio, GPIO_OUT);
    gpio_init(config->CS_gpio);
    gpio_set_dir(config->CS_gpio, GPIO_OUT);
    gpio_init(config->BL_gpio);
    gpio_set_dir(config->BL_gpio, GPIO_OUT);

    gpio_put(config->CS_gpio, 1);
    gpio_put(config->DC_gpio, 0);
    gpio_put(config->BL_gpio, 1);

    gpio_set_function(config->BL_gpio, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(config->BL_gpio);
    pwm_set_wrap(slice_num, 100);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 1);
    pwm_set_clkdiv(slice_num, 50);
    pwm_set_enabled(slice_num, true);
}

void lcd_set_backlight(uint8_t level)
{
    if (level > 100) {
        level = 100;
    }
    pwm_set_chan_level(slice_num, PWM_CHAN_B, level);
}

/* Cycle reset for all displays */
static void lcd_hardware_reset(lcd_config_t *config)
{
    gpio_put(config->RST_gpio, 1);
    sleep_ms(100);
    gpio_put(config->RST_gpio, 0);
    sleep_ms(100);
    gpio_put(config->RST_gpio, 1);
    sleep_ms(100);
}

/* Select an LCD and send a command byte */
static void lcd_send_command(lcd_config_t *config, uint8_t reg)
{
    gpio_put(config->DC_gpio, 0);
    gpio_put(config->CS_gpio, 0);
    spi_write_blocking(spi1, &reg, 1);
    gpio_put(config->CS_gpio, 1);
}

/* Select an LCD and send a data byte */
static void lcd_send_data_byte(lcd_config_t *config, uint8_t data)
{
    gpio_put(config->DC_gpio, 1);
    gpio_put(config->CS_gpio, 0);
    spi_write_blocking(spi1, &data, 1);
    gpio_put(config->CS_gpio, 1);
}

/* Select an LCD and send a data word (2 bytes) */
static void lcd_send_data_word(lcd_config_t *config, uint16_t data)
{
    gpio_put(config->DC_gpio, 1);
    gpio_put(config->CS_gpio, 0);
    spi_write_blocking(spi1, (uint8_t *)&data, 2);
    gpio_put(config->CS_gpio, 1);
}

/******************************************************************************
function :	Initialize the lcd register
parameter:
******************************************************************************/
static void LCD_1IN47_InitReg(lcd_config_t *config)
{
    lcd_send_command(config, 0x11);
    sleep_ms(120);
    lcd_send_command(config, 0x36);
    if (config->scan_dir == HORIZONTAL)
        lcd_send_data_byte(config, 0x00);
    else
        lcd_send_data_byte(config, 0x70);

    lcd_send_command(config, 0x3A);
    lcd_send_data_byte(config, 0x05);

    lcd_send_command(config, 0xB2);
    lcd_send_data_byte(config, 0x0C);
    lcd_send_data_byte(config, 0x0C);
    lcd_send_data_byte(config, 0x00);
    lcd_send_data_byte(config, 0x33);
    lcd_send_data_byte(config, 0x33);

    lcd_send_command(config, 0xB7);
    lcd_send_data_byte(config, 0x35);

    lcd_send_command(config, 0xBB);
    lcd_send_data_byte(config, 0x35);

    lcd_send_command(config, 0xC0);
    lcd_send_data_byte(config, 0x2C);

    lcd_send_command(config, 0xC2);
    lcd_send_data_byte(config, 0x01);

    lcd_send_command(config, 0xC3);
    lcd_send_data_byte(config, 0x13);

    lcd_send_command(config, 0xC4);
    lcd_send_data_byte(config, 0x20);

    lcd_send_command(config, 0xC6);
    lcd_send_data_byte(config, 0x0F);

    lcd_send_command(config, 0xD0);
    lcd_send_data_byte(config, 0xA4);
    lcd_send_data_byte(config, 0xA1);

    lcd_send_command(config, 0xD6);
    lcd_send_data_byte(config, 0xA1);

    lcd_send_command(config, 0xE0);
    lcd_send_data_byte(config, 0xF0);
    lcd_send_data_byte(config, 0x00);
    lcd_send_data_byte(config, 0x04);
    lcd_send_data_byte(config, 0x04);
    lcd_send_data_byte(config, 0x04);
    lcd_send_data_byte(config, 0x05);
    lcd_send_data_byte(config, 0x29);
    lcd_send_data_byte(config, 0x33);
    lcd_send_data_byte(config, 0x3E);
    lcd_send_data_byte(config, 0x38);
    lcd_send_data_byte(config, 0x12);
    lcd_send_data_byte(config, 0x12);
    lcd_send_data_byte(config, 0x28);
    lcd_send_data_byte(config, 0x30);

    lcd_send_command(config, 0xE1);
    lcd_send_data_byte(config, 0xF0);
    lcd_send_data_byte(config, 0x07);
    lcd_send_data_byte(config, 0x0A);
    lcd_send_data_byte(config, 0x0D);
    lcd_send_data_byte(config, 0x0B);
    lcd_send_data_byte(config, 0x07);
    lcd_send_data_byte(config, 0x28);
    lcd_send_data_byte(config, 0x33);
    lcd_send_data_byte(config, 0x3E);
    lcd_send_data_byte(config, 0x36);
    lcd_send_data_byte(config, 0x14);
    lcd_send_data_byte(config, 0x14);
    lcd_send_data_byte(config, 0x29);
    lcd_send_data_byte(config, 0x32);

    lcd_send_command(config, 0x21);

    lcd_send_command(config, 0x11);
    sleep_ms(120);
    lcd_send_command(config, 0x29);
}

/********************************************************************************
function:	Set the resolution and scanning method of the screen
parameter:
        Scan_dir:   Scan direction
********************************************************************************/
static void LCD_1IN47_SetAttributes(lcd_config_t *config, uint8_t scan_dir)
{
    config->scan_dir = scan_dir;
    uint8_t MemoryAccessReg = 0x00;

    if (scan_dir == HORIZONTAL) {
        config->height = LCD_WIDTH;
        config->width = LCD_HEIGHT;
        MemoryAccessReg = 0X78;
    } else {
        config->height = LCD_HEIGHT;
        config->width = LCD_WIDTH;
        MemoryAccessReg = 0X00;
    }
    lcd_send_command(config, 0x36);              // MX, MY, RGB mode
    lcd_send_data_byte(config, MemoryAccessReg); // 0x08 set RGB
}

/********************************************************************************
function :	Initialize the lcd
parameter:
********************************************************************************/
void LCD_1IN47_Init(lcd_config_t *config, uint8_t scan_dir, bool reset)
{
    lcd_set_backlight(90);
    // Hardware reset
    if (reset) {
        lcd_hardware_reset(config);
    }

    // Set the resolution and scanning method of the screen
    LCD_1IN47_SetAttributes(config, scan_dir);

    // Set the initialization register
    LCD_1IN47_InitReg(config);
}

/********************************************************************************
function:	Sets the start position and size of the display area
parameter:
        Xstart 	:   X direction Start coordinates
        Ystart  :   Y direction Start coordinates
        Xend    :   X direction end coordinates
        Yend    :   Y direction end coordinates
********************************************************************************/
void LCD_1IN47_SetWindows(lcd_config_t *config, uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
{
    if (HORIZONTAL) { // set the X coordinates
        lcd_send_command(config, 0x2A);
        lcd_send_data_byte(config, 0x00);
        lcd_send_data_byte(config, 0x22);
        lcd_send_data_byte(config, 0x00);
        lcd_send_data_byte(config, 0xcd);

        // set the Y coordinates
        lcd_send_command(config, 0x2B);
        lcd_send_data_byte(config, 0x00);
        lcd_send_data_byte(config, 0x00);
        lcd_send_data_byte(config, 0x01);
        lcd_send_data_byte(config, 0x3f);
    } else {
        // set the X coordinates
        lcd_send_command(config, 0x2A);
        lcd_send_data_byte(config, 0x00);
        lcd_send_data_byte(config, 0x00);
        lcd_send_data_byte(config, 0x01);
        lcd_send_data_byte(config, 0x3f);

        // set the Y coordinates
        lcd_send_command(config, 0x2B);
        lcd_send_data_byte(config, 0x00);
        lcd_send_data_byte(config, 0x22);
        lcd_send_data_byte(config, 0x00);
        lcd_send_data_byte(config, 0xcd);
    }

    lcd_send_command(config, 0x2C);
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void LCD_1IN47_Clear(lcd_config_t *config, uint16_t Color)
{
    uint16_t j, i;
    uint16_t Image[config->width * config->height];

    Color = ((Color << 8) & 0xff00) | (Color >> 8);

    for (j = 0; j < config->height * config->width; j++) {
        Image[j] = Color;
    }

    LCD_1IN47_SetWindows(config, 0, 0, config->width, config->height);
    gpio_put(config->DC_gpio, 1);
    gpio_put(config->CS_gpio, 0);
    for (j = 0; j < config->height; j++) {
        spi_write_blocking(spi1, (uint8_t *)&Image[j * config->width], config->width * 2);
    }
    gpio_put(config->CS_gpio, 1);
}

void LCD_1IN47_Display_2Bit(lcd_config_t *config, void *Image)
{
    uint16_t j, x;
    LCD_1IN47_SetWindows(config, 0, 0, config->width, config->height);
    gpio_put(config->DC_gpio, 1);
    gpio_put(config->CS_gpio, 0);

    uint8_t linebuf[config->width * 2];

    for (j = 0; j < config->height; j++) {
        uint8_t *src = (uint8_t *)Image + (j * ((config->width + 3) / 4));
        for (x = 0; x < config->width; x++) {
            int byte = x / 4;
            int shift = 6 - 2 * (x % 4);
            uint8_t val = (src[byte] >> shift) & 0x3;
            uint16_t color = color_table[val];

            linebuf[x * 2] = (color >> 8) & 0xFF;
            linebuf[x * 2 + 1] = color & 0xFF;
        }
        spi_write_blocking(spi1, (uint8_t *)linebuf, config->width * 2);
    }
    gpio_put(config->CS_gpio, 1);
    lcd_send_command(config, 0x29);
}

void LCD_1IN47_DisplayPoint(lcd_config_t *config, uint16_t X, uint16_t Y, uint16_t Color)
{
    LCD_1IN47_SetWindows(config, X, Y, X, Y);
    lcd_send_data_word(config, Color);
}
