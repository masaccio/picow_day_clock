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

extern const unsigned char *dns_icon;
extern const unsigned char *ntp_icon;
extern const unsigned char *wifi_icon;

#endif // _BITMAP_H
