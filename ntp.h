/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include "lwip/ip_addr.h"
#include "pico/stdlib.h"

typedef struct NTP_T_ {
    ip_addr_t ntp_server_address;
    bool dns_request_sent;
    struct udp_pcb *ntp_pcb;
    absolute_time_t ntp_test_time;
    alarm_id_t ntp_resend_alarm;
} NTP_T;

#ifndef NTP_SERVER
#define NTP_SERVER "pool.ntp.org" // Default NTP server
#endif

#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_TEST_TIME (30 * 1000)
#define NTP_RESEND_TIME (10 * 1000)

extern int64_t ntp_failed_handler(alarm_id_t id, void *user_data);

extern void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);

extern void ntp_request(NTP_T *state);

extern NTP_T *ntp_init(void);

extern void ntp_result(NTP_T *state, int status, time_t *result);