/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#ifndef TEST_MODE
#include "lwip/ip_addr.h"
#else
#include "mock.h"
#endif

typedef struct
{
    struct udp_pcb *udp;
    ip_addr_t ip;
} dns_server_t;

void dns_server_init(dns_server_t *d, ip_addr_t *ip);
void dns_server_deinit(dns_server_t *d);
