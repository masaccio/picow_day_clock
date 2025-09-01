#ifndef _LCD_H
#define _LCD_H

#include <time.h>

/* Waveshare SDK */
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "LCD_1in47.h"

/* Local includes */
#include "fonts_extra.h"

#define LCD_HEIGHT LCD_1IN47_HEIGHT
#define LCD_WIDTH LCD_1IN47_WIDTH
#define BGCOLOR BLACK
#define FGCOLOR WHITE
#define TEXT_FONT Roboto_Medium24
#define TEXT_FONT_HEIGHT 24
#define DIGIT_FONT Roboto_Medium200

typedef struct lcd_state_t
{
    UWORD *frame_buffer;
    int y_offset;
    LCD_GPIO_Config pins;
} lcd_state_t;

extern lcd_state_t *lcd_init(LCD_GPIO_Config pins);

extern void print_line(lcd_state_t *state, const char *format, ...);

extern void clear_screen(lcd_state_t *state);

void update_screen(lcd_state_t *state);

#endif /* _LCD_H */