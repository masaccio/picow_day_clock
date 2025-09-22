#ifndef _TEST_H
#define _TEST_H

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
    // Wi-Fi
    int cyw43_auth_error_count;
    int cyw43_arch_wifi_connect_status;
    int cyw43_auth_timeout_count;
    bool cyw43_arch_init_fail;
    // UDP
    bool udp_new_ip_type_fail;
    bool udp_sendto_fail;
    udp_response_type_t udp_response_type;
    // DNS
    int dns_lookup_delay;
    bool dns_lookup_fail;
    bool dns_bad_arg;
    // Watchdog
    bool watchdog_caused_reboot;
} test_config_t;

extern int test_main(void);
extern test_config_t test_config;

#endif // _TEST_H
