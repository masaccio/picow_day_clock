#ifndef _TEST_H
#define _TEST_H

#include <stdbool.h>

#define LOG_BUFFER_SIZE 256

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
    // DNS
    int dns_lookup_delay;
    bool dns_lookup_fail;
    // Watchdog
    bool watchdog_caused_reboot;
} test_config_t;

extern int test_main(void);
extern test_config_t test_config;

#endif // _TEST_H
