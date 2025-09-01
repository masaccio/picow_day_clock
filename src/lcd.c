#include <stdarg.h>

/* Local includes */
#include "gui_paint_extra.h"
#include "lcd.h"

static UBYTE lcd_frame_buffer[(LCD_HEIGHT + 1) * LCD_WIDTH * 2];

lcd_state_t *lcd_init(LCD_GPIO_Config pins)
{
    lcd_state_t *lcd_state = (lcd_state_t *)calloc(1, sizeof(lcd_state_t));

    sleep_ms(100); /* TODO: is this required? */

    if (DEV_Module_Init(pins) != 0) {
        return NULL;
    }

    lcd_state->frame_buffer = (UWORD *)lcd_frame_buffer;
    lcd_state->y_offset = 0;
    lcd_state->pins = pins;

    LCD_1IN47_Init(pins, VERTICAL);
    DEV_SET_PWM(0);

    Paint_NewImage((UBYTE *)lcd_state->frame_buffer, LCD_WIDTH, LCD_HEIGHT, 0, BGCOLOR);
    Paint_SetScale(65);
    Paint_Clear(BGCOLOR);
    Paint_SetRotate(ROTATE_90);

    DEV_SET_PWM(100);

    return lcd_state;
}

void print_line(lcd_state_t *state, const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Paint_DrawVariableWidthString(0, state->y_offset, buffer, &TEXT_FONT, FGCOLOR, BGCOLOR);
    state->y_offset += TEXT_FONT_HEIGHT + 2;
}

void clear_screen(lcd_state_t *state)
{
    Paint_Clear(BGCOLOR);
    state->y_offset = 0;
    LCD_1IN47_Display(state->pins, state->frame_buffer);
}

void update_screen(lcd_state_t *state)
{
    LCD_1IN47_Display(state->pins, state->frame_buffer);
}