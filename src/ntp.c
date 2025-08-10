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

// Called with results of operation
void ntp_result(ntp_state_t *state, int status, time_t *result) {
    if (status == 0 && result) {
        state->time_handler(state->parent_state, result);
    }

    if (state->ntp_resend_alarm > 0) {
        cancel_alarm(state->ntp_resend_alarm);
        state->ntp_resend_alarm = 0;
    }
    state->ntp_test_time = make_timeout_time_ms(NTP_TEST_TIME);
    state->dns_request_sent = false;
}

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

int64_t ntp_failed_handler(alarm_id_t id, void *user_data) {
    ntp_state_t *state = (ntp_state_t *)user_data;
    state->error_handler(state->parent_state, "NTP request failed");
    ntp_result(state, -1, NULL);
    return 0;
}

// Call back with a DNS result
void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    ntp_state_t *state = (ntp_state_t *)arg;
    if (ipaddr) {
        state->ntp_server_address = *ipaddr;
        printf("NTP address: %s\r\n", ipaddr_ntoa(ipaddr));
        ntp_request(state);
    } else {
        state->error_handler(state->parent_state, "NTP DNS request failed");
        ntp_result(state, -1, NULL);
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
        ntp_result(state, 0, &epoch);
    } else {
        state->error_handler(state->parent_state, "NTP response error");
        ntp_result(state, -1, NULL);
    }
    pbuf_free(p);
}

// Perform initialisation
extern ntp_state_t *ntp_init(void *parent_state, ntp_time_handler_t time_handler, ntp_error_handler_t error_handler) {

    ntp_state_t *state = (ntp_state_t *)calloc(1, sizeof(ntp_state_t));
    if (!state) {
        printf("Failed to allocate NTP state\r\n");
        return NULL;
    }
    state->parent_state = parent_state;
    state->time_handler = time_handler;
    state->error_handler = error_handler;

    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state->ntp_pcb) {
        state->error_handler(state->parent_state, "Failed to create UDP PCB");
        free(state);
        return NULL;
    }
    udp_recv(state->ntp_pcb, ntp_recv, state);
    return state;
}
