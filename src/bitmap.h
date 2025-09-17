#ifndef _BITMAP_H
#define _BITMAP_H

#include <stdint.h>

typedef struct
{
    const uint8_t width;
    const uint8_t *table;
} font_glyph_t;

typedef struct
{
    const font_glyph_t *table;
    uint16_t byte_width;
    uint16_t height;
} font_t;

extern font_t text_font;
extern font_t clock_digit_font;

#define ICON_SIZE 24

extern const uint8_t *dns_icon;
extern const uint8_t *ntp_icon;
extern const uint8_t *wifi_icon;

#endif // _BITMAP_H
