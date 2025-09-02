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
    uint16_t color;
    uint16_t rotate;
    uint16_t width_byte;
    uint16_t height_byte;
} frame_buffer_t;

/**
 * Display rotate
 **/
#define ROTATE_0 0
#define ROTATE_90 90
#define ROTATE_180 180
#define ROTATE_270 270

// init and Clear
frame_buffer_t *fb_create(uint16_t Width, uint16_t Height, uint16_t Rotate, uint16_t Color);
void Paint_SelectImage(frame_buffer_t *frame_buffer);
void fb_rotate(frame_buffer_t *frame_buffer, uint16_t Rotate);
void Paint_SetPixel(frame_buffer_t *frame_buffer, uint16_t Xpoint, uint16_t Ypoint, uint16_t Color);

void fb_clear(frame_buffer_t *frame_buffer, uint16_t Color);
void Paint_ClearWindows(frame_buffer_t *frame_buffer, uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,
                        uint16_t Color);

// Drawing
void Paint_DrawPoint(frame_buffer_t *frame_buffer, uint16_t Xpoint, uint16_t Ypoint, uint16_t Color);
void Paint_DrawLine(frame_buffer_t *frame_buffer, uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,
                    uint16_t Color);
void fb_draw_rectangle(frame_buffer_t *frame_buffer, uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,
                       uint16_t Color);

// Additional string functions not present in the original library
int Paint_DrawVariableWidthChar(frame_buffer_t *frame_buffer, uint16_t Xpoint, uint16_t Ypoint, const char Acsii_Char,
                                var_width_font_t *Font, uint16_t Color_Foreground, uint16_t Color_Background);

void fb_write_string(frame_buffer_t *frame_buffer, uint16_t Xstart, uint16_t Ystart, const char *pString,
                     var_width_font_t *Font, uint16_t Color_Foreground, uint16_t Color_Background);

#endif /* __FB_H */
