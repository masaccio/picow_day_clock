#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fb.h"
#include "lcd.h"

/******************************************************************************
function: Create Image
parameter:
    image   :   Pointer to the image cache
    width   :   The width of the picture
    Height  :   The height of the picture
    color   :   Whether the picture is inverted
******************************************************************************/
frame_buffer_t *fb_create(uint16_t width, uint16_t height, uint16_t rotate)
{
    frame_buffer_t *state = (frame_buffer_t *)calloc(1, sizeof(frame_buffer_t));
    if (state == NULL) {
        return NULL;
    }

    state->data = (uint8_t *)malloc((height + 1) * (width / 4));
    if (state->data == NULL) {
        free(state);
        return NULL;
    }

    state->width_memory = width;
    state->height_memory = height;

    /* Hard-wired 2 bits per pixel */
    state->width_byte = (state->width_memory % 4 == 0) ? (state->width_memory / 4) : (state->width_memory / 4 + 1);
    state->height_byte = height;

    state->rotate = rotate;

    if (rotate == ROTATE_0 || rotate == ROTATE_180) {
        state->width = width;
        state->height = height;
    } else {
        state->width = height;
        state->height = width;
    }
    return state;
}

/******************************************************************************
function: Select Image Rotate
parameter:
    Rotate : 0,90,180,270
******************************************************************************/
void fb_rotate(frame_buffer_t *state, uint16_t rotate)
{
    if (!(rotate == ROTATE_0 || rotate == ROTATE_90 || rotate == ROTATE_180 || rotate == ROTATE_270)) {
        rotate = ROTATE_0;
    }
    state->rotate = rotate;
}

/******************************************************************************
function: Draw Pixels
parameter:
    x_point : At point x
    y_point : At point y
    color  : Painted colors
******************************************************************************/
void fb_set_pixel(frame_buffer_t *state, uint16_t x_point, uint16_t y_point, uint16_t color)
{
    if (x_point > state->width || y_point > state->height) {
        return;
    }
    uint16_t x, y;

    switch (state->rotate) {
    case 0:
        x = x_point;
        y = y_point;
        break;
    case 90:
        x = state->width_memory - y_point - 1;
        y = x_point;
        break;
    case 180:
        x = state->width_memory - x_point - 1;
        y = state->height_memory - y_point - 1;
        break;
    case 270:
        x = y_point;
        y = state->height_memory - x_point - 1;
        break;
    default:
        return;
    }

    if (x > state->width_memory || y > state->height_memory) {
        return;
    }

    uint32_t Addr = x / 4 + y * state->width_byte;
    color = color % 4; // Guaranteed color scale is 4  --- 0~3
    uint8_t Rdata = state->data[Addr];

    Rdata = Rdata & (~(0xC0 >> ((x % 4) * 2)));
    state->data[Addr] = Rdata | ((color << 6) >> ((x % 4) * 2));
}

/* Clear the entire frame buffer and set to a specific color */
void fb_clear(frame_buffer_t *state, color_t color)
{
    for (uint16_t y = 0; y < state->height_byte; y++) {
        for (uint16_t x = 0; x < state->width_byte; x++) {
            uint32_t addr = x + y * state->width_byte;
            /* 2 bits per pixel */
            state->data[addr] = color | (color << 2) | (color << 4) | (color << 6);
        }
    }
}

/******************************************************************************
function: Clear the color of a window
parameter:
    Xstart : x starting point
    Ystart : y starting point
    Xend   : x end point
    Yend   : y end point
    color  : Painted colors
******************************************************************************/
void paint_clear_windows(frame_buffer_t *state, uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end,
                         uint16_t color)
{
    uint16_t x, y;
    for (y = y_start; y < y_end; y++) {
        for (x = x_start; x < x_end; x++) { // 8 pixel =  1 byte
            fb_set_pixel(state, x, y, color);
        }
    }
}

/******************************************************************************
function: Draw Point(x_point, y_point) Fill the color
parameter:
    x_point		: The x_point coordinate of the point
    y_point		: The y_point coordinate of the point
    color		: Painted color
******************************************************************************/
void paint_draw_point(frame_buffer_t *state, uint16_t x_point, uint16_t y_point, uint16_t color)
{
    if (x_point > state->width || y_point > state->height) {
        return;
    }

    int16_t x_dir_num, y_dir_num;
    for (x_dir_num = 0; x_dir_num < 2 * 1 - 1; x_dir_num++) {
        for (y_dir_num = 0; y_dir_num < 2 * 1 - 1; y_dir_num++) {
            if (x_point + x_dir_num - 1 < 0 || y_point + y_dir_num - 1 < 0)
                break;
            fb_set_pixel(state, x_point + x_dir_num - 1, y_point + y_dir_num - 1, color);
        }
    }
}

/******************************************************************************
function: Draw a line of arbitrary slope
parameter:
    Xstart ：Starting x_point point coordinates
    Ystart ：Starting x_point point coordinates
    Xend   ：End point x_point coordinate
    Yend   ：End point y_point coordinate
    color  ：The color of the line segment
******************************************************************************/
void paint_draw_line(frame_buffer_t *state, uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end,
                     uint16_t color)
{
    if (x_start > state->width || y_start > state->height || x_end > state->width || y_end > state->height) {
        return;
    }

    uint16_t x_point = x_start;
    uint16_t y_point = y_start;
    int dx = (int)x_end - (int)x_start >= 0 ? x_end - x_start : x_start - x_end;
    int dy = (int)y_end - (int)y_start <= 0 ? y_end - y_start : y_start - y_end;

    // Increment direction, 1 is positive, -1 is counter;
    int x_addway = x_start < x_end ? 1 : -1;
    int y_addway = y_start < y_end ? 1 : -1;

    // Cumulative error
    int esp = dx + dy;
    char dotted_len = 0;

    for (;;) {
        dotted_len++;
        // Painted dotted line, 2 point is really virtual
        paint_draw_point(state, x_point, y_point, color);
        if (2 * esp >= dy) {
            if (x_point == x_end)
                break;
            esp += dy;
            x_point += x_addway;
        }
        if (2 * esp <= dx) {
            if (y_point == y_end)
                break;
            esp += dx;
            y_point += y_addway;
        }
    }
}

/******************************************************************************
function: Draw a rectangle
parameter:
    Xstart ：Rectangular  Starting x_point point coordinates
    Ystart ：Rectangular  Starting x_point point coordinates
    Xend   ：Rectangular  End point x_point coordinate
    Yend   ：Rectangular  End point y_point coordinate
    color  ：The color of the Rectangular segment
******************************************************************************/
void fb_draw_rectangle(frame_buffer_t *state, uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end,
                       uint16_t color)
{
    if (x_start > state->width || y_start > state->height || x_end > state->width || y_end > state->height) {
        return;
    }

    uint16_t y_point;
    for (y_point = y_start; y_point < y_end; y_point++) {
        paint_draw_line(state, x_start, y_point, x_end, y_point, color);
    }
}

int fb_write_char(frame_buffer_t *state, uint16_t x_point, uint16_t y_point, const char ascii_char,
                  var_width_font_t *font, color_t fgcolor, color_t bgcolor)
{
    uint16_t page, column;

    const var_width_font_entry_t *entry = &font->table[ascii_char - ' '];
    const uint8_t *ptr = entry->table;
    int font_height = font->Height;
    int font_width = entry->Width;

    for (page = 0; page < font->Height; page++) {
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
        ptr += font->Width - font_width / 8; // Move to next line
    } // Write all

    return font_width;
}

void fb_write_string(frame_buffer_t *state, uint16_t x_start, uint16_t y_start, const char *p_string,
                     var_width_font_t *font, color_t fgcolor, color_t bgcolor)
{
    uint16_t x_point = x_start;
    uint16_t y_point = y_start;

    if (x_start > state->width || y_start > state->height) {
        return;
    }

    int width = 0;
    while (*p_string != '\0') {
        // if x direction filled , reposition to(x_start,y_point),y_point is y
        // direction plus the Height of the character
        if ((x_point + width) > state->width) {
            x_point = x_start;
            y_point += font->Height;
        }

        // If the y direction is full, reposition to(x_start, y_start)
        if ((y_point + font->Height) > state->height) {
            x_point = x_start;
            y_point = y_start;
        }
        width = fb_write_char(state, x_point, y_point, *p_string, font, fgcolor, bgcolor);

        // The next character of the address
        p_string++;

        x_point += width;
    }
}