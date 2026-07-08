#pragma once

#include <stdint.h>
#include <stdlib.h>

// Pico SDK
#ifndef TEST_MODE
#include "hardware/flash.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "pico/stdlib.h"
#else
#include "mock.h"
#endif

#include "clock_config.h"
#include "config.h"
#include "lcd.h"
#include "ntp.h"
#include "watchdog.h"
#include "wifi.h"

typedef struct clock_state_t
{
    // Asynchronously modified state
    volatile time_t ntp_time;
    volatile int clock_tick_updated;
    volatile int ntp_time_updated;
    volatile uint ap_mode_triggered;

    // NTP state
    ntp_state_t *ntp_state;
    time_t ntp_last_sync;
    uint32_t ntp_interval;

    // LCD state
    lcd_state_t *lcd_states[NUM_LCDS];
    char current_lcd_digits[NUM_LCDS + 1];

    // Timer state
    repeating_timer_t timer;
    clock_config_t clock_config;

    // Why watchdog reset happened
    int cold_boot;
    time_t last_watchdog_error_time;
    watchdog_error_t watchdog_reset_error;
    ntp_error_t ntp_reset_error;
    wifi_error_t wifi_reset_error;

    // Miscellaneous
    int first_clock_tick;  // Forces display update on first tick
    bool wifi_initialized; // Ensures cyw43_arch_init() is not called multiple times
} clock_state_t;

// Clock functions and state that are shared with the test harness
extern err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err);
extern persistent_state_t persistent_state;
extern int day_of_week(int day, int month, int year);

// Callbacks that are called by modules
extern bool clock_timer_callback(repeating_timer_t *);
extern void ntp_timer_callback(void *state, time_t *ntp_time);
extern int config_store_handler(void *config, bool invalidate);
extern void clock_task(clock_state_t *state);

extern void on_clock_alloc_failed(void);

extern void on_lcd_init_failed(clock_state_t *state, unsigned int lcd_num);

extern int time_is_dst(time_t utc_now, clock_state_t *state);

extern time_t tm_to_epoch(struct tm *tm);

extern const char *time_as_string(time_t t, clock_state_t *state);

#ifdef TEST_MODE
extern void fatal_reset(clock_state_t *state, ntp_error_t ntp_error, wifi_error_t wifi_error);
#else
extern void __attribute__((noreturn)) fatal_reset(clock_state_t *state, ntp_error_t ntp_error, wifi_error_t wifi_error);
#endif

clock_state_t *clock_init(void);

extern int clock_start(clock_state_t *);

extern void reboot(void);
