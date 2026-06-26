#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "clock.h"

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

#define FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define assert_with_msg(a, b, msg)                                                                                     \
    do {                                                                                                               \
        if ((a) != (b)) {                                                                                              \
            printf("ASSERT FAILED: %s:%d: %s\n", FILENAME, __LINE__, (msg));                                           \
            return 1;                                                                                                  \
        } else if (mock_ctx.config.test_verbose) {                                                                     \
            printf("ASSERT OK: %s:%d: %s\n", FILENAME, __LINE__, (msg));                                               \
        }                                                                                                              \
    } while (0)

typedef struct
{
    uint64_t timestamp_ms;
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
    UDP_NTP_BAD_PORT
} udp_response_type_t;

typedef struct
{
    struct
    {
        unsigned int calloc_fail_at;
        int cyw43_arch_init_fail;
        int cyw43_arch_wifi_connect_status;
        int cyw43_auth_error_count;
        int cyw43_auth_timeout_count;
        int dns_bad_arg;
        int dns_lookup_delay;
        int dns_lookup_fail;
        unsigned int pbuf_alloc_fail_at;
        int tcp_open_fail;
        int udp_invalid_addr;
        int udp_new_ip_type_fail;
        udp_response_type_t udp_response_type;
        int udp_sendto_fail;
        int watchdog_caused_reboot;
        int config_button_pressed;
    } inject;

    struct
    {
        char **log_buffer;
        unsigned int log_buffer_size;
        unsigned int clock_state_alloc_failed;
        unsigned int lcd_init_failed;
        unsigned int calloc_counter;
        unsigned int pbuf_alloc_counter;
        unsigned long long system_time_ms;
        unsigned long long boot_time_ms;
        unsigned long long watchdog_time_ms;
        unsigned long long ntp_seconds;
        ntp_error_t fatal_ntp_error;
        wifi_error_t fatal_wifi_error;
        int watchdog_reboot_called;
        int fatal_reset_caught;
        unsigned int free_counter;
        unsigned int alloced_counter;
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
        unsigned int test_verbose;
    } config;
} mock_context_t;

extern int test_main(void);
extern mock_context_t mock_ctx;
