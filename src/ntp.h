#pragma once

#include <stdint.h>
#include <time.h>

#ifndef TEST_MODE
#include "lwip/ip_addr.h"
#else
#include "mock.h"
#endif

typedef enum
{
    NTP_OK,             // No error status
    NTP_INIT_ERROR,     // NTP failed to initialise
    NTP_DNS_ERROR,      // DNS lookup failed
    NTP_TIMEOUT_ERROR,  // Too many NTP timeouts occurred
    NTP_PROTOCOL_ERROR, // UDP protocol error happened in NTP processing
    NTP_MEMORY_ERROR,   // Failed to allocate memory for UDP packets
    NTP_NULL_ERROR = -1 // Indicates absence of status
} ntp_error_t;

typedef enum
{
    NTP_IDLE,            // No NTP request is currently active
    NTP_KOD,             // Server sent 'kiss of death' to tell us to back off
    NTP_SUCCESS,         // NTP lookup completed successfully
    NTP_PENDING,         // Waiting for NTP until timeout
    NTP_FAILED,          // Unable to update NTP
    NTP_NULL_STATUS = -1 // Indicates absence of status
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

typedef struct
{
    ntp_error_t error;
    ntp_status_t status;
    uint32_t request_start_ms;
} ntp_atomic_state_t;

extern void ntp_request(ntp_state_t *state);

extern bool ntp_init(ntp_state_t *state, void *parent_state, ntp_time_handler_t time_handler);

extern ntp_error_t ntp_request_async(ntp_state_t *state);

extern void ntp_atomic_status(ntp_state_t *state, ntp_status_t status, ntp_error_t error, uint32_t ms,
                              ntp_atomic_state_t *atomic_state);

extern const char *ntp_error_to_string(ntp_error_t status);
