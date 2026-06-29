// Pico SDK
#ifndef TEST_MODE
#include "hardware/watchdog.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "pico/cyw43_arch.h"
#else
#include "mock.h"
#endif

// Local includes
#include "clock.h"

// Make an NTP request
void ntp_request(ntp_state_t *state)
{
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    if (!p) {
        CLOCK_DEBUG("NTP: failed to allocate PBUF\r\n");
        state->error = NTP_MEMORY_ERROR;
        cyw43_arch_lwip_end();
        return;
    }
    uint8_t *req = (uint8_t *)p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    int err = udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    if (err != 0) {
        CLOCK_DEBUG("NTP: send error %d\r\n", err);
        state->error = NTP_PROTOCOL_ERROR;
        pbuf_free(p);
        cyw43_arch_lwip_end();
        return;
    }
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

// Called by dns_gethostbyname() after a DNS request completes
// having previously returned ERR_INPROGRESS to ntp_get_time()
static void ntp_dns_callback(const char *hostname, const ip_addr_t *ipaddr, void *arg)
{
    ntp_state_t *state = (ntp_state_t *)arg;
    (void)hostname;
    if (ipaddr) {
        state->ntp_server_address = *ipaddr;
        ntp_request(state);
    } else {
        CLOCK_DEBUG("NTP: DNS error for %s\r\n", hostname);
        state->error = NTP_DNS_ERROR;
    }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    ntp_state_t *state = (ntp_state_t *)arg;
    (void)pcb;

    watchdog_update();

    uint8_t first_byte = pbuf_get_at(p, 0);
    uint8_t mode = first_byte & 0x07;
    uint8_t leap = (first_byte >> 6) & 0x03;
    uint8_t stratum = pbuf_get_at(p, 1);

    int addrs_valid = ip_addr_cmp(addr, &state->ntp_server_address);
    int response_valid = port == NTP_PORT && p->tot_len == NTP_MSG_LEN && mode == 0x4;
    int packet_valid = addrs_valid && response_valid;

    if (!packet_valid) {
        CLOCK_DEBUG("NTP: invalid response: addrs %s, port=%d, len=%d, mode=0x%x, stratum=0x%x, leap=0x%x\r\n",
                    addrs_valid ? "valid" : "invalid", port, p->tot_len, mode, stratum, leap);
        state->error = NTP_PROTOCOL_ERROR;
    } else if (leap == 3) {
        CLOCK_DEBUG("NTP: server unsynchronized: addrs %s, port=%d, len=%d, mode=0x%x, stratum=0x%x, leap=0x%x\r\n",
                    addrs_valid ? "valid" : "invalid", port, p->tot_len, mode, stratum, leap);
        state->error = NTP_PROTOCOL_ERROR;
    } else if (stratum == 0) {
        // We got a 'kiss of death' from the NTP server for too many requests.
        state->status = NTP_KOD;
        CLOCK_DEBUG("NTP: server responded with KoD\r\n");
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

        state->status = NTP_DONE;
        CLOCK_DEBUG("NTP: update success timestamp=%llu\r\n", seconds_since_1970);
        state->time_handler(state->parent_state, &seconds_since_1970);
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

    cyw43_arch_lwip_begin();
    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    cyw43_arch_lwip_end();

    if (!state->ntp_pcb) {
        CLOCK_DEBUG("Failed to allocate UDP buffer\r\n");
        free(state);
        return NULL;
    }

    cyw43_arch_lwip_begin();
    udp_recv(state->ntp_pcb, ntp_recv, state);
    cyw43_arch_lwip_end();

    return state;
}

ntp_error_t ntp_get_time(ntp_state_t *state)
{
    absolute_time_t start_time = get_absolute_time();

    state->status = NTP_PENDING;
    state->error = NTP_OK;

    cyw43_arch_lwip_begin();
    char *ntp_server = ((clock_state_t *)state->parent_state)->clock_config.ntp_server;
    int ntp_timeout_ms = ((clock_state_t *)state->parent_state)->clock_config.ntp_timeout;
    int dns_status = dns_gethostbyname(ntp_server, &state->ntp_server_address, ntp_dns_callback, state);
    cyw43_arch_lwip_end();

    if (dns_status == ERR_OK) {
        ntp_request(state);
    } else if (dns_status != ERR_INPROGRESS) {
        CLOCK_DEBUG("NTP: DNS lookup failed with error %d\r\n", dns_status);
        return NTP_DNS_ERROR;
    }

    // Wait for async NTP request to complete or timeout
    while (state->status == NTP_PENDING && state->error == NTP_OK) {
        watchdog_update();
        sleep_ms(500);

        if (absolute_time_diff_us(start_time, get_absolute_time()) > ntp_timeout_ms * 1000) {
            CLOCK_DEBUG("NTP: DNS timed out after %d seconds\r\n", ntp_timeout_ms / 1000);
            return NTP_TIMEOUT_ERROR;
        }
    }
    return state->error;
}
