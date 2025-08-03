#include "DEV_Config.h"
#include "Debug.h"
#include "GUI_Paint.h"

int Paint_DrawVariableWidthChar(UWORD Xpoint, UWORD Ypoint,
                                const char Acsii_Char, vFONT *Font,
                                UWORD Color_Foreground,
                                UWORD Color_Background) {
  UWORD Page, Column;

  const vFONTENTRY *entry = &Font->table[Acsii_Char - ' '];
  const uint8_t *ptr = entry->table;
  int font_height = Font->Height;
  int font_width = entry->Width;

  Debug("Paint_DrawVariableWidthChar: character '%c', font_height=%d\r\n",
        Acsii_Char, font_height);

  for (Page = 0; Page < Font->Height; Page++) {
    for (Column = 0; Column < font_width; Column++) {
      // To determine whether the font background color and screen background
      // color is consistent
      if (FONT_BACKGROUND ==
          Color_Background) { // this process is to speed up the scan
        if (*ptr & (0x80 >> (Column % 8))) {
          Debug("Paint_DrawVariableWidthChar: [%d,%d] 0x%04x\r\n",
                Xpoint + Column, Ypoint + Page, Color_Foreground);
          Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
        }
      } else {
        if (*ptr & (0x80 >> (Column % 8))) {
          Debug("Paint_DrawVariableWidthChar: [%d,%d] 0x%04x\r\n",
                Xpoint + Column, Ypoint + Page, Color_Foreground);
          Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
        } else {
          Debug("Paint_DrawVariableWidthChar: [%d,%d] 0x%04x\r\n",
                Xpoint + Column, Ypoint + Page, Color_Background);
          Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Background);
        }
      }
      // One pixel is 8 bits
      if (Column % 8 == 7)
        ptr++;
    } // Write a line
    ptr += Font->Width - font_width / 8; // Move to next line
  } // Write all

  Debug("Paint_DrawVariableWidthChar: last width %d\n", font_width);
  return font_width;
}

void Paint_DrawVariableWidthString(UWORD Xstart, UWORD Ystart,
                                   const char *pString, vFONT *Font,
                                   UWORD Color_Foreground,
                                   UWORD Color_Background) {
  UWORD Xpoint = Xstart;
  UWORD Ypoint = Ystart;

  Debug("Paint_DrawVariableWidthString: starting\r\n");

  if (Xstart > Paint.Width || Ystart > Paint.Height) {
    Debug("Paint_DrawVariableWidthString Input exceeds the normal display "
          "range\r\n");
    return;
  }

  int width = 0;
  while (*pString != '\0') {
    // if X direction filled , reposition to(Xstart,Ypoint),Ypoint is Y
    // direction plus the Height of the character
    if ((Xpoint + width) > Paint.Width) {
      Xpoint = Xstart;
      Ypoint += Font->Height;
    }

    Debug("Paint_DrawVariableWidthString: character '%c'\r\n", *pString);
    // If the Y direction is full, reposition to(Xstart, Ystart)
    if ((Ypoint + Font->Height) > Paint.Height) {
      Xpoint = Xstart;
      Ypoint = Ystart;
    }
    width = Paint_DrawVariableWidthChar(Xpoint, Ypoint, *pString, Font,
                                        Color_Foreground, Color_Background);

    // The next character of the address
    pString++;

    Xpoint += width;
  }
}