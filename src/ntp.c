/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * Additional code copyright (c) 2025 Jon Connell
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// #include <string.h>
// #include <time.h>

#include "ntp.h"

#include "pico/cyw43_arch.h"

#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

// Make an NTP request
void ntp_request(ntp_state_t *state) {
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *)p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

// Called by dns_gethostbyname() after a DNS request completes
// having previously returned ERR_INPROGRESS to ntp_get_time()
void ntp_dns_callback(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    ntp_state_t *state = (ntp_state_t *)arg;
    if (ipaddr) {
        state->ntp_server_address = *ipaddr;
        printf("NTP address: %s\r\n", ipaddr_ntoa(ipaddr));
        ntp_request(state);
    } else {
        state->status = NTP_STATUS_DNS_ERROR;
    }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    ntp_state_t *state = (ntp_state_t *)arg;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    // Check the result
    if (ip_addr_cmp(addr, &state->ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN && mode == 0x4 &&
        stratum != 0) {
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 =
            seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
        uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
        time_t epoch = seconds_since_1970;

        state->status = NTP_STATUS_SUCCESS;
        state->time_handler(state->parent_state, &epoch);
    } else {
        state->status = NTP_STATUS_INVALID_RESPONSE;
    }
    pbuf_free(p);
}

// Perform initialisation
extern ntp_state_t *ntp_init(void *parent_state, ntp_time_handler_t time_handler) {

    ntp_state_t *state = (ntp_state_t *)calloc(1, sizeof(ntp_state_t));
    if (!state) {
        printf("Failed to allocate NTP state\r\n");
        return NULL;
    }

    state->parent_state = parent_state;
    state->time_handler = time_handler;

    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state->ntp_pcb) {
        free(state);
        return NULL;
    }
    udp_recv(state->ntp_pcb, ntp_recv, state);
    return state;
}

ntp_status_t ntp_get_time(ntp_state_t *ntp_state) {
    absolute_time_t start_time = get_absolute_time();

    cyw43_arch_lwip_begin();
    int dns_status = dns_gethostbyname(NTP_SERVER, &ntp_state->ntp_server_address, ntp_dns_callback, ntp_state);
    cyw43_arch_lwip_end();

    ntp_state->status = NTP_STATUS_PENDING;
    if (dns_status == ERR_OK) {
        ntp_request(ntp_state);
    } else if (dns_status != ERR_INPROGRESS) {
        return NTP_STATUS_DNS_ERROR;
    }

    while (ntp_state->status != NTP_STATUS_SUCCESS) {
        sleep_ms(50); /* wait for background lwIP */

        if (absolute_time_diff_us(start_time, get_absolute_time()) > NTP_TIMEOUT_MS * 1000) {
            return NTP_STATUS_TIMEOUT;
        }
    }

    return NTP_STATUS_SUCCESS;
}