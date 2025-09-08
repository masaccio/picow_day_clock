
#ifndef _CLOCK_H
#define _CLOCK_H

#ifdef CLOCK_DEBUG_ENABLED
#define CLOCK_DEBUG(...) printf(__VA_ARGS__)
#else
#define CLOCK_DEBUG(...) ((void)0)
#endif

#include "lcd.h"
#include "ntp.h"

typedef struct clock_state_t
{
    // NTP state
    ntp_state_t *ntp_state;
    time_t ntp_time;
    time_t ntp_last_sync;
    int ntp_interval;
    int ntp_drift;
    // LCD state
    lcd_state_t *lcd_states[NUM_LCDS];
    char current_lcd_digits[NUM_LCDS];
    // Timer state
    repeating_timer_t timer;
} clock_state_t;

extern time_t tm_to_epoch(struct tm *tm);

#endif // _CLOCK_H
