#ifndef _FONT_H
#define _FONT_H

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

#endif /* _FONT_H */
