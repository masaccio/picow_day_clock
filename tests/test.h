#pragma once

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "clock.h"
#include "mock.h"
#include "ntp.h"
#include "watchdog.h"
#include "wifi.h"

// Macro to define a strongly-typed ring-buffer queue
#define DEFINE_TEST_QUEUE(TYPE, NAME, SIZE)                                                                            \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        TYPE data[SIZE];                                                                                               \
        size_t head;                                                                                                   \
        size_t tail;                                                                                                   \
        size_t count;                                                                                                  \
    } NAME;                                                                                                            \
                                                                                                                       \
    static inline void NAME##_init(NAME *q)                                                                            \
    {                                                                                                                  \
        q->head = 0;                                                                                                   \
        q->tail = 0;                                                                                                   \
        q->count = 0;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    static inline void NAME##_push(NAME *q, TYPE item)                                                                 \
    {                                                                                                                  \
        /* Write to the current tail position */                                                                       \
        q->data[q->tail] = item;                                                                                       \
        q->tail = (q->tail + 1) % SIZE;                                                                                \
                                                                                                                       \
        /* If not full, just increment count. */                                                                       \
        if (q->count < SIZE) {                                                                                         \
            q->count++;                                                                                                \
        } else {                                                                                                       \
            /* OVERFLOW! Advance the head to effectively "delete" the oldest item */                                   \
            q->head = (q->head + 1) % SIZE;                                                                            \
        }                                                                                                              \
    }

typedef struct
{
    time_t timestamp_ms;
    watchdog_error_t watchdog;
    ntp_error_t ntp;
    wifi_error_t wifi;
} icon_event_t;

DEFINE_TEST_QUEUE(icon_event_t, icon_queue_t, 32)

typedef struct
{
    time_t timestamp_ms;
    color_t color;
    lcd_status_message_t msg;
} lcd_message_event_t;

DEFINE_TEST_QUEUE(lcd_message_event_t, lcd_message_queue_t, 32)

typedef enum
{
    UDP_NTP_OK,
    UDP_NTP_KOD,
    UDP_NTP_LEAP3,
    UDP_NTP_INVALID,
    UDP_NTP_BAD_LEN,
    UDP_NTP_BAD_PORT,
    UDP_TIMEOUT,
} udp_response_type_t;

typedef struct
{
    struct
    {
        int cyw43_arch_init_fail;              // cyw43_arch_init() return value
        int cyw43_arch_wifi_connect_status;    // cyw43_arch_wifi_connect_timeout_ms() return value
        int cyw43_auth_error_count;            // Generate Wi-Fi auth errors
        int cyw43_auth_timeout_count;          // Generate Wi-Fi timeout errors
        int dns_bad_arg;                       // dns_gethostbyname() returns ERR_ARG
        int dns_lookup_fail;                   // DNS hostname lookup fails
        uint32_t dns_latency_ms;               // Time to respond to a DNS request
        unsigned int pbuf_alloc_fail_at;       // Allows specific pbuf_alloc() calls to fail
        int tcp_open_fail;                     // tcp_open() fails
        int tcp_bind_fail;                     // tcp_bind() fails
        int tcp_listen_fail;                   // tcp_listen_with_backlog() fails
        int tcp_close_fail;                    // tcp_close() fails
        int udp_invalid_addr;                  // Return an invalid sender address for UDP
        int udp_new_ip_type_fail;              // udp_new_ip_type() returns NULL
        udp_response_type_t udp_response_type; // Steers UDP responses for NTP
        int udp_sendto_fail;                   // udp_sendto() returns failure
        uint32_t udp_latency_ms;               // Time to respond to a UDP request
        int watchdog_caused_reboot;            // Clock starts up with positive watchdog_caused_reboot()
        int factory_reset_pressed;             // Factory reset GPIO is asserted
        int fatal_reset_no_longjmp;            // Do not longjmp() inside fatal_reset()
        int exit_on_ntp_success;               // Terminate main loop if NTP returned valid
        time_t exit_after_ms;                  // Maximum number of ms to simulate
        uint16_t adc_level;                    // Ambient light sensor level
    } inject;

    // The Discrete Event Simulator State
    struct
    {
        // DNS Simulation
        bool dns_pending;
        time_t dns_fire_time;
        void (*dns_cb)(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
        void *dns_arg;
        char dns_hostname[256];

        // UDP/NTP Simulation
        bool udp_pending;
        time_t udp_fire_time;
        struct udp_pcb *udp_pcb;
        udp_recv_fn udp_recv_cb;
        void *udp_recv_arg;

        // TCP/Captive Portal Simulation
        tcp_accept_fn tcp_accept_cb;
        tcp_recv_fn tcp_recv_cb;
        void *listen_arg;
        void *conn_arg;
        void *tcp_arg;
        struct tcp_pcb listen_pcb;
        struct tcp_pcb conn_pcb;
        time_t tcp_next_fire_time;
        char tcp_write_buffer[TCP_IP_BUFFER_SIZE];
        char tcp_payloads[10][2048]; // Encapsulated TCP payloads
        int tcp_payload_count;
        int tcp_payload_idx;

        tcp_poll_fn tcp_poll_cb;
        uint8_t tcp_poll_interval;
        time_t tcp_next_poll_time;

        // TCP Sent (ACK) Simulation
        tcp_sent_fn tcp_sent_cb;
        bool tcp_sent_pending;
        time_t tcp_sent_fire_time;
        u16_t tcp_sent_len;

        // TCP Error Simulation
        tcp_err_fn tcp_err_cb;
        bool tcp_err_pending;
        time_t tcp_err_fire_time;
        err_t tcp_err_code;

        // Hardware Timer Simulation
        bool clock_timer_active;
        time_t clock_timer_delay;
        time_t clock_timer_next_fire;
        bool (*clock_timer_cb)(repeating_timer_t *rt);
        repeating_timer_t *clock_timer_arg;

        bool backlight_timer_active;
        time_t backlight_timer_delay;
        time_t backlight_timer_next_fire;
        bool (*backlight_timer_cb)(repeating_timer_t *rt);
        repeating_timer_t *backlight_timer_arg;
    } sim;

    struct
    {
        unsigned int clock_state_alloc_failed;
        int cyw43_arch_init_fail;
        unsigned int calloc_counter;
        unsigned int pbuf_alloc_counter;
        time_t system_time_ms;
        time_t boot_time_ms;
        time_t watchdog_time_ms;
        time_t ntp_seconds;
        ntp_error_t fatal_ntp_error;
        wifi_error_t fatal_wifi_error;
        int watchdog_reboot_called;
        int fatal_reset_caught;
        int ntp_packet_sent;
        unsigned int udp_sendto_calls;
        uint16_t udp_last_send_len;

        // LCD events
        icon_queue_t icon_history;
        lcd_message_queue_t lcd_msg_history;
        watchdog_error_t watchdog_icon;
        ntp_error_t ntp_icon;
        wifi_error_t wifi_icon;
        uint8_t lcd_brightness;
    } spy;

    struct
    {
        unsigned int test_verbose;
        uint16_t udp_port;
    } config;
} mock_context_t;

extern int test_main(void);
extern mock_context_t mock_ctx;
extern jmp_buf fatal_reset_jmp_buf;
