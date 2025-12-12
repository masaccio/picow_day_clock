#ifndef CLOCK_H
#define CLOCK_H

#include <stdlib.h>

// Pico SDK
#ifndef TEST_MODE
#include "lwip/ip_addr.h"
#include "pico/stdlib.h"
#else
#include "mock.h"
#endif

#include "config.h"

#ifdef TEST_MODE
extern int test_printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
#endif

#ifdef CLOCK_DEBUG_ENABLED
#ifdef TEST_MODE
#define CLOCK_DEBUG(...) test_printf(__VA_ARGS__)
#else
#define CLOCK_DEBUG(format, ...) printf("CLOCK: " format, ##__VA_ARGS__)
#endif // #if TEST_MODE
#else
#define CLOCK_DEBUG(...) ((void)0)
#endif

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
    WIFI_STATUS_SUCCESS,
    WIFI_STATUS_INIT_FAIL,
    WIFI_STATUS_TIMEOUT,
    WIFI_STATUS_BAD_AUTH,
    WIFI_STATUS_CONNECT_FAILED,
    WIFI_STATUS_UNKNOWN_ERROR,
} wifi_status_t;

typedef enum
{
    STATUS_NONE,
    STATUS_WIFI_OK,
    STATUS_NTP_OK,
    STATUS_WIFI_INIT,
    STATUS_WIFI_TIMEOUT,
    STATUS_WIFI_AUTH,
    STATUS_WIFI_CONNECT,
    STATUS_WIFI_ERROR,
    STATUS_NTP_INIT,
    STATUS_NTP_DNS,
    STATUS_NTP_TIMEOUT,
    STATUS_NTP_MEMORY,
    STATUS_NTP_INVALID,
    STATUS_WATCHDOG_RESET,
    STATUS_WATCHDOG_RESET_FOR_WIFI,
    STATUS_WATCHDOG_RESET_FOR_NTP,
} clock_status_t;

typedef enum
{
    NTP_STATUS_PENDING,
    NTP_STATUS_SUCCESS,
    NTP_STATUS_DNS_ERROR,
    NTP_STATUS_TIMEOUT,
    NTP_STATUS_INVALID_RESPONSE,
    NTP_STATUS_MEMORY_ERROR,
    NTP_STATUS_KOD,
} ntp_status_t;

typedef struct
{
    uint32_t boot_count;
    clock_status_t reset_error;
} persistent_state_t;

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
    int dns_request_sent;
    ntp_status_t status;
} ntp_state_t;

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
    char current_lcd_digits[NUM_LCDS + 1];
    // Timer state
    repeating_timer_t timer;
    clock_status_t last_reset_error;
    int init_done;
} clock_state_t;

// Clock functions and state that are shared with the test harness
extern const char *status_to_string(clock_status_t status);
extern persistent_state_t persistent_state;
extern int last_day_of_month(int day, int month, int year);
extern bool clock_timer_callback(repeating_timer_t *);
extern void ntp_timer_callback(void *state, time_t *ntp_time);

// Cross-module functions
extern wifi_status_t connect_to_wifi(const char ssid[], const char password[]);

extern lcd_state_t *lcd_init(uint16_t RST_gpio, uint16_t DC_gpio, uint16_t BL_gpio, uint16_t CS_gpio, uint16_t CLK_gpio,
                             uint16_t MOSI_gpio, int reset);

extern void lcd_set_backlight(lcd_state_t *state, uint8_t level);

extern void lcd_init_peripherals(lcd_state_t *state, int reset);

void lcd_update_icon(lcd_state_t *state, clock_status_t status, int is_error);

extern void lcd_print_line(lcd_state_t *state, uint16_t line_num, color_t color, const char *buffer);

extern void lcd_print_clock_digit(lcd_state_t *state, color_t color, const char ascii_char);

extern void lcd_draw_rectangle(lcd_state_t *state, uint16_t x_start, uint16_t y_start, uint16_t width, uint16_t height,
                               uint16_t color);

void lcd_clear_screen(lcd_state_t *state, uint16_t color);

extern int time_is_dst(struct tm *utc);

extern time_t tm_to_epoch(struct tm *tm);

extern const char *time_as_string(time_t ntp_time);

extern void ntp_request(ntp_state_t *state);

extern ntp_state_t *ntp_init(void *parent_state, ntp_time_handler_t time_handler);

extern ntp_status_t ntp_get_time(ntp_state_t *ntp_state);

#endif // CLOCK_H
