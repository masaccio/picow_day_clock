#ifndef __FB_H
#define __FB_H

#include <stdbool.h>
#include <stdint.h>

#include "font.h"

typedef struct
{
    uint8_t *data;
    uint16_t width;
    uint16_t height;
    uint16_t width_memory;
    uint16_t height_memory;
    uint16_t rotate;
    uint16_t width_byte;
    uint16_t height_byte;
} frame_buffer_t;

typedef enum
{
    BLACK = 0x00,
    RED = 0x01,
    GREEN = 0x02,
    WHITE = 0x03
} color_t;

#define BGCOLOR BLACK
#define FGCOLOR WHITE

/**
 * Display rotate
 **/
#define ROTATE_0 0
#define ROTATE_90 90
#define ROTATE_180 180
#define ROTATE_270 270

frame_buffer_t *fb_create(uint16_t width, uint16_t height, uint16_t rotate);

void fb_rotate(frame_buffer_t *frame_buffer, uint16_t rotate);

void fb_clear(frame_buffer_t *frame_buffer, color_t color);

void fb_set_pixel(frame_buffer_t *frame_buffer, uint16_t x_point, uint16_t y_point, uint16_t color);

void paint_clear_windows(frame_buffer_t *frame_buffer, uint16_t x_start, uint16_t y_start, uint16_t x_end,
                         uint16_t y_end, uint16_t color);

void paint_draw_point(frame_buffer_t *frame_buffer, uint16_t x_point, uint16_t y_point, uint16_t color);

void paint_draw_line(frame_buffer_t *frame_buffer, uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end,
                     uint16_t color);

void fb_draw_rectangle(frame_buffer_t *frame_buffer, uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end,
                       uint16_t color);

int fb_write_char(frame_buffer_t *frame_buffer, uint16_t x_point, uint16_t y_point, const char ascii_char, font_t *font,
                  color_t fgcolor, color_t bgcolor);

void fb_write_string(frame_buffer_t *frame_buffer, uint16_t x_start, uint16_t y_start, const char *p_string,
                     font_t *font, color_t fgcolor, color_t bgcolor);

#endif // __FB_H
