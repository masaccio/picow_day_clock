#ifndef _FONT_H
#define _FONT_H

#include <stdint.h>

typedef struct
{
    const uint8_t width;
    const uint8_t *table;
} var_width_font_entry_t;

typedef struct
{
    const var_width_font_entry_t *table;
    uint16_t width;
    uint16_t height;
} var_width_font_t;

extern var_width_font_t text_font;
extern var_width_font_t clock_digit_font;

#endif /* _FONT_H */