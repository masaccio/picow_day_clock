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
    Color   :   Whether the picture is inverted
******************************************************************************/
frame_buffer_t *fb_create(uint16_t width, uint16_t height, uint16_t rotate, uint16_t color)
{
    frame_buffer_t *frame_buffer = (frame_buffer_t *)calloc(1, sizeof(frame_buffer_t));
    if (frame_buffer == NULL) {
        return NULL;
    }

    frame_buffer->data = (uint8_t *)malloc((height + 1) * (width / 4));
    if (frame_buffer->data == NULL) {
        free(frame_buffer);
        return NULL;
    }

    frame_buffer->width_memory = width;
    frame_buffer->height_memory = height;
    frame_buffer->color = color;

    /* Hard-wired 2 bits per pixel */
    frame_buffer->width_byte =
        (frame_buffer->width_memory % 4 == 0) ? (frame_buffer->width_memory / 4) : (frame_buffer->width_memory / 4 + 1);
    frame_buffer->height_byte = height;

    frame_buffer->rotate = rotate;

    if (rotate == ROTATE_0 || rotate == ROTATE_180) {
        frame_buffer->width = width;
        frame_buffer->height = height;
    } else {
        frame_buffer->width = height;
        frame_buffer->height = width;
    }
    return frame_buffer;
}

/******************************************************************************
function: Select Image Rotate
parameter:
    Rotate : 0,90,180,270
******************************************************************************/
void fb_rotate(frame_buffer_t *frame_buffer, uint16_t rotate)
{
    if (!(rotate == ROTATE_0 || rotate == ROTATE_90 || rotate == ROTATE_180 || rotate == ROTATE_270)) {
        rotate = ROTATE_0;
    }
    frame_buffer->rotate = rotate;
}

/******************************************************************************
function: Draw Pixels
parameter:
    Xpoint : At point X
    Ypoint : At point Y
    Color  : Painted colors
******************************************************************************/
void Paint_SetPixel(frame_buffer_t *frame_buffer, uint16_t Xpoint, uint16_t Ypoint, uint16_t Color)
{
    if (Xpoint > frame_buffer->width || Ypoint > frame_buffer->height) {
        return;
    }
    uint16_t X, Y;

    switch (frame_buffer->rotate) {
    case 0:
        X = Xpoint;
        Y = Ypoint;
        break;
    case 90:
        X = frame_buffer->width_memory - Ypoint - 1;
        Y = Xpoint;
        break;
    case 180:
        X = frame_buffer->width_memory - Xpoint - 1;
        Y = frame_buffer->height_memory - Ypoint - 1;
        break;
    case 270:
        X = Ypoint;
        Y = frame_buffer->height_memory - Xpoint - 1;
        break;
    default:
        return;
    }

    if (X > frame_buffer->width_memory || Y > frame_buffer->height_memory) {
        return;
    }

    uint32_t Addr = X / 4 + Y * frame_buffer->width_byte;
    Color = Color % 4; // Guaranteed color scale is 4  --- 0~3
    uint8_t Rdata = frame_buffer->data[Addr];

    Rdata = Rdata & (~(0xC0 >> ((X % 4) * 2)));
    frame_buffer->data[Addr] = Rdata | ((Color << 6) >> ((X % 4) * 2));
}

/******************************************************************************
function: Clear the color of the picture
parameter:
    Color : Painted colors
******************************************************************************/
void fb_clear(frame_buffer_t *frame_buffer, uint16_t color)
{
    for (uint16_t Y = 0; Y < frame_buffer->height_byte; Y++) {
        for (uint16_t X = 0; X < frame_buffer->width_byte; X++) { // 8 pixel =  1 byte
            uint32_t addr = X + Y * frame_buffer->width_byte;
            frame_buffer->data[addr] = color;
        }
    }
}

/******************************************************************************
function: Clear the color of a window
parameter:
    Xstart : x starting point
    Ystart : Y starting point
    Xend   : x end point
    Yend   : y end point
    Color  : Painted colors
******************************************************************************/
void Paint_ClearWindows(frame_buffer_t *frame_buffer, uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,
                        uint16_t Color)
{
    uint16_t X, Y;
    for (Y = Ystart; Y < Yend; Y++) {
        for (X = Xstart; X < Xend; X++) { // 8 pixel =  1 byte
            Paint_SetPixel(frame_buffer, X, Y, Color);
        }
    }
}

/******************************************************************************
function: Draw Point(Xpoint, Ypoint) Fill the color
parameter:
    Xpoint		: The Xpoint coordinate of the point
    Ypoint		: The Ypoint coordinate of the point
    Color		: Painted color
******************************************************************************/
void Paint_DrawPoint(frame_buffer_t *frame_buffer, uint16_t Xpoint, uint16_t Ypoint, uint16_t Color)
{
    if (Xpoint > frame_buffer->width || Ypoint > frame_buffer->height) {
        return;
    }

    int16_t XDir_Num, YDir_Num;
    for (XDir_Num = 0; XDir_Num < 2 * 1 - 1; XDir_Num++) {
        for (YDir_Num = 0; YDir_Num < 2 * 1 - 1; YDir_Num++) {
            if (Xpoint + XDir_Num - 1 < 0 || Ypoint + YDir_Num - 1 < 0)
                break;
            Paint_SetPixel(frame_buffer, Xpoint + XDir_Num - 1, Ypoint + YDir_Num - 1, Color);
        }
    }
}

/******************************************************************************
function: Draw a line of arbitrary slope
parameter:
    Xstart ：Starting Xpoint point coordinates
    Ystart ：Starting Xpoint point coordinates
    Xend   ：End point Xpoint coordinate
    Yend   ：End point Ypoint coordinate
    Color  ：The color of the line segment
******************************************************************************/
void Paint_DrawLine(frame_buffer_t *frame_buffer, uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,
                    uint16_t Color)
{
    if (Xstart > frame_buffer->width || Ystart > frame_buffer->height || Xend > frame_buffer->width ||
        Yend > frame_buffer->height) {
        return;
    }

    uint16_t Xpoint = Xstart;
    uint16_t Ypoint = Ystart;
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
        Paint_DrawPoint(frame_buffer, Xpoint, Ypoint, Color);
        if (2 * Esp >= dy) {
            if (Xpoint == Xend)
                break;
            Esp += dy;
            Xpoint += XAddway;
        }
        if (2 * Esp <= dx) {
            if (Ypoint == Yend)
                break;
            Esp += dx;
            Ypoint += YAddway;
        }
    }
}

/******************************************************************************
function: Draw a rectangle
parameter:
    Xstart ：Rectangular  Starting Xpoint point coordinates
    Ystart ：Rectangular  Starting Xpoint point coordinates
    Xend   ：Rectangular  End point Xpoint coordinate
    Yend   ：Rectangular  End point Ypoint coordinate
    Color  ：The color of the Rectangular segment
******************************************************************************/
void fb_draw_rectangle(frame_buffer_t *frame_buffer, uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,
                       uint16_t Color)
{
    if (Xstart > frame_buffer->width || Ystart > frame_buffer->height || Xend > frame_buffer->width ||
        Yend > frame_buffer->height) {
        return;
    }

    uint16_t Ypoint;
    for (Ypoint = Ystart; Ypoint < Yend; Ypoint++) {
        Paint_DrawLine(frame_buffer, Xstart, Ypoint, Xend, Ypoint, Color);
    }
}

int Paint_DrawVariableWidthChar(frame_buffer_t *frame_buffer, uint16_t Xpoint, uint16_t Ypoint, const char Acsii_Char,
                                var_width_font_t *Font, uint16_t Color_Foreground, uint16_t Color_Background)
{
    uint16_t Page, Column;

    const var_width_font_entry_t *entry = &Font->table[Acsii_Char - ' '];
    const uint8_t *ptr = entry->table;
    int font_height = Font->Height;
    int font_width = entry->Width;

    for (Page = 0; Page < Font->Height; Page++) {
        for (Column = 0; Column < font_width; Column++) {
            if (*ptr & (0x80 >> (Column % 8))) {
                Paint_SetPixel(frame_buffer, Xpoint + Column, Ypoint + Page, Color_Foreground);
            } else {
                Paint_SetPixel(frame_buffer, Xpoint + Column, Ypoint + Page, Color_Background);
            }
            // One pixel is 8 bits
            if (Column % 8 == 7)
                ptr++;
        } // Write a line
        ptr += Font->Width - font_width / 8; // Move to next line
    } // Write all

    return font_width;
}

void fb_write_string(frame_buffer_t *frame_buffer, uint16_t Xstart, uint16_t Ystart, const char *pString,
                     var_width_font_t *Font, uint16_t Color_Foreground, uint16_t Color_Background)
{
    uint16_t Xpoint = Xstart;
    uint16_t Ypoint = Ystart;

    if (Xstart > frame_buffer->width || Ystart > frame_buffer->height) {
        return;
    }

    int width = 0;
    while (*pString != '\0') {
        // if X direction filled , reposition to(Xstart,Ypoint),Ypoint is Y
        // direction plus the Height of the character
        if ((Xpoint + width) > frame_buffer->width) {
            Xpoint = Xstart;
            Ypoint += Font->Height;
        }

        // If the Y direction is full, reposition to(Xstart, Ystart)
        if ((Ypoint + Font->Height) > frame_buffer->height) {
            Xpoint = Xstart;
            Ypoint = Ystart;
        }
        width = Paint_DrawVariableWidthChar(frame_buffer, Xpoint, Ypoint, *pString, Font, Color_Foreground,
                                            Color_Background);

        // The next character of the address
        pString++;

        Xpoint += width;
    }
}