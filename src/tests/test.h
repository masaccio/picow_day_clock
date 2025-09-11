#ifndef _TEST_H
#define _TEST_H

#include <stdbool.h>

#define LOG_BUFFER_SIZE 256

typedef struct
{
    int cyw43_auth_error_count;
    int cyw43_arch_wifi_connect_status;
    bool cyw43_arch_init_fail;
    bool udp_new_ip_type_fail;
    bool dns_lookup_fail;
} test_config_t;

extern int test_main(void);
extern test_config_t test_config;

#endif // _TEST_H
