#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "clock.h"
#include "mock.h"

#define LOG_BUFFER_SIZE 256

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
    }                                                                                                                  \
                                                                                                                       \
    static inline bool NAME##_pop(NAME *q, TYPE *item)                                                                 \
    {                                                                                                                  \
        if (q->count == 0)                                                                                             \
            return false;                                                                                              \
        *item = q->data[q->head];                                                                                      \
        q->head = (q->head + 1) % SIZE;                                                                                \
        q->count--;                                                                                                    \
        return true;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    static inline void NAME##_clear(NAME *q)                                                                           \
    {                                                                                                                  \
        NAME##_init(q);                                                                                                \
    }

typedef struct
{
    time_t timestamp_ms;
    watchdog_error_t watchdog;
    ntp_error_t ntp;
    wifi_error_t wifi;
} icon_event_t;

DEFINE_TEST_QUEUE(icon_event_t, icon_queue_t, 32)

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
        char **buffer;
        unsigned int buffer_size;
    } logs;

    struct
    {
        unsigned int calloc_fail_at;
        int cyw43_arch_init_fail;
        int cyw43_arch_wifi_connect_status;
        int cyw43_auth_error_count;
        int cyw43_auth_timeout_count;
        int dns_bad_arg;
        int dns_lookup_fail;
        uint32_t dns_latency_ms;
        unsigned int pbuf_alloc_fail_at;
        int tcp_open_fail;
        int udp_invalid_addr;
        int udp_new_ip_type_fail;
        udp_response_type_t udp_response_type;
        int udp_sendto_fail;
        uint32_t udp_latency_ms;
        int watchdog_caused_reboot;
        int factory_reset_pressed;
        int fatal_reset_no_longjmp;
        int exit_on_ntp_success;
        char tcp_write_buffer[TCP_IP_BUFFER_SIZE];
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
        bool timer_active;
        time_t timer_delay;
        time_t timer_next_fire;
        bool (*timer_cb)(repeating_timer_t *rt);
        repeating_timer_t *timer_arg;
    } sim;

    struct
    {
        unsigned int clock_state_alloc_failed;
        unsigned int lcd_init_failed;
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

        // Icons
        icon_queue_t icon_history;
        struct
        {
            watchdog_error_t watchdog;
            ntp_error_t ntp;
            wifi_error_t wifi;
        } icon_state;
    } spy;

    struct
    {
        unsigned int frees;
        unsigned int allocs;
    } leak_checker;

    struct
    {
        unsigned int test_verbose;
        uint16_t udp_port;
    } config;
} mock_context_t;

extern int test_main(void);
extern mock_context_t mock_ctx;
