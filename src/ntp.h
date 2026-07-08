#pragma once

#include <stdint.h>

typedef enum
{
    NTP_OK,
    NTP_INIT_ERROR,
    NTP_DNS_ERROR,
    NTP_TIMEOUT_ERROR,
    NTP_PROTOCOL_ERROR,
    NTP_MEMORY_ERROR,
} ntp_error_t;

typedef enum
{
    NTP_IDLE,    // No NTP request is currently active
    NTP_KOD,     // Server sent 'kiss of death' to tell us to back off
    NTP_SUCCESS, // NTP lookup completed successfully
    NTP_PENDING, // Waiting for NTP until timeout
    NTP_FAILED,  // Unable to update NTP
} ntp_status_t;

typedef void (*ntp_time_handler_t)(void *state, time_t *time);

typedef struct ntp_state_t
{
    ntp_time_handler_t time_handler;
    uint16_t ntp_port;
    ip_addr_t ntp_server_address;
    struct udp_pcb *ntp_pcb;
    void *parent_state;
    uint32_t request_start_ms;
    volatile ntp_error_t error;
    volatile ntp_status_t status;
} ntp_state_t;

extern void ntp_request(ntp_state_t *state);

extern ntp_state_t *ntp_init(void *parent_state, ntp_time_handler_t time_handler);

extern ntp_error_t ntp_request_async(ntp_state_t *state);

extern const char *ntp_error_to_string(ntp_error_t status);
