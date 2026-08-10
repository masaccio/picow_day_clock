// Pico SDK
#ifndef TEST_MODE
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "pico/cyw43_arch.h"
#include "pico/time.h"
#else
#include "mock.h"
#endif

// Local includes
#include "clock.h"

void ntp_atomic_status(ntp_state_t *state, ntp_status_t status, ntp_error_t error, uint32_t ms,
                       ntp_atomic_state_t *atomic_state)
{
    uint32_t ints = save_and_disable_interrupts();
    if (status != NTP_NULL_STATUS) {
        state->status = status;
    }
    if (error != NTP_NULL_ERROR) {
        state->error = error;
    }
    if (ms != 0) {
        state->request_start_ms = ms;
    }

    if (atomic_state != NULL) {
        atomic_state->status = state->status;
        atomic_state->error = state->error;
        atomic_state->request_start_ms = state->request_start_ms;
    }
    restore_interrupts(ints);
}

const char *ntp_error_to_string(ntp_error_t status)
{
    switch (status) {
        STATUS_CASE(NTP_OK)
        STATUS_CASE(NTP_INIT_ERROR)
        STATUS_CASE(NTP_DNS_ERROR)
        STATUS_CASE(NTP_TIMEOUT_ERROR)
        STATUS_CASE(NTP_PROTOCOL_ERROR)
        STATUS_CASE(NTP_MEMORY_ERROR)
        STATUS_CASE(NTP_NULL_ERROR)
    }
    return "UNKNOWN_STATUS";
}

// Make an NTP request
void ntp_request(ntp_state_t *state)
{
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    if (!p) {
        CLOCK_DEBUG("NTP: failed to allocate PBUF\r\n");
        ntp_atomic_status(state, NTP_FAILED, NTP_MEMORY_ERROR, 0, NULL);
        cyw43_arch_lwip_end();
        return;
    }
    uint8_t *req = (uint8_t *)p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;

    CLOCK_DEBUG("NTP request to %s:%d\r\n", ipaddr_ntoa(&state->ntp_server_address), state->ntp_port);
    int err = udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, state->ntp_port);
    if (err != 0) {
        CLOCK_DEBUG("NTP: send error %d\r\n", err);
        ntp_atomic_status(state, NTP_FAILED, NTP_PROTOCOL_ERROR, 0, NULL);
        pbuf_free(p);
        cyw43_arch_lwip_end();
        return;
    }
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

// Called by dns_gethostbyname() after a DNS request completes having previously returned ERR_INPROGRESS
static void ntp_dns_callback(const char *hostname, const ip_addr_t *ipaddr, void *arg)
{
    ntp_state_t *state = (ntp_state_t *)arg;
    (void)hostname;
    if (ipaddr && ipaddr->addr) {
        state->ntp_server_address = *ipaddr;
        ntp_request(state);
    } else {
        CLOCK_DEBUG("NTP: DNS error for %s\r\n", hostname);
        ntp_atomic_status(state, NTP_FAILED, NTP_DNS_ERROR, 0, NULL);
    }
}

// Called by lwIP when a UDP datagram is received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    ntp_state_t *state = (ntp_state_t *)arg;
    (void)pcb;

    uint8_t first_byte = pbuf_get_at(p, 0);
    uint8_t mode = first_byte & 0x07;
    uint8_t leap = (first_byte >> 6) & 0x03;
    uint8_t stratum = pbuf_get_at(p, 1);

    int addrs_valid = ip_addr_cmp(addr, &state->ntp_server_address);
    int response_valid = port == state->ntp_port && p->tot_len == NTP_MSG_LEN && mode == 0x4;
    int packet_valid = addrs_valid && response_valid;

    if (!addrs_valid) {
        CLOCK_DEBUG("NTP: DNS lookup failed\r\n");
        ntp_atomic_status(state, NTP_FAILED, NTP_DNS_ERROR, 0, NULL);
    } else if (!packet_valid) {
        CLOCK_DEBUG("NTP: invalid response: addrs %s, port=%d, len=%d, mode=0x%x, stratum=0x%x, leap=0x%x\r\n",
                    addrs_valid ? "valid" : "invalid", port, p->tot_len, mode, stratum, leap);
        ntp_atomic_status(state, NTP_FAILED, NTP_PROTOCOL_ERROR, 0, NULL);
    } else if (stratum == 0) {
        // We got a 'kiss of death' from the NTP server for too many requests.
        ntp_atomic_status(state, NTP_KOD, NTP_OK, 0, NULL);
        CLOCK_DEBUG("NTP: server responded with KoD\r\n");
    } else if (leap == 3) {
        CLOCK_DEBUG("NTP: server unsynchronized: addrs %s, port=%d, len=%d, mode=0x%x, stratum=0x%x, leap=0x%x\r\n",
                    addrs_valid ? "valid" : "invalid", port, p->tot_len, mode, stratum, leap);
        ntp_atomic_status(state, NTP_FAILED, NTP_PROTOCOL_ERROR, 0, NULL);
    } else {
        // Also allows leap to be 0b01 or 0b10 and rather than adjusting for leap seconds, we just
        // get a new timestamp the next day.
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 = (uint32_t)(((uint32_t)seconds_buf[0] << 24) | ((uint32_t)seconds_buf[1] << 16) |
                                                 ((uint32_t)seconds_buf[2] << 8) | ((uint32_t)seconds_buf[3]));
        time_t seconds_since_1970;
        if (seconds_since_1900 < NTP_DELTA) {
            // Era 1 (2036-)
            uint64_t era_seconds = (uint64_t)seconds_since_1900 + 0x100000000ULL;
            seconds_since_1970 = (time_t)(era_seconds - NTP_DELTA);
        } else {
            // Era 0 (1900-2036)
            seconds_since_1970 = (time_t)(seconds_since_1900 - NTP_DELTA);
        }

        ntp_atomic_status(state, NTP_SUCCESS, NTP_OK, 0, NULL);
        CLOCK_DEBUG("NTP: update success ntp=%lu, epoch=%llu\r\n", seconds_since_1900, seconds_since_1970);
        state->time_handler(state->parent_state, &seconds_since_1970);
    }

    pbuf_free(p);
}

extern bool ntp_init(ntp_state_t *state, void *parent_state, ntp_time_handler_t time_handler)
{
    memset(state, 0, sizeof(ntp_state_t));
    state->parent_state = parent_state;
    state->time_handler = time_handler;

    cyw43_arch_lwip_begin();
    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    cyw43_arch_lwip_end();

    if (!state->ntp_pcb) {
        CLOCK_DEBUG("Failed to allocate UDP buffer\r\n");
        return false;
    }

    cyw43_arch_lwip_begin();
    udp_recv(state->ntp_pcb, ntp_recv, state);
    cyw43_arch_lwip_end();

    return true;
}

ntp_error_t ntp_request_async(ntp_state_t *state)
{
    uint32_t request_start_ms = to_ms_since_boot(get_absolute_time());
    ntp_atomic_status(state, NTP_PENDING, NTP_OK, request_start_ms, NULL);

    cyw43_arch_lwip_begin();
    char *ntp_server = ((clock_state_t *)state->parent_state)->flash_config.ntp_server;
    int dns_status = dns_gethostbyname(ntp_server, &state->ntp_server_address, ntp_dns_callback, state);
    cyw43_arch_lwip_end();

    if (dns_status == ERR_OK) {
        ntp_request(state);
    } else if (dns_status != ERR_INPROGRESS) {
        CLOCK_DEBUG("NTP: DNS lookup failed with error %d\r\n", dns_status);
        return NTP_DNS_ERROR;
    }

    return NTP_OK;
}
