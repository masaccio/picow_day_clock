#include <stdarg.h>

/* Local includes */
#include "gui_paint_extra.h"
#include "lcd.h"

static UWORD lcd_frame_buffer[LCD_HEIGHT * LCD_WIDTH];

lcd_state_t *lcd_init(void) {
    lcd_state_t *lcd_state = (lcd_state_t *)calloc(1, sizeof(lcd_state_t));

    if (DEV_Module_Init() != 0) {
        return NULL;
    }

    lcd_state->frame_buffer = lcd_frame_buffer;
    lcd_state->y_offset = 0;

    LCD_1IN47_Init(VERTICAL);
    LCD_1IN47_Clear(BGCOLOR);
    DEV_SET_PWM(0);

    Paint_NewImage((UBYTE *)lcd_state->frame_buffer, LCD_WIDTH, LCD_HEIGHT, 0, BGCOLOR);
    Paint_SetScale(65);
    Paint_Clear(BGCOLOR);

    DEV_SET_PWM(100);
    return lcd_state;
}

void print_line(lcd_state_t *state, const char *format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Paint_DrawVariableWidthString(0, state->y_offset, buffer, &TEXT_FONT, FGCOLOR, BGCOLOR);
    state->y_offset += TEXT_FONT_HEIGHT + 2;
}

void clear_screen(lcd_state_t *state) {
    Paint_Clear(BGCOLOR);
    state->y_offset = 0;
    LCD_1IN47_Display(state->frame_buffer);
}
