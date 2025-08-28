#ifndef _GUI_PAINT_EXTRA_H
#define _GUI_PAINT_EXTRA_H

/* Waveshare SDK */
#include "DEV_Config.h"
#include "fonts.h"

/* Local includes */
#include "fonts_extra.h"

// Additional string functions not present in the original library
int Paint_DrawVariableWidthChar(UWORD Xpoint, UWORD Ypoint, const char Acsii_Char, vFONT *Font, UWORD Color_Foreground,
                                UWORD Color_Background);

void Paint_DrawVariableWidthString(UWORD Xstart, UWORD Ystart, const char *pString, vFONT *Font, UWORD Color_Foreground,
                                   UWORD Color_Background);

#endif /* _GUI_PAINT_EXTRA_H */