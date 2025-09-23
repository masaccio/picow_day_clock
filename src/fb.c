#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "fb.h"

// Create a new frame buffer, allocating memory and initialising
frame_buffer_t *fb_create(void)
{
    frame_buffer_t *state = (frame_buffer_t *)calloc(1, sizeof(frame_buffer_t));
    if (state == NULL) {
        return NULL;
    }

    state->data = (uint8_t *)malloc((LCD_LONG_EDGE_PIXELS + 1) * (LCD_SHORT_EDGE_PIXELS / 4));
    if (state->data == NULL) {
        free(state);
        return NULL;
    }

    state->short_edge_num_pixels = LCD_SHORT_EDGE_PIXELS;
    state->long_edge_num_pixels = LCD_LONG_EDGE_PIXELS;

    // Hard-wired 2 bits per pixel
    state->short_edge_num_bytes = (state->short_edge_num_pixels % 4 == 0) ? (state->short_edge_num_pixels / 4)
                                                                          : (state->short_edge_num_pixels / 4 + 1);
    state->long_edge_num_bytes = LCD_LONG_EDGE_PIXELS;

    return state;
}

// Draw an individual pixel
void fb_set_pixel(frame_buffer_t *state, uint16_t x_point, uint16_t y_point, uint16_t color)
{
    if (x_point > LCD_LONG_EDGE_PIXELS || y_point > LCD_SHORT_EDGE_PIXELS) {
        return;
    }
    uint16_t x, y;
    x = state->short_edge_num_pixels - y_point - 1;
    y = x_point;

    if (x > state->short_edge_num_pixels || y > state->long_edge_num_pixels) {
        return;
    }

    uint32_t addr = x / 4 + y * state->short_edge_num_bytes;
    color = color % 4; // Guaranteed color scale is 4  --- 0~3
    uint8_t data_byte = state->data[addr];

    data_byte = data_byte & (~(0xC0 >> ((x % 4) * 2)));
    state->data[addr] = data_byte | ((color << 6) >> ((x % 4) * 2));
}
void fb_copy_image(frame_buffer_t *state, const uint8_t *image, uint16_t x_start, uint16_t y_start,
                   uint16_t image_width, uint16_t image_height, color_t fgcolor)
{
    int bytes_per_row = (image_width + 7) / 8;

    for (int yy = 0; yy < image_height; yy++) {
        for (int byte_idx = 0; byte_idx < bytes_per_row; byte_idx++) {
            uint8_t byte = *image++;
            for (int bit = 0; bit < 8; bit++) {
                int x = x_start + (byte_idx * 8) + bit;
                if (x < x_start + image_width) {
                    // Most significant bit is left-most pixel
                    fb_set_pixel(state, x, y_start + yy, (byte & (1 << (7 - bit))) ? fgcolor : 0);
                }
            }
        }
    }
}

// Clear the entire frame buffer and set to a specific color
void fb_clear(frame_buffer_t *state, color_t color)
{
    for (uint16_t y = 0; y < state->long_edge_num_bytes; y++) {
        for (uint16_t x = 0; x < state->short_edge_num_bytes; x++) {
            uint32_t addr = x + y * state->short_edge_num_bytes;
            // 2 bits per pixel
            state->data[addr] = color | (color << 2) | (color << 4) | (color << 6);
        }
    }
}

// Write a single character to the display
int fb_write_char(frame_buffer_t *state, uint16_t x_point, uint16_t y_point, const char ascii_char, font_t *font,
                  color_t fgcolor, color_t bgcolor)
{
    uint16_t page, column;

    const font_glyph_t *entry = &font->table[ascii_char - ' '];
    const uint8_t *ptr = entry->table;
    uint16_t font_width = entry->width;

    for (page = 0; page < font->height; page++) {
        for (column = 0; column < font_width; column++) {
            if (*ptr & (0x80 >> (column % 8))) {
                fb_set_pixel(state, x_point + column, y_point + page, fgcolor);
            } else {
                fb_set_pixel(state, x_point + column, y_point + page, bgcolor);
            }
            // One pixel is 8 bits
            if (column % 8 == 7)
                ptr++;
        } // Write a line
        ptr += font->byte_width - font_width / 8; // Move to next line
    } // Write all

    return font_width;
}

void fb_write_string(frame_buffer_t *state, uint16_t x_start, uint16_t y_start, const char *p_string, font_t *font,
                     color_t fgcolor, color_t bgcolor)
{
    uint16_t x_point = x_start;
    uint16_t y_point = y_start;

    if (x_start > LCD_LONG_EDGE_PIXELS || y_start > LCD_SHORT_EDGE_PIXELS) {
        return;
    }

    int width = 0;
    while (*p_string != '\0') {
        // if x direction filled , reposition to(x_start,y_point),y_point is y
        // direction plus the Height of the character
        if ((x_point + width) > LCD_LONG_EDGE_PIXELS) {
            x_point = x_start;
            y_point += font->height;
        }

        // If the y direction is full, reposition to(x_start, y_start)
        if ((y_point + font->height) > LCD_SHORT_EDGE_PIXELS) {
            x_point = x_start;
            y_point = y_start;
        }
        width = fb_write_char(state, x_point, y_point, *p_string, font, fgcolor, bgcolor);

        // The next character of the address
        p_string++;

        x_point += width;
    }
}
