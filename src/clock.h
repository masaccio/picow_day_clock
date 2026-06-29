#pragma once

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

#include "config.h"

#ifdef CLOCK_DEBUG_ENABLED
#ifdef TEST_MODE
extern int test_printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
#define CLOCK_DEBUG(...) test_printf(__VA_ARGS__)
#else
#define CLOCK_DEBUG(format, ...) printf("CLOCK: " format, ##__VA_ARGS__)
#endif // #if TEST_MODE
#else
#define CLOCK_DEBUG(...) ((void)0)
#endif

#define STATUS_CASE(STATUS)                                                                                            \
    case STATUS:                                                                                                       \
        return #STATUS;

// Whilst the display is capable of many more colors, we limit our bitmaps to 4-bit
// color to save memory
typedef enum
{
    BLACK = 0x00,
    RED = 0x01,
    GREEN = 0x02,
    CYAN = 0x03
} color_t;

#define BG_COLOR BLACK
#define FG_COLOR GREEN

typedef enum
{
    WIFI_OK,            // Everything is OK
    WIFI_INIT_ERROR,    // Memory allocation failed
    WIFI_TIMEOUT_ERROR, // Too many Wi-Fi timeouts
    WIFI_AUTH_ERROR,    // Too many authentication errors
    WIFI_CONNECT_ERROR, // A TCP/IP connection error
    WIFI_UNKNOWN_ERROR, // Any other Wi-Fi related error
} wifi_error_t;

typedef enum
{
    NTP_OK,
    NTP_INIT_ERROR,
    NTP_DNS_ERROR,
    NTP_TIMEOUT_ERROR,
    NTP_PROTOCOL_ERROR,
    NTP_MEMORY_ERROR,
} ntp_error_t;

typedef enum
{
    NTP_DONE,    // NTP lookup completed successfully or with an error
    NTP_KOD,     // Server sent 'kiss of death' to tell us to back off
    NTP_PENDING, // Waiting for NTP response up until timeout
} ntp_status_t;

typedef enum
{
    WATCHDOG_OK,
    WATCHDOG_RESET,
    WATCHDOG_NTP,
    WATCHDOG_WIFI,
} watchdog_error_t;

typedef enum
{
    DST_RULE_NONE = 0,
    DST_RULE_NA,
    DST_RULE_EU,
    DST_RULE_AU,
    DST_RULE_NZ,
    DST_RULE_CL,
    DST_RULE_IL
} dst_rule_t;

#define CONFIG_MAGIC ((uint32_t)'C' << 24 | (uint32_t)'L' << 16 | (uint32_t)'O' << 8 | (uint32_t)'K')
#define FLASH_TARGET_OFFSET (1024 * 1024)
#ifndef FLASH_CONFIG_ADDR // defined in mock.h for tests
#define FLASH_CONFIG_ADDR XIP_BASE + FLASH_TARGET_OFFSET
#endif

typedef struct
{
    uint32_t magic_marker;
    uint32_t boot_count;
    watchdog_error_t watchdog_error;
    ntp_error_t ntp_error;
    wifi_error_t wifi_error;
} persistent_state_t;

typedef struct
{
    uint32_t magic_marker;
    char wifi_ssid[WIFI_SSID_MAX_LEN + 1];
    char wifi_password[WIFI_PASSWORD_MAX_LEN + 1];
    char ntp_server[HOSTNAME_MAX_LEN + 1];
    int16_t tz_offset_mins;
    dst_rule_t dst_rule;
    int ntp_timeout;
} clock_config_t;

typedef int (*store_config_handler_t)(clock_config_t *config, int invalidate);

typedef struct lcd_state_t
{
    // GPIO config
    uint16_t RST_gpio;
    uint16_t DC_gpio;
    uint16_t BL_gpio;
    uint16_t CS_gpio;
    uint16_t CLK_gpio;
    uint16_t MOSI_gpio;
    // Additional config
    uint16_t width;
    uint16_t height;
} lcd_state_t;

typedef void (*ntp_time_handler_t)(void *state, time_t *time);

typedef struct ntp_state_t
{
    ntp_time_handler_t time_handler;
    ip_addr_t ntp_server_address;
    struct udp_pcb *ntp_pcb;
    void *parent_state;
    ntp_error_t error;
    ntp_status_t status;
} ntp_state_t;

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
    int ntp_interval;

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
    int first_clock_tick;    // Forces display update on first tick
    int wifi_is_initialized; // Ensures cyw43_arch_init() is not called multiple times
} clock_state_t;

typedef struct
{
    struct tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
    store_config_handler_t store_config;
    int active_connections;
} tcp_server_t;

typedef struct
{
    struct tcp_pcb *pcb;
    int sent_len;
    char headers[TCP_IP_BUFFER_SIZE];
    char result[TCP_IP_BUFFER_SIZE];
    int header_len;
    int result_len;
    ip_addr_t *gw;
    store_config_handler_t store_config;
    tcp_server_t *server_state;
} tcp_connect_state_t;

// Clock functions and state that are shared with the test harness
extern const char *watchdog_error_to_string(watchdog_error_t status);
extern const char *ntp_error_to_string(ntp_error_t status);
extern const char *wifi_error_to_string(wifi_error_t status);
extern err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err);
extern persistent_state_t persistent_state;
extern int day_of_week(int day, int month, int year);

// Callbacks that are called by modules
extern bool clock_timer_callback(repeating_timer_t *);
extern void ntp_timer_callback(void *state, time_t *ntp_time);
extern int config_store_handler(clock_config_t *config, int invalidate);
extern void clock_task(clock_state_t *state);

extern void on_clock_alloc_failed(void);

extern void on_lcd_init_failed(clock_state_t *state, unsigned int lcd_num);

extern wifi_error_t connect_to_wifi(clock_state_t *clock_state, const char ssid[], const char password[]);

extern wifi_error_t start_wifi_access_point(clock_state_t *clock_state, store_config_handler_t store_config);

extern lcd_state_t *lcd_init(uint16_t RST_gpio, uint16_t DC_gpio, uint16_t BL_gpio, uint16_t CS_gpio, uint16_t CLK_gpio,
                             uint16_t MOSI_gpio, int reset);

extern void lcd_set_backlight(lcd_state_t *state, uint8_t level);

extern void lcd_init_peripherals(lcd_state_t *state, int reset);

void lcd_update_icons(lcd_state_t *state, watchdog_error_t watchdog_error, ntp_error_t ntp_error,
                      wifi_error_t wifi_error);

extern void lcd_print_line(lcd_state_t *state, uint16_t line_num, color_t color, const char *buffer);

extern void lcd_print_clock_digit(lcd_state_t *state, color_t color, const char ascii_char);

extern void lcd_draw_rectangle(lcd_state_t *state, uint16_t x_start, uint16_t y_start, uint16_t width, uint16_t height,
                               uint16_t color);

void lcd_clear_screen(lcd_state_t *state, uint16_t color);

extern int time_is_dst(time_t utc_now, clock_state_t *state);

extern time_t tm_to_epoch(struct tm *tm);

extern const char *time_as_string(time_t t, clock_state_t *state);

extern void ntp_request(ntp_state_t *state);

extern ntp_state_t *ntp_init(void *parent_state, ntp_time_handler_t time_handler);

extern ntp_error_t ntp_get_time(ntp_state_t *ntp_state);

extern void __attribute__((noreturn)) fatal_reset(clock_state_t *state, ntp_error_t ntp_error, wifi_error_t wifi_error);

clock_state_t *clock_init(void);

extern int clock_start(clock_state_t *);
