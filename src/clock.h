#ifndef _CLOCK_H
#define _CLOCK_H

#include <stdlib.h>

// Pico SDK
#ifndef TEST_MODE
#include "lwip/ip_addr.h"
#include "pico/stdlib.h"
#else
#include "mock.h"
#endif

#include "config.h"
#include "fb.h"

#ifdef CLOCK_DEBUG_ENABLED
#ifdef TEST_MODE
extern int test_printf(const char *format, ...);
#define CLOCK_DEBUG(...) test_printf(__VA_ARGS__)
#else
#define CLOCK_DEBUG(format, ...) printf("CLOCK: " format, ##__VA_ARGS__)
#endif // #if TEST_MODE
#else
#define CLOCK_DEBUG(...) ((void)0)
#endif

typedef enum
{
    WIFI_STATUS_SUCCESS = 0,
    WIFI_STATUS_INIT_FAIL = -1,
    WIFI_STATUS_TIMEOUT = -2,
    WIFI_STATUS_BAD_AUTH = -3,
    WIFI_STATUS_CONNECT_FAILED = -4,
    WIFI_STATUS_UNKNOWN_ERROR = -5,
} wifi_status_t;

typedef enum
{
    STATUS_WIFI_OK = 0,
    STATUS_NTP_OK = 1,
    STATUS_WIFI_INIT = -1,
    STATUS_WIFI_TIMEOUT = -2,
    STATUS_WIFI_AUTH = -3,
    STATUS_WIFI_CONNECT = -4,
    STATUS_WIFI_ERROR = -5,
    STATUS_NTP_INIT = -6,
    STATUS_NTP_DNS = -7,
    STATUS_NTP_TIMEOUT = -8,
    STATUS_NTP_MEMORY = -9,
    STATUS_NTP_INVALID = -10,
    STATUS_WATCHDOG_RESET = -11,
    STATUS_NONE = 0xff,
} clock_status_t;

typedef enum
{
    NTP_STATUS_PENDING = 0,
    NTP_STATUS_SUCCESS = 1,
    NTP_STATUS_DNS_ERROR = -1,
    NTP_STATUS_TIMEOUT = -2,
    NTP_STATUS_INVALID_RESPONSE = -3,
    NTP_STATUS_MEMORY_ERROR = -4,
    NTP_STATUS_KOD = -5,
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
    // Frame buffer
    frame_buffer_t *fb;
} lcd_state_t;

typedef void (*ntp_time_handler_t)(void *state, time_t *time);

typedef struct ntp_state_t
{
    ip_addr_t ntp_server_address;
    bool dns_request_sent;
    struct udp_pcb *ntp_pcb;
    void *parent_state;
    ntp_status_t status;
    ntp_time_handler_t time_handler;
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
    bool init_done;
    repeating_timer_t timer;
    clock_status_t last_reset_error;
} clock_state_t;

extern wifi_status_t connect_to_wifi(const char ssid[], const char password[]);

extern lcd_state_t *lcd_init(uint16_t RST_gpio, uint16_t DC_gpio, uint16_t BL_gpio, uint16_t CS_gpio, uint16_t CLK_gpio,
                             uint16_t MOSI_gpio, bool reset);

extern void lcd_set_backlight(lcd_state_t *state, uint8_t level);

extern void lcd_init_peripherals(lcd_state_t *state, bool reset);

void lcd_update_icon(lcd_state_t *state, clock_status_t status, bool is_error);

extern void lcd_print_line(lcd_state_t *state, uint16_t line_num, color_t color, const char *buffer);

extern void lcd_print_clock_digit(lcd_state_t *state, color_t color, const char ascii_char);

extern void lcd_clear_screen(lcd_state_t *state, color_t color);

void lcd_update_screen(lcd_state_t *state);

extern bool time_is_dst(struct tm *utc);

extern time_t tm_to_epoch(struct tm *tm);

extern const char *time_as_string(time_t ntp_time);

extern void ntp_request(ntp_state_t *state);

extern ntp_state_t *ntp_init(void *parent_state, ntp_time_handler_t time_handler);

extern ntp_status_t ntp_get_time(ntp_state_t *ntp_state);

#endif // _CLOCK_H
