#ifndef TEST_H
#define TEST_H

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
    int cyw43_auth_error_count;
    int cyw43_arch_wifi_connect_status;
    int cyw43_auth_timeout_count;
    int dns_lookup_delay;
    udp_response_type_t udp_response_type;
    // Flags go last for correct memory alignment
    int cyw43_arch_init_fail;
    int udp_new_ip_type_fail;
    int udp_sendto_fail;
    int udp_invalid_addr;
    int dns_lookup_fail;
    int dns_bad_arg;
    int watchdog_caused_reboot;
} test_config_t;

extern int test_main(void);

// Globals shared between the mocker and the test harness
extern test_config_t test_config;
extern unsigned int log_buffer_size;
extern unsigned int calloc_fail_at;
extern unsigned int calloc_counter;
extern unsigned int pbuf_alloc_fail_at;
extern unsigned int test_verbose;
extern int watchdog_reboot_called;
extern unsigned int log_buffer_size;
extern char **log_buffer;
extern unsigned long long mock_system_time_ms;
extern unsigned long long mock_boot_time_ms;
extern unsigned long long watchdog_time_ms;
extern unsigned long long mock_ntp_seconds;

#endif // TEST_H
