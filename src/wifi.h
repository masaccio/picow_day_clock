#pragma once

#include "clock_config.h"

typedef enum
{
    WIFI_OK,            // Everything is OK
    WIFI_INIT_ERROR,    // Memory allocation failed
    WIFI_TIMEOUT_ERROR, // Too many Wi-Fi timeouts
    WIFI_AUTH_ERROR,    // Too many authentication errors
    WIFI_CONNECT_ERROR, // A TCP/IP connection error
    WIFI_UNKNOWN_ERROR, // Any other Wi-Fi related error
} wifi_error_t;

typedef struct
{
    struct tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
    int (*store_config)(void *, bool);
    int active_connections;
} tcp_server_t;

typedef struct
{
    struct tcp_pcb *pcb;
    int sent_len;
    char headers[TCP_IP_BUFFER_SIZE];
    char result[TCP_IP_BUFFER_SIZE];
    int header_len;
    int result_len;
    ip_addr_t *gw;
    int (*store_config)(void *, bool);
    tcp_server_t *server_state;
} tcp_connect_state_t;

extern wifi_error_t connect_to_wifi(const char ssid[], const char password[], bool *wifi_initialized);

extern wifi_error_t start_wifi_access_point(int (*store_config)(void *, bool), bool *wifi_initialized);

extern const char *wifi_error_to_string(wifi_error_t status);

// Export for testing
extern err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err);
