// Mock definitions for unit testing
#ifndef MOCK_H
#define MOCK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
#include <sys/time.h>
#endif

// Define system data types
typedef unsigned int uint;
typedef char err_t;
typedef unsigned short u16_t;
typedef unsigned char u8_t;

// Mock SDK types
typedef unsigned int pbuf_type;
struct udp_pcb
{
    unsigned int mock;
};
typedef struct
{
    unsigned int mock;
} mock_struct_t;
typedef mock_struct_t spi_inst_t;
typedef mock_struct_t udp_pcb;
typedef struct repeating_timer_t
{
    void *user_data;
} repeating_timer_t;

typedef unsigned int pbuf_layer;
#define PBUF_TRANSPORT ((pbuf_layer)0)
#ifndef NTP_MSG_LEN
#define NTP_MSG_LEN 48
#endif
#ifndef NTP_PORT
#define NTP_PORT 123
#endif

typedef struct pbuf
{
    u8_t payload[NTP_MSG_LEN];
    u16_t tot_len;
} pbuf;
#define repeating_timer repeating_timer_t

typedef unsigned long absolute_time_t;
typedef struct ip_addr_t
{
    unsigned long addr;

} ip_addr_t;
typedef void (*dns_callback_fn)(const char *name, const ip_addr_t *ipaddr, void *arg);
typedef unsigned long alarm_id_t;
typedef void (*udp_recv_fn)(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr,
                            short unsigned int port);

// Standard library functions
#define printf(...) mock_printf(__VA_ARGS__)
extern int mock_printf(const char *format, ...);

extern void *mock_calloc(size_t, size_t);
#define calloc(num, size) mock_calloc(num, size)

extern unsigned long long mock_system_time_ms;

#define time(tloc) mock_time(tloc)
extern time_t mock_time(time_t *);

#define settimeofday(tp, tzp) mock_settimeofday(tp, tzp)
#ifdef _WIN32
struct timeval;
#endif
int mock_settimeofday(const struct timeval *, void *);

// Mock definitions for SDK constants
enum
{
    ERR_OK,
    ERR_INPROGRESS,
    PWM_CHAN_B,
    IPADDR_TYPE_ANY,
    PBUF_RAM,
    GPIO_FUNC_PWM,
    GPIO_FUNC_SPI,
    GPIO_OUT,
    PICO_ERROR_CONNECT_FAILED,
    PICO_ERROR_TIMEOUT,
    PICO_ERROR_BADAUTH,
    CYW43_AUTH_WPA2_AES_PSK,
};
#define spi1 ((spi_inst_t *)0xdeadbeef)

// SPI functions
uint spi_init(spi_inst_t *spi, int baudrate);
int spi_write_blocking(spi_inst_t *spi, const uint8_t *src, size_t len);

// GPIO functions
void gpio_set_function(uint gpio, uint func);
void gpio_init(uint gpio);
void gpio_set_dir(uint gpio, bool out);
void gpio_put(uint gpio, int value);

// PWM functions
int pwm_gpio_to_slice_num(uint gpio);
void pwm_set_wrap(uint slice_num, uint32_t wrap);
void pwm_set_chan_level(uint slice_num, uint chan, uint32_t level);
void pwm_set_clkdiv(uint slice_num, float divider);
void pwm_set_enabled(uint slice_num, bool enabled);

// Wi-Fi functions
int cyw43_arch_init(void);
void cyw43_arch_lwip_begin(void);
void cyw43_arch_lwip_end(void);
void cyw43_arch_enable_sta_mode(void);
int cyw43_arch_wifi_connect_timeout_ms(const char *ssid, const char *password, uint auth, uint timeout_ms);
void cyw43_arch_deinit(void);

// Utility functions
void sleep_ms(uint32_t ms);
void stdio_init_all(void);

// lwIP functions
struct pbuf *pbuf_alloc(pbuf_layer l, u16_t length, pbuf_type type);
err_t udp_sendto(struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *dst_ip, u16_t dst_port);
u8_t pbuf_free(struct pbuf *p);
u16_t pbuf_copy_partial(const struct pbuf *p, void *dataptr, u16_t len, u16_t offset);
u8_t pbuf_get_at(const struct pbuf *p, u16_t offset);
int dns_gethostbyname(const char *hostname, ip_addr_t *addr, dns_callback_fn found, void *arg);
void udp_recv(struct udp_pcb *pcb, udp_recv_fn recv, void *recv_arg);
struct udp_pcb *udp_new_ip_type(u8_t type);
int ip_addr_cmp(const ip_addr_t *addr1, const ip_addr_t *addr2);

// Timer functions
absolute_time_t get_absolute_time(void);
int64_t absolute_time_diff_us(absolute_time_t t1, absolute_time_t t2);
int add_repeating_timer_ms(uint32_t ms, bool (*callback)(repeating_timer_t *), void *user_data,
                           repeating_timer_t *out_timer);

// Watchdog
bool watchdog_caused_reboot(void);
void watchdog_enable(uint32_t delay_ms, bool pause_on_debug);
void watchdog_update(void);

#endif // MOCK_H
