#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "clock.h"
#include "mock.h"
#include "test.h"

// LCD functions
extern void mock_update_icons(watchdog_error_t watchdog_error, ntp_error_t ntp_error, wifi_error_t wifi_error);
extern lcd_state_t *mock_lcd_init(uint16_t RST_gpio, uint16_t DC_gpio, uint16_t BL_gpio, uint16_t CS_gpio,
                                  uint16_t CLK_gpio, uint16_t MOSI_gpio, int reset);
extern void mock_reset_icons(void);

static watchdog_error_t last_watchdog_error = (watchdog_error_t)0xff;
static ntp_error_t last_ntp_error = (ntp_error_t)0xff;
static wifi_error_t last_wifi_error = (wifi_error_t)0xff;

void mock_reset_icons(void)
{
    last_watchdog_error = (watchdog_error_t)0xff;
    last_ntp_error = (ntp_error_t)0xff;
    last_wifi_error = (wifi_error_t)0xff;
}

void lcd_print_line(lcd_state_t *state, uint16_t line_num, color_t color, const char *buffer)
{
    (void)state;
    (void)line_num;
    (void)color;
    (void)buffer;
    mock_printf("%s\n", buffer);
}

void lcd_clear_screen(lcd_state_t *state, uint16_t color)
{
    (void)state;
    (void)color;
}

void lcd_print_clock_digit(lcd_state_t *state, color_t color, const char ascii_char)
{
    (void)state;
    (void)color;
    (void)ascii_char;
}

void lcd_update_icons(lcd_state_t *state, watchdog_error_t wd, ntp_error_t ntp, wifi_error_t wifi)
{
    (void)state;

    // 1. Maintain easy assertions (The Current State)
    mock_ctx.spy.icon_state.watchdog = wd;
    mock_ctx.spy.icon_state.ntp = ntp;
    mock_ctx.spy.icon_state.wifi = wifi;

    // 2. Maintain the debug ledger (The Sequence)
    icon_event_t event = {.timestamp_ms = mock_ctx.spy.system_time_ms, .watchdog = wd, .ntp = ntp, .wifi = wifi};
    icon_queue_t_push(&mock_ctx.spy.icon_history, event);
}

extern lcd_state_t *lcd_init(uint16_t RST_gpio, uint16_t DC_gpio, uint16_t BL_gpio, uint16_t CS_gpio, uint16_t CLK_gpio,
                             uint16_t MOSI_gpio, int reset)
{
    (void)RST_gpio;
    (void)DC_gpio;
    (void)BL_gpio;
    (void)CS_gpio;
    (void)CLK_gpio;
    (void)MOSI_gpio;
    (void)reset;

    lcd_state_t *state = (lcd_state_t *)calloc(1, sizeof(lcd_state_t));
    if (!state) {
        return NULL;
    }
    return state;
}

// These system calls have been redefined in mock.h so undef them here
// so that we can call into the system libraries as needed.
#undef printf
#undef calloc
#undef free

// SPI functions
uint spi_init(spi_inst_t *spi, int baudrate)
{
    (void)spi;
    (void)baudrate;
    return (uint)0;
}

int spi_write_blocking(spi_inst_t *spi, const uint8_t *src, size_t len)
{
    (void)spi;
    (void)src;
    (void)len;
    return 0;
}

// GPIO functions
void gpio_set_function(uint gpio, uint func)
{
    (void)gpio;
    (void)func;
}

void gpio_init(uint gpio)
{
    (void)gpio;
}

void gpio_set_dir(uint gpio, int out)
{
    (void)gpio;
    (void)out;
}

void gpio_put(uint gpio, int value)
{
    (void)gpio;
    (void)value;
}

void gpio_pull_up(uint gpio)
{
    (void)gpio;
}

int gpio_get(uint gpio)
{
    if (gpio == CONFIG_BUTTON_GPIO)
        // 0 means pressed due to pull-up
        return mock_ctx.inject.config_button_pressed == 0;
    return 0;
}

// PWM functions
int pwm_gpio_to_slice_num(uint gpio)
{
    (void)gpio;
    return 0;
}

void pwm_set_wrap(uint slice_num, uint32_t wrap)
{
    (void)slice_num;
    (void)wrap;
}

void pwm_set_chan_level(uint slice_num, uint chan, uint32_t level)
{
    (void)slice_num;
    (void)chan;
    (void)level;
}

void pwm_set_clkdiv(uint slice_num, float divider)
{
    (void)slice_num;
    (void)divider;
}

void pwm_set_enabled(uint slice_num, int enabled)
{
    (void)slice_num;
    (void)enabled;
}

// Wi-Fi functions
int cyw43_arch_init(void)
{
    return mock_ctx.inject.cyw43_arch_init_fail ? 1 : 0;
}

void cyw43_arch_enable_sta_mode(void)
{
}

void cyw43_arch_enable_ap_mode(const char *ssid, const char *password, uint32_t auth)
{
    (void)ssid;
    (void)password;
    (void)auth;
}

void cyw43_arch_disable_ap_mode(void)
{
}

void cyw43_arch_disable_sta_mode(void)
{
}

void cyw43_arch_lwip_begin(void)
{
}

void cyw43_arch_lwip_end(void)
{
}

int cyw43_arch_wifi_connect_timeout_ms(const char *ssid, const char *password, uint auth, uint timeout_ms)
{
    (void)ssid;
    (void)password;
    (void)auth;
    (void)timeout_ms;
    if (mock_ctx.inject.cyw43_auth_error_count > 0) {
        mock_ctx.inject.cyw43_auth_error_count--;
        return PICO_ERROR_BADAUTH;
    }
    if (mock_ctx.inject.cyw43_auth_timeout_count > 0) {
        mock_ctx.inject.cyw43_auth_timeout_count--;
        sleep_ms(10 * 1000);
        return PICO_ERROR_TIMEOUT;
    }
    return mock_ctx.inject.cyw43_arch_wifi_connect_status;
}

void cyw43_arch_deinit(void)
{
}

static uint32_t cyw43_state_storage = 0xffffffff;

void *cyw43_state = &cyw43_state_storage;

int cyw43_wifi_get_mac(void *self, int itf, uint8_t *mac)
{
    (void)self;
    (void)itf;
    static uint8_t test_mac[6] = {0x00, 0x00, 0xde, 0xad, 0xbe, 0xef};
    memcpy(mac, test_mac, sizeof(test_mac));
    return 0;
}

// Timer functions
void sleep_ms(uint32_t ms)
{
    mock_ctx.spy.system_time_ms += ms;
    mock_ctx.spy.boot_time_ms += ms;
    mock_ctx.spy.watchdog_time_ms += ms;
}

static const char *dns_hostname;
static dns_callback_fn dns_found_func;
static void *dns_found_arg;

absolute_time_t get_absolute_time(void)
{
    return (absolute_time_t)(mock_ctx.spy.boot_time_ms * 1000);
}

int64_t absolute_time_diff_us(absolute_time_t from, absolute_time_t to)
{
    if (mock_ctx.inject.dns_lookup_delay) {
        mock_ctx.inject.dns_lookup_delay--;
        if (mock_ctx.inject.dns_lookup_delay == 0) {
            if (mock_ctx.inject.dns_lookup_fail) {
                dns_found_func(dns_hostname, NULL, dns_found_arg);
            } else {
                static const ip_addr_t ipaddr = {0xdeadbeef};
                dns_found_func(dns_hostname, &ipaddr, dns_found_arg);
            }
        }
    }
    return (int64_t)to - (int64_t)from;
}

bool add_repeating_timer_ms(uint32_t ms, bool (*callback)(repeating_timer_t *), void *user_data,
                            repeating_timer_t *out_timer)
{
    (void)ms;
    (void)callback;
    out_timer->user_data = user_data;
    return 0;
}

int mock_printf(const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    int buffer_len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (mock_ctx.spy.log_buffer_size >= LOG_BUFFER_SIZE) {
        printf("*** MOCK BUFFER OVERFLOW!\n");
        return 1;
    } else {
        mock_ctx.spy.log_buffer[mock_ctx.spy.log_buffer_size] = (char *)calloc(1, (size_t)buffer_len + 1);
        // Reference strings do not have newlines
        for (int ii = buffer_len - 1; ii > 0; ii--) {
            if (buffer[ii] == '\r' || buffer[ii] == '\n') {
                buffer[ii] = (char)0;
            }
        }
        strncpy(mock_ctx.spy.log_buffer[mock_ctx.spy.log_buffer_size], buffer, (size_t)buffer_len);
        if (mock_ctx.config.test_verbose) {
            printf("DEBUG: %s\n", buffer);
        }
        mock_ctx.spy.log_buffer_size += 1;
        return 0;
    }
}

time_t mock_time(time_t *tloc)
{
    (void)tloc; // Always called with NULL
    return (time_t)(mock_ctx.spy.system_time_ms / 1000);
}

int mock_settimeofday(const struct timeval *tp, void *tzp)
{
    (void)tzp; // Implementation has ignores timezones
    mock_ctx.spy.system_time_ms = (unsigned long long)(tp->tv_sec * 1000 + (tp->tv_usec * 1000));
    return 0;
}

// Utility functions
void stdio_init_all(void)
{
}

int test_printf(const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    (void)vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    printf("DEBUG: %s", buffer);
    return 0;
}

void *mock_calloc(size_t num, size_t size)
{
    if (mock_ctx.inject.calloc_fail_at != 0) {
        mock_ctx.spy.calloc_counter++;
        if (mock_ctx.spy.calloc_counter >= mock_ctx.inject.calloc_fail_at) {
            mock_ctx.inject.calloc_fail_at = 0;
            return NULL;
        }
    }
    mock_ctx.spy.alloced_counter++;
    return calloc(num, size);
}

void mock_free(void *ptr)
{
    if (ptr != NULL) {
        mock_ctx.spy.free_counter++;
        free(ptr);
    }
}

// lwIP functions
static udp_recv_fn udp_recv_callback;
static void *udp_recv_callback_arg;
struct pbuf *pbuf_alloc(pbuf_layer l, u16_t length, pbuf_type type)
{
    (void)l;
    (void)length;
    (void)type;

    if (mock_ctx.inject.pbuf_alloc_fail_at != 0) {
        mock_ctx.spy.pbuf_alloc_counter++;
        if (mock_ctx.spy.pbuf_alloc_counter >= mock_ctx.inject.pbuf_alloc_fail_at) {
            return NULL;
        }
    }

    static struct pbuf p;
    return &p;
}

err_t udp_sendto(struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *dst_ip, u16_t dst_port)
{
    (void)pcb;
    (void)p;
    (void)dst_ip;
    (void)dst_port;

    if (mock_ctx.inject.udp_sendto_fail) {
        return -1;
    }
    // Calling the recv() callback right away is harmless for this implementation.
    // And we hard-code a hacked up NTP packet as that's all that will ever be used
    p->tot_len = (mock_ctx.inject.udp_response_type == UDP_NTP_BAD_LEN) ? 0x0 : NTP_MSG_LEN;
    p->payload[0] = (mock_ctx.inject.udp_response_type == UDP_NTP_INVALID) ? 0x0 : 0x4; // mode
    p->payload[1] = (mock_ctx.inject.udp_response_type == UDP_NTP_KOD) ? 0x0 : 0x1;     // stratum
    p->payload[40] = (mock_ctx.spy.ntp_seconds >> 24) & 0xff;
    p->payload[41] = (mock_ctx.spy.ntp_seconds >> 16) & 0xff;
    p->payload[42] = (mock_ctx.spy.ntp_seconds >> 8) & 0xff;
    p->payload[43] = mock_ctx.spy.ntp_seconds & 0xff;
    static ip_addr_t bad_addr = {.addr = 0xffff};
    udp_recv_callback(udp_recv_callback_arg, pcb, p, (mock_ctx.inject.udp_invalid_addr) ? &bad_addr : dst_ip,
                      (mock_ctx.inject.udp_response_type == UDP_NTP_BAD_PORT) ? 0x0 : NTP_PORT);

    // Only generate a single invalid response
    mock_ctx.inject.udp_response_type = UDP_NTP_OK;
    return ERR_OK;
}

u8_t pbuf_free(struct pbuf *p)
{
    (void)p;
    return 1;
}

u16_t pbuf_copy_partial(const struct pbuf *p, void *dataptr, u16_t len, u16_t offset)
{
    memcpy(dataptr, p->payload + offset, len);
    return 0;
}

u8_t pbuf_get_at(const struct pbuf *p, u16_t offset)
{
    return p->payload[offset];
}

int dns_gethostbyname(const char *hostname, ip_addr_t *addr, dns_callback_fn found, void *arg)
{
    if (mock_ctx.inject.dns_lookup_delay > 0) {
        dns_hostname = hostname;
        dns_found_func = found;
        dns_found_arg = arg;
        return ERR_INPROGRESS;
    }
    if (mock_ctx.inject.dns_lookup_fail) {
        addr->addr = 0;
        found(hostname, NULL, arg);
        return ERR_INPROGRESS;
    } else if (mock_ctx.inject.dns_bad_arg) {
        return ERR_ARG;
    } else {
        return ERR_OK;
    }
}

void udp_recv(struct udp_pcb *pcb, udp_recv_fn recv, void *recv_arg)
{
    (void)pcb;
    udp_recv_callback = recv;
    udp_recv_callback_arg = recv_arg;
}

struct udp_pcb *udp_new_ip_type(u8_t type)
{
    (void)type;
    if (mock_ctx.inject.udp_new_ip_type_fail) {
        return NULL;
    }
    static struct udp_pcb buffer = {0xdeadbeef};
    return &buffer;
}

int ip_addr_cmp(const ip_addr_t *addr1, const ip_addr_t *addr2)
{
    return addr1->addr == addr2->addr;
}

// Watchdog
void watchdog_update(void)
{
    mock_ctx.spy.watchdog_time_ms = 0;
}

int watchdog_caused_reboot(void)
{
    return mock_ctx.inject.watchdog_caused_reboot || mock_ctx.spy.watchdog_reboot_called;
}

void watchdog_reboot(uint32_t pc, uint32_t sp, uint32_t delay_ms)
{
    (void)pc;
    (void)sp;
    (void)delay_ms;
    mock_ctx.spy.watchdog_reboot_called = 1;
}

void watchdog_enable(uint32_t delay_ms, int pause_on_debug)
{
    (void)delay_ms;
    (void)pause_on_debug;
}

// Flash
static uint32_t interrupt_status;

uint32_t save_and_disable_interrupts(void)
{
    return interrupt_status;
}

void restore_interrupts(uint32_t status)
{
    interrupt_status = status;
}

static clock_config_t flash_clock_config_storage;
void *flash_clock_config = (void *)&flash_clock_config_storage;

static mock_struct_t mock_tcp_server_state_storage;
void *mock_tcp_server_state = (void *)&mock_tcp_server_state_storage;

void flash_range_erase(uint32_t flash_offs, size_t count)
{
    (void)flash_offs;
    memset(&flash_clock_config, 0, count);
}

void flash_range_program(uint32_t flash_offs, const uint8_t *data, size_t count)
{
    (void)flash_offs;
    memcpy(&flash_clock_config, data, count);
}

// TCP/IP
err_t tcp_recved(struct tcp_pcb *pcb, u16_t len)
{
    (void)pcb;
    (void)len;
    return 0;
}

char mock_tcp_write_buffer[8192];

err_t tcp_write(struct tcp_pcb *pcb, const void *dataptr, u16_t len, u8_t apiflags)
{
    (void)pcb;
    (void)apiflags;
    strncpy(mock_tcp_write_buffer, (const char *)dataptr, len);
    return 0;
}

err_t tcp_output(struct tcp_pcb *pcb)
{
    (void)pcb;
    return 0;
}

const char *ip4addr_ntoa(ip_addr_t *ipaddr)
{
    (void)ipaddr;
    return "192.168.4.1";
}

void dhcp_server_deinit(dhcp_server_t *d)
{
    (void)d;
}

void dhcp_server_init(dhcp_server_t *d, ip_addr_t *ip, ip_addr_t *nm)
{
    (void)d;
    (void)ip;
    (void)nm;
}

void dns_server_deinit(dns_server_t *d)
{
    (void)d;
}

void dns_server_init(dns_server_t *d, ip_addr_t *ip)
{
    (void)d;
    (void)ip;
}
