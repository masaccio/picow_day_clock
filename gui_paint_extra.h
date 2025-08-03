#include "fonts.h"
#include "DEV_Config.h"

// Additional string functions not present in the original library
int Paint_DrawVariableWidthChar(UWORD Xpoint, UWORD Ypoint,
                                const char Acsii_Char, vFONT *Font,
                                UWORD Color_Foreground, UWORD Color_Background);

void Paint_DrawVariableWidthString(UWORD Xstart, UWORD Ystart,
                                   const char *pString, vFONT *Font,
                                   UWORD Color_Foreground,
                                   UWORD Color_Background);