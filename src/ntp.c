/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * Additional code copyright (c) 2025 Jon Connell
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Pico SDK
#ifndef TEST_MODE
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "pico/cyw43_arch.h"
#else
#include "tests/mock.h"
#endif

// Local includes
#include "clock.h"
#include "ntp.h"

// Make an NTP request
void ntp_request(ntp_state_t *state)
{
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    if (!p) {
        CLOCK_DEBUG("NTP: failed to allocate PBUF\r\n");
        state->status = NTP_STATUS_MEMORY_ERROR;
        cyw43_arch_lwip_end();
        return;
    }
    uint8_t *req = (uint8_t *)p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    int err = udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    if (err != 0) {
        CLOCK_DEBUG("NTP: send error %d\r\n", err);
        state->status = NTP_STATUS_INVALID_RESPONSE;
        pbuf_free(p);
        cyw43_arch_lwip_end();
        return;
    }
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

// Called by dns_gethostbyname() after a DNS request completes
// having previously returned ERR_INPROGRESS to ntp_get_time()
void ntp_dns_callback(const char *hostname, const ip_addr_t *ipaddr, void *arg)
{
    ntp_state_t *state = (ntp_state_t *)arg;
    (void)hostname;
    if (ipaddr) {
        state->ntp_server_address = *ipaddr;
        ntp_request(state);
    } else {
        CLOCK_DEBUG("NTP: DNS error for %s\r\n", hostname);
        state->status = NTP_STATUS_DNS_ERROR;
    }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    ntp_state_t *state = (ntp_state_t *)arg;
    (void)pcb;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    bool addrs_valid = ip_addr_cmp(addr, &state->ntp_server_address);
    bool response_valid = port == NTP_PORT && p->tot_len == NTP_MSG_LEN && mode == 0x4;
    if (addrs_valid && response_valid && stratum != 0) {
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 =
            seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
        uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
        time_t epoch = seconds_since_1970;

        state->status = NTP_STATUS_SUCCESS;
        state->time_handler(state->parent_state, &epoch);
    } else if (addrs_valid && response_valid && stratum == 0) {
        // We got a 'kiss of death' from the NTP server for too many requests.
        state->status = NTP_STATUS_KOD;
        CLOCK_DEBUG("NTP: server responded with KoD\r\n");
    } else {
        CLOCK_DEBUG("NTP: invalid response: addrs %s, port %s, len %s, mode %s, stratum %s\r\n",
                    addrs_valid ? "valid" : "invalid", port == NTP_PORT ? "valid" : "invalid",
                    p->tot_len == NTP_MSG_LEN ? "valid" : "invalid", mode == 0x4 ? "valid" : "invalid",
                    stratum != 0 ? "valid" : "invalid");
        state->status = NTP_STATUS_INVALID_RESPONSE;
    }
    pbuf_free(p);
}

// Perform initialisation
extern ntp_state_t *ntp_init(void *parent_state, ntp_time_handler_t time_handler)
{
    ntp_state_t *state = (ntp_state_t *)calloc(1, sizeof(ntp_state_t));
    if (!state) {
        CLOCK_DEBUG("Failed to allocate NTP state\r\n");
        return NULL;
    }

    state->parent_state = parent_state;
    state->time_handler = time_handler;

    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state->ntp_pcb) {
        CLOCK_DEBUG("Failed to allocate UDP buffer\r\n");
        free(state);
        return NULL;
    }
    udp_recv(state->ntp_pcb, ntp_recv, state);
    return state;
}

ntp_status_t ntp_get_time(ntp_state_t *state)
{
    absolute_time_t start_time = get_absolute_time();

    if (state->ntp_server_address.addr != 0) {
        ntp_request(state);
        return state->status;
    }

    cyw43_arch_lwip_begin();
    int dns_status = dns_gethostbyname(NTP_SERVER, &state->ntp_server_address, ntp_dns_callback, state);
    cyw43_arch_lwip_end();

    state->status = NTP_STATUS_PENDING;
    if (dns_status == ERR_OK) {
        ntp_request(state);
    } else if (dns_status != ERR_INPROGRESS) {
        CLOCK_DEBUG("NTP: DNS lookup failed with error %d\r\n", dns_status);
        return NTP_STATUS_DNS_ERROR;
    }

    while (state->status != NTP_STATUS_SUCCESS) {
        sleep_ms(500); // wait for background lwIP

        if (absolute_time_diff_us(start_time, get_absolute_time()) > NTP_TIMEOUT_MS * 1000) {
            CLOCK_DEBUG("NTP: DNS timed out after %d seconds\r\n", NTP_TIMEOUT_MS / 1000);
            return NTP_STATUS_TIMEOUT;
        }
    }

    return state->status;
}
