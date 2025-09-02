#ifndef __FB_H
#define __FB_H

#include <stdbool.h>
#include <stdint.h>

#include "fonts.h"

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

frame_buffer_t *fb_create(uint16_t Width, uint16_t Height, uint16_t Rotate);

void fb_rotate(frame_buffer_t *frame_buffer, uint16_t Rotate);

void fb_clear(frame_buffer_t *frame_buffer, color_t color);

void fb_set_pixel(frame_buffer_t *frame_buffer, uint16_t Xpoint, uint16_t Ypoint, uint16_t Color);

void Paint_ClearWindows(frame_buffer_t *frame_buffer, uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,
                        uint16_t Color);

void Paint_DrawPoint(frame_buffer_t *frame_buffer, uint16_t Xpoint, uint16_t Ypoint, uint16_t Color);

void Paint_DrawLine(frame_buffer_t *frame_buffer, uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,
                    uint16_t Color);

void fb_draw_rectangle(frame_buffer_t *frame_buffer, uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,
                       uint16_t Color);

int fb_write_char(frame_buffer_t *frame_buffer, uint16_t Xpoint, uint16_t Ypoint, const char Acsii_Char,
                  var_width_font_t *Font, color_t fgcolor, color_t bgcolor);

void fb_write_string(frame_buffer_t *frame_buffer, uint16_t Xstart, uint16_t Ystart, const char *pString,
                     var_width_font_t *Font, color_t fgcolor, color_t bgcolor);

#endif /* __FB_H */
