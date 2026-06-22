#pragma once

#include <stdbool.h>

#define LOG_BUFFER_SIZE 256

typedef enum
{
    UDP_NTP_OK,
    UDP_NTP_KOD,
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
    } inject;

    struct
    {
        char **log_buffer;
        unsigned int log_buffer_size;
        unsigned int calloc_counter;
        unsigned int pbuf_alloc_counter;
        unsigned long long system_time_ms;
        unsigned long long boot_time_ms;
        unsigned long long watchdog_time_ms;
        unsigned long long ntp_seconds;
        int watchdog_reboot_called;
        int watchdog_caused_reboot;
    } spy;

    struct
    {
        unsigned int test_verbose;
    } config;
} mock_context_t;

extern int test_main(void);
extern mock_context_t mock_ctx;
