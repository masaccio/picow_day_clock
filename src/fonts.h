#ifndef _FONTS_H
#define _FONTS_H

#include <stdint.h>

typedef struct
{
    const uint8_t Width;
    const uint8_t *table;
} var_width_font_entry_t;

typedef struct
{
    const var_width_font_entry_t *table;
    uint16_t Width;
    uint16_t Height;
} var_width_font_t;

extern var_width_font_t Roboto_Medium24;
extern var_width_font_t Roboto_Medium200;

#endif /* _FONTS_H */