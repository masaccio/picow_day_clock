/* Waveshare SDK includes */
#ifndef _LCD_H
#define _LCD_H

#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "LCD_1in47.h"

typedef struct lcd_state_t {
    UWORD *frame_buffer;
    int y_offset;
} lcd_state_t;

extern lcd_state_t *init_lcd(void);

extern void print_line(lcd_state_t *state, const char *format, ...);

extern void clear_screen(lcd_state_t *state);

#define LCD_HEIGHT LCD_1IN47_HEIGHT
#define LCD_WIDTH LCD_1IN47_WIDTH
#define BGCOLOR BLACK
#define FGCOLOR WHITE
#define TEXT_FONT Roboto_Medium24
#define TEXT_FONT_HEIGHT 24
#define DIGIT_FONT Roboto_Medium200

#endif /* _LCD_H */