/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
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

// Determines if given UTC time is in British Summer Time (BST)
int is_bst(struct tm *utc) {
    if (!utc) return false;

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

// Called with results of operation
void ntp_result(NTP_T *state, int status, time_t *result) {
    if (status == 0 && result) {
        int bst = is_bst(gmtime(result));
        time_t local_time_t = *result + (bst ? 3600 : 0);
        struct tm *local = gmtime(&local_time_t);

        printf("NTP time is %02d/%02d/%04d %02d:%02d:%02d (%s)\n", local->tm_mday, local->tm_mon + 1,
               local->tm_year + 1900, local->tm_hour, local->tm_min, local->tm_sec, bst ? "BST" : "GMT");
    }

    if (state->ntp_resend_alarm > 0) {
        cancel_alarm(state->ntp_resend_alarm);
        state->ntp_resend_alarm = 0;
    }
    state->ntp_test_time = make_timeout_time_ms(NTP_TEST_TIME);
    state->dns_request_sent = false;
}

// Make an NTP request
void ntp_request(NTP_T *state) {
    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure
    // correct locking. You can omit them if you are in a callback from lwIP. Note
    // that when using pico_cyw_arch_poll these calls are a no-op and can be
    // omitted, but it is a good practice to use them in case you switch the
    // cyw43_arch type later.
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
    NTP_T *state = (NTP_T *)user_data;
    printf("ntp request failed\n");
    ntp_result(state, -1, NULL);
    return 0;
}

// Call back with a DNS result
void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    NTP_T *state = (NTP_T *)arg;
    if (ipaddr) {
        state->ntp_server_address = *ipaddr;
        printf("NTP address: %s\n", ipaddr_ntoa(ipaddr));
        ntp_request(state);
    } else {
        printf("NTP DNS request failed\n");
        ntp_result(state, -1, NULL);
    }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    NTP_T *state = (NTP_T *)arg;
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
        printf("invalid ntp response\n");
        ntp_result(state, -1, NULL);
    }
    pbuf_free(p);
}

// Perform initialisation
NTP_T *ntp_init(void) {
    NTP_T *state = (NTP_T *)calloc(1, sizeof(NTP_T));
    if (!state) {
        printf("failed to allocate state\n");
        return NULL;
    }
    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state->ntp_pcb) {
        printf("failed to create pcb\n");
        free(state);
        return NULL;
    }
    udp_recv(state->ntp_pcb, ntp_recv, state);
    return state;
}
