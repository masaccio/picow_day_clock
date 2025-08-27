/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _NTP_H
#define _NTP_H

#include <stdlib.h>

#include "lwip/ip_addr.h"
#include "pico/stdlib.h"

#include "lcd.h"

typedef void (*ntp_time_handler_t)(void *state, time_t *time);

typedef enum {
    NTP_STATUS_PENDING = 0,
    NTP_STATUS_SUCCESS = 1,
    NTP_STATUS_DNS_ERROR = -1,
    NTP_STATUS_TIMEOUT = -2,
    NTP_STATUS_INVALID_RESPONSE = -3,
} ntp_status_t;

typedef struct ntp_state_t {
    ip_addr_t ntp_server_address;
    bool dns_request_sent;
    struct udp_pcb *ntp_pcb;
    absolute_time_t ntp_test_time;
    void *parent_state;
    ntp_status_t status;
    ntp_time_handler_t time_handler;
} ntp_state_t;

#ifndef NTP_SERVER
#define NTP_SERVER "pool.ntp.org" // Default NTP server
#endif

#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_TIMEOUT_MS (30 * 1000)

extern int64_t ntp_failed_handler(alarm_id_t id, void *user_data);

extern void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);

extern void ntp_request(ntp_state_t *state);

extern ntp_state_t *ntp_init(void *parent_state, ntp_time_handler_t time_handler);

extern void ntp_result(ntp_state_t *state, int status, time_t *result);

extern ntp_status_t ntp_get_time(ntp_state_t *ntp_state);

#endif /* _NTP_H */