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
void Paint_ClearWindows(frame_buffer_t *state, uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,
                        uint16_t color)
{
    uint16_t x, y;
    for (y = Ystart; y < Yend; y++) {
        for (x = Xstart; x < Xend; x++) { // 8 pixel =  1 byte
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
void Paint_DrawPoint(frame_buffer_t *state, uint16_t x_point, uint16_t y_point, uint16_t color)
{
    if (x_point > state->width || y_point > state->height) {
        return;
    }

    int16_t XDir_Num, YDir_Num;
    for (XDir_Num = 0; XDir_Num < 2 * 1 - 1; XDir_Num++) {
        for (YDir_Num = 0; YDir_Num < 2 * 1 - 1; YDir_Num++) {
            if (x_point + XDir_Num - 1 < 0 || y_point + YDir_Num - 1 < 0)
                break;
            fb_set_pixel(state, x_point + XDir_Num - 1, y_point + YDir_Num - 1, color);
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
void Paint_DrawLine(frame_buffer_t *state, uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,
                    uint16_t color)
{
    if (Xstart > state->width || Ystart > state->height || Xend > state->width || Yend > state->height) {
        return;
    }

    uint16_t x_point = Xstart;
    uint16_t y_point = Ystart;
    int dx = (int)Xend - (int)Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
    int dy = (int)Yend - (int)Ystart <= 0 ? Yend - Ystart : Ystart - Yend;

    // Increment direction, 1 is positive, -1 is counter;
    int XAddway = Xstart < Xend ? 1 : -1;
    int YAddway = Ystart < Yend ? 1 : -1;

    // Cumulative error
    int Esp = dx + dy;
    char Dotted_Len = 0;

    for (;;) {
        Dotted_Len++;
        // Painted dotted line, 2 point is really virtual
        Paint_DrawPoint(state, x_point, y_point, color);
        if (2 * Esp >= dy) {
            if (x_point == Xend)
                break;
            Esp += dy;
            x_point += XAddway;
        }
        if (2 * Esp <= dx) {
            if (y_point == Yend)
                break;
            Esp += dx;
            y_point += YAddway;
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
void fb_draw_rectangle(frame_buffer_t *state, uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,
                       uint16_t color)
{
    if (Xstart > state->width || Ystart > state->height || Xend > state->width || Yend > state->height) {
        return;
    }

    uint16_t y_point;
    for (y_point = Ystart; y_point < Yend; y_point++) {
        Paint_DrawLine(state, Xstart, y_point, Xend, y_point, color);
    }
}

int fb_write_char(frame_buffer_t *state, uint16_t x_point, uint16_t y_point, const char Acsii_Char,
                  var_width_font_t *Font, color_t fgcolor, color_t bgcolor)
{
    uint16_t Page, Column;

    const var_width_font_entry_t *entry = &Font->table[Acsii_Char - ' '];
    const uint8_t *ptr = entry->table;
    int font_height = Font->Height;
    int font_width = entry->Width;

    for (Page = 0; Page < Font->Height; Page++) {
        for (Column = 0; Column < font_width; Column++) {
            if (*ptr & (0x80 >> (Column % 8))) {
                fb_set_pixel(state, x_point + Column, y_point + Page, fgcolor);
            } else {
                fb_set_pixel(state, x_point + Column, y_point + Page, bgcolor);
            }
            // One pixel is 8 bits
            if (Column % 8 == 7)
                ptr++;
        } // Write a line
        ptr += Font->Width - font_width / 8; // Move to next line
    } // Write all

    return font_width;
}

void fb_write_string(frame_buffer_t *state, uint16_t Xstart, uint16_t Ystart, const char *pString,
                     var_width_font_t *Font, color_t fgcolor, color_t bgcolor)
{
    uint16_t x_point = Xstart;
    uint16_t y_point = Ystart;

    if (Xstart > state->width || Ystart > state->height) {
        return;
    }

    int width = 0;
    while (*pString != '\0') {
        // if x direction filled , reposition to(Xstart,y_point),y_point is y
        // direction plus the Height of the character
        if ((x_point + width) > state->width) {
            x_point = Xstart;
            y_point += Font->Height;
        }

        // If the y direction is full, reposition to(Xstart, Ystart)
        if ((y_point + Font->Height) > state->height) {
            x_point = Xstart;
            y_point = Ystart;
        }
        width = fb_write_char(state, x_point, y_point, *pString, Font, fgcolor, bgcolor);

        // The next character of the address
        pString++;

        x_point += width;
    }
}