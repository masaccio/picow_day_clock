/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * Additional code copyright (c) 2025 Jon Connell
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Pico SDK */
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "pico/cyw43_arch.h"

/* Local includes */
#include "common.h"
#include "ntp.h"

// Determines if given UTC time is in British Summer Time (BST)
bool time_is_bst(struct tm *utc)
{
    if (!utc)
        return false;

    int year = utc->tm_year + 1900;

    // Find last Sunday in March
    struct tm start = {
        .tm_year = year - 1900, .tm_mon = 2, .tm_mday = 31, .tm_hour = 1, .tm_min = 0, .tm_sec = 0, .tm_isdst = 0};
    mktime(&start);
    while (start.tm_wday != 0) {
        start.tm_mday--;
        mktime(&start);
    }

    // Find last Sunday in October
    struct tm end = {
        .tm_year = year - 1900, .tm_mon = 9, .tm_mday = 31, .tm_hour = 1, .tm_min = 0, .tm_sec = 0, .tm_isdst = 1};
    mktime(&end);
    while (end.tm_wday != 0) {
        end.tm_mday--;
        mktime(&end);
    }

    time_t now = mktime(utc);
    time_t start_time = mktime(&start);
    time_t end_time = mktime(&end);

    return now >= start_time && now < end_time;
}

const char *time_as_string(time_t ntp_time)
{
    static char buffer[32];

    int bst = time_is_bst(gmtime(&ntp_time));
    time_t local_time_t = ntp_time + (bst ? 3600 : 0);
    struct tm *local = gmtime(&local_time_t);

    snprintf(buffer, sizeof(buffer), "NTP: time is %02d/%02d/%04d %02d:%02d:%02d (%s)", local->tm_mday,
             local->tm_mon + 1, local->tm_year + 1900, local->tm_hour, local->tm_min, local->tm_sec,
             bst ? "BST" : "GMT");
    return (const char *)buffer;
}

// Make an NTP request
void ntp_request(ntp_state_t *state)
{
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    // if (!p) {
    //     state->status = NTP_STATUS_MEMORY_ERROR;
    //     cyw43_arch_lwip_end();
    //     return;
    // }
    uint8_t *req = (uint8_t *)p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    // if (err != 0) {
    //     state->status = NTP_STATUS_INVALID_RESPONSE;
    //     pbuf_free(p);
    //     cyw43_arch_lwip_end();
    //     return;
    // }
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

// Called by dns_gethostbyname() after a DNS request completes
// having previously returned ERR_INPROGRESS to ntp_get_time()
void ntp_dns_callback(const char *hostname, const ip_addr_t *ipaddr, void *arg)
{
    ntp_state_t *state = (ntp_state_t *)arg;
    if (ipaddr) {
        state->ntp_server_address = *ipaddr;
        CLOCK_DEBUG("NTP: got address %s\r\n", ipaddr_ntoa(ipaddr));
        ntp_request(state);
    } else {
        state->status = NTP_STATUS_DNS_ERROR;
    }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    CLOCK_DEBUG("NTP: receiving packets\r\n");

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
extern ntp_state_t *ntp_init(void *parent_state, ntp_time_handler_t time_handler)
{
    CLOCK_DEBUG("NTP: starting init\r\n");

    ntp_state_t *state = (ntp_state_t *)calloc(1, sizeof(ntp_state_t));
    if (!state) {
        CLOCK_DEBUG("Failed to allocate NTP state\r\n");
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

ntp_status_t ntp_get_time(ntp_state_t *ntp_state)
{
    absolute_time_t start_time = get_absolute_time();

    cyw43_arch_lwip_begin();
    CLOCK_DEBUG("NTP: looking up %s\r\n", NTP_SERVER);
    int dns_status = dns_gethostbyname(NTP_SERVER, &ntp_state->ntp_server_address, ntp_dns_callback, ntp_state);
    cyw43_arch_lwip_end();

    ntp_state->status = NTP_STATUS_PENDING;
    if (dns_status == ERR_OK) {
        CLOCK_DEBUG("NTP: DNS lookup successful\r\n");
        ntp_request(ntp_state);
    } else if (dns_status != ERR_INPROGRESS) {
        CLOCK_DEBUG("NTP: DNS lookup failed with error %d\r\n", dns_status);
        return NTP_STATUS_DNS_ERROR;
    }

    while (ntp_state->status != NTP_STATUS_SUCCESS) {
        sleep_ms(500); /* wait for background lwIP */

        if (absolute_time_diff_us(start_time, get_absolute_time()) > NTP_TIMEOUT_MS * 1000) {
            CLOCK_DEBUG("NTP: DNS timed out\r\n", dns_status);
            return NTP_STATUS_TIMEOUT;
        }
    }

    return NTP_STATUS_SUCCESS;
}