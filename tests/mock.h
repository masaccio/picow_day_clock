#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "config.h"

// =============================================================================
// Core System & Mock Types
// =============================================================================

// Basic data types
typedef unsigned int uint;
typedef char err_t;
typedef unsigned short u16_t;
typedef unsigned char u8_t;

// Opaque generic mock structure for hardware layers
typedef struct
{
    unsigned int mock;
} mock_struct_t;

typedef mock_struct_t spi_inst_t;

// =============================================================================
// lwIP Constants & Error Codes
// =============================================================================

// Standard lwIP Error Codes (Must be <= 0)
#define ERR_OK 0
#define ERR_MEM -1
#define ERR_BUF -2
#define ERR_TIMEOUT -3
#define ERR_RTE -4
#define ERR_INPROGRESS -5
#define ERR_VAL -6
#define ERR_WOULDBLOCK -7
#define ERR_USE -8
#define ERR_ALREADY -9
#define ERR_ISCONN -10
#define ERR_CONN -11
#define ERR_IF -12
#define ERR_ABRT -13
#define ERR_RST -14
#define ERR_CLSD -15
#define ERR_ARG -16

// lwIP TCP/IP Flags
#define TCP_WRITE_FLAG_MORE 0x02
#define PBUF_RAM 0

// =============================================================================
// Pico SDK Hardware Constants
// =============================================================================

// GPIO Directions and Functions
#define GPIO_IN 0
#define GPIO_OUT 1
#define GPIO_FUNC_SPI 1
#define GPIO_FUNC_PWM 4
#define PWM_CHAN_B 1

// =============================================================================
// Pico CYW43 Wi-Fi Architecture Constants
// =============================================================================

// Pico SDK generic error codes (Must be < 0)
#define PICO_ERROR_TIMEOUT -1
#define PICO_ERROR_GENERIC -2
#define PICO_ERROR_NO_DATA -3
#define PICO_ERROR_NOT_PERMITTED -4
#define PICO_ERROR_INVALID_ARG -5
#define PICO_ERROR_IO -6
#define PICO_ERROR_BADAUTH -7
#define PICO_ERROR_CONNECT_FAILED -8

// CYW43 Networking Constants
#define CYW43_AUTH_WPA2_AES_PSK 0x00400004
#define CYW43_ITF_STA 0

// Default Captive Portal IP bounds (Usually 192.168.4.1)
#define CYW43_DEFAULT_IP_AP_ADDRESS 0xC0A80401
#define CYW43_DEFAULT_IP_MASK 0xFFFFFF00

// =============================================================================
// Standard Library Overrides (Host execution)
// =============================================================================

#define printf(...) test_printf(__VA_ARGS__)

// =============================================================================
// Pico SDK Hardware Mocks
// =============================================================================

// --- Interrupts ---
uint32_t save_and_disable_interrupts(void);
void restore_interrupts(uint32_t status);

// --- Time & Timers ---
typedef unsigned long absolute_time_t;

typedef struct repeating_timer_t
{
    void *user_data;
} repeating_timer_t;

#define repeating_timer repeating_timer_t

absolute_time_t get_absolute_time(void);
int64_t absolute_time_diff_us(absolute_time_t from, absolute_time_t to);
uint32_t to_ms_since_boot(absolute_time_t t);
void sleep_ms(uint32_t ms);

bool add_repeating_timer_ms(uint32_t delay_ms, bool (*callback)(repeating_timer_t *), void *user_data,
                            repeating_timer_t *out_timer);

// --- Watchdog ---
#define SRAM_END (uint32_t)-1
int watchdog_caused_reboot(void);
void watchdog_enable(uint32_t delay_ms, int pause_on_debug);
void watchdog_reboot(uint32_t pc, uint32_t sp, uint32_t delay_ms);
void watchdog_update(void);

// --- Flash Memory ---
#define FLASH_SECTOR_SIZE (uint32_t)0
#define FLASH_PAGE_SIZE (uint32_t)256
#define FLASH_CONFIG_ADDR mock_flash_config

extern void *mock_flash_config;
void flash_range_erase(uint32_t flash_offs, size_t count);
void flash_range_program(uint32_t flash_offs, const uint8_t *data, size_t count);

// =============================================================================
// lwIP Network Stack Mocks
// =============================================================================

// --- IP Addresses ---
typedef struct ip_addr_t
{
    uint32_t addr;
} ip_addr_t;
typedef ip_addr_t ip4_addr_t;
typedef struct netif
{
    unsigned int mock;
} netif;

#define IPADDR_TYPE_ANY 46
#ifndef IP_ANY_TYPE
#define IP_ANY_TYPE NULL
#endif

#define ipaddr_ntoa(ipaddr) ip4addr_ntoa(ipaddr)
const char *ip4addr_ntoa(const ip_addr_t *ipaddr);
#define PP_HTONL(ipaddr) (ipaddr)
#define IP4_ADDR(ipaddr, a, b, c, d)                                                                                   \
    do {                                                                                                               \
        (ipaddr)->addr = (((uint32_t)(a) & 0xffu) << 24u) | (((uint32_t)(b) & 0xffu) << 16u) |                         \
                         (((uint32_t)(c) & 0xffu) << 8u) | ((uint32_t)(d) & 0xffu);                                    \
    } while (0)
#define ip_2_ip4(ipaddr) ((ip4_addr_t *)(ipaddr))
#define ip4_addr_get_u32(ipaddr) ((ipaddr)->addr)
#define lwip_htons(v) ((uint16_t)((((uint16_t)(v)) << 8) | (((uint16_t)(v)) >> 8)))
#define lwip_ntohs(v) lwip_htons(v)

// --- Packets (pbuf) ---
typedef unsigned int pbuf_type;
typedef unsigned int pbuf_layer;
#define PBUF_TRANSPORT ((pbuf_layer)0)

typedef struct pbuf
{
    u8_t payload[TCP_IP_BUFFER_SIZE];
    u16_t tot_len;
    u16_t len;
} pbuf;

// --- Protocol Control Blocks (PCB) ---
struct udp_pcb
{
    unsigned int mock;
};

struct tcp_pcb
{
    unsigned int mock;
};

struct pbuf *pbuf_alloc(pbuf_layer l, u16_t length, pbuf_type type);
u8_t pbuf_free(struct pbuf *p);
u16_t pbuf_copy_partial(const struct pbuf *p, void *dataptr, u16_t len, u16_t offset);
u8_t pbuf_get_at(const struct pbuf *p, u16_t offset);

// --- lwIP Callbacks ---
typedef void (*udp_recv_fn)(void *arg, struct udp_pcb *pcb, struct pbuf *p, const struct ip_addr_t *addr,
                            short unsigned int port);
typedef err_t (*tcp_accept_fn)(void *arg, struct tcp_pcb *newpcb, err_t err);
typedef err_t (*tcp_recv_fn)(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
typedef err_t (*tcp_poll_fn)(void *arg, struct tcp_pcb *tpcb);
typedef err_t (*tcp_sent_fn)(void *arg, struct tcp_pcb *tpcb, u16_t len);
typedef void (*tcp_err_fn)(void *arg, err_t err);

// --- UDP API ---
struct udp_pcb *udp_new_ip_type(u8_t type);
void udp_recv(struct udp_pcb *pcb, udp_recv_fn recv, void *recv_arg);
err_t udp_sendto(struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *dst_ip, u16_t dst_port);

// --- TCP API ---
int ip_addr_cmp(const ip_addr_t *addr1, const ip_addr_t *addr2);
struct tcp_pcb *tcp_new(void);
struct tcp_pcb *tcp_new_ip_type(u8_t type);
void tcp_arg(struct tcp_pcb *pcb, void *arg);
err_t tcp_bind(struct tcp_pcb *pcb, const ip_addr_t *ipaddr, u16_t port);
struct tcp_pcb *tcp_listen_with_backlog(struct tcp_pcb *pcb, u8_t backlog);
void tcp_accept(struct tcp_pcb *pcb, tcp_accept_fn accept);
void tcp_recv(struct tcp_pcb *pcb, tcp_recv_fn recv);
void tcp_poll(struct tcp_pcb *pcb, tcp_poll_fn poll, u8_t interval);
void tcp_sent(struct tcp_pcb *pcb, tcp_sent_fn sent);
void tcp_err(struct tcp_pcb *pcb, tcp_err_fn err);
err_t tcp_output(struct tcp_pcb *pcb);
err_t tcp_write(struct tcp_pcb *pcb, const void *dataptr, u16_t len, u8_t apiflags);
err_t tcp_recved(struct tcp_pcb *pcb, u16_t len);
err_t tcp_close(struct tcp_pcb *pcb);
void tcp_abort(struct tcp_pcb *pcb);

// --- DNS API ---
err_t dns_gethostbyname(const char *hostname, ip_addr_t *addr,
                        void (*found)(const char *name, const ip_addr_t *ipaddr, void *callback_arg),
                        void *callback_arg);

// CYW43 API
extern void *cyw43_state;
int cyw43_arch_init(void);
int cyw43_arch_wifi_connect_timeout_ms(const char *ssid, const char *password, uint auth, uint timeout_ms);
void cyw43_arch_disable_ap_mode(void);
void cyw43_arch_disable_sta_mode(void);
void cyw43_arch_enable_ap_mode(const char *ssid, const char *password, uint32_t auth);
void cyw43_arch_enable_sta_mode(void);
void cyw43_arch_lwip_begin(void);
void cyw43_arch_lwip_end(void);
int cyw43_wifi_get_mac(void *self, int itf, uint8_t *mac);

// =============================================================================
// GPIO mocks
// =============================================================================
void stdio_init_all(void);
void gpio_init(uint gpio);
void gpio_set_dir(uint gpio, int out);
void gpio_pull_up(uint gpio);
int gpio_get(uint gpio);

// =============================================================================
// ADC mocks
// =============================================================================
int adc_init(void);
void adc_gpio_init(uint gpio);
void adc_select_input(uint input);
uint16_t adc_read(void);

// =============================================================================
// Application Domain (Wi-Fi & NTP Logic)
// =============================================================================

// Time
#undef time
#define time(tloc) mock_time(tloc)
#undef settimeofday
#define settimeofday(tp, tzp) mock_settimeofday(tp, tzp)
time_t mock_time(time_t *tloc);
int mock_settimeofday(const struct timeval *tp, void *tzp);

extern int test_printf(const char *format, ...) __attribute__((format(printf, 1, 2)));

// Wi-Fi AP testing
extern void *mock_tcp_server_state;
err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);

// TCP/IP mocking
void mock_queue_tcp_payload(const char *body);
void mock_clear_tcp_payloads(void);
void mock_inject_tcp_error(err_t code, uint32_t delay_ms);
