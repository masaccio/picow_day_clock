#include "fonts.h"
#include <stdint.h>

typedef struct _vFontEntry {
    const uint8_t Width;
    const uint8_t *table;
} vFONTENTRY;

typedef struct _vFont {
    const vFONTENTRY *table;
    uint16_t Width;
    uint16_t Height;
} vFONT;

extern vFONT Roboto_Medium24;
extern vFONT Roboto_Medium200;