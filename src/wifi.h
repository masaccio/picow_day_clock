#pragma once

#ifndef TEST_MODE
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#else
#include "mock.h"
#endif

#include "config.h"

struct flash_config_t;

typedef bool (*store_config_handler_t)(struct flash_config_t *config, bool invalidate);

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
    store_config_handler_t store_config;
    struct flash_config_t *flash_config;
    char *form_html;
    u16_t form_html_len;
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
    store_config_handler_t store_config;
    struct flash_config_t *flash_config;
    char *form_html;
    u16_t form_html_len;
    tcp_server_t *server_state;
} tcp_connect_state_t;

extern wifi_error_t connect_to_wifi(const char ssid[], const char password[], bool *wifi_initialized);

extern wifi_error_t start_wifi_access_point(struct flash_config_t *flash_config, store_config_handler_t store_config,
                                            bool *wifi_initialized);

extern const char *wifi_error_to_string(wifi_error_t status);

// Export for testing
extern err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err);
