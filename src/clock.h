
#ifndef _CLOCK_H
#define _CLOCK_H

#if CLOCK_DEBUG_ENABLED
#if TEST_MODE
extern int test_printf(const char *format, ...);
#define CLOCK_DEBUG(...) test_printf(__VA_ARGS__)
#else
#define CLOCK_DEBUG(...) printf(__VA_ARGS__)
#endif // #if TEST_MODE
#else
#define CLOCK_DEBUG(...) ((void)0)
#endif

#include "lcd.h"
#include "ntp.h"

#define LCD1_GPIO_DC 6
#define LCD1_GPIO_CS 7
#define LCD2_GPIO_DC 8
#define LCD2_GPIO_CS 9
#define LCD3_GPIO_DC 2
#define LCD3_GPIO_CS 3
#define LCD4_GPIO_DC 4
#define LCD4_GPIO_CS 5
#define LCD5_GPIO_DC 14
#define LCD5_GPIO_CS 15
#define LCD6_GPIO_DC 16
#define LCD6_GPIO_CS 17
#define LCD7_GPIO_DC 18
#define LCD7_GPIO_CS 19
#define LCD_GPIO_RST 12
#define LCD_GPIO_BL 13
#define LCD_GPIO_CLK 10
#define LCD_GPIO_MOSI 11

typedef struct clock_state_t
{ // NTP state
    ntp_state_t *ntp_state;
    time_t ntp_time;
    time_t ntp_last_sync;
    int ntp_interval;
    int ntp_drift;
    // LCD state
    lcd_state_t *lcd_states[NUM_LCDS];
    char current_lcd_digits[NUM_LCDS + 1];
    // Timer state
    bool init_done;
    repeating_timer_t timer;
} clock_state_t;

extern time_t tm_to_epoch(struct tm *tm);

#endif // _CLOCK_H
