// Pico SDK
#ifndef TEST_MODE
#include "hardware/watchdog.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "dhcpserver.h"
#include "dnsserver.h"
#include "html_form.h"
#else
#include "mock.h"
#endif

#include <string.h>

// Local includes
#include "clock.h"
#include "config.h"

static uint wifi_is_initialized = 0;

wifi_error_t connect_to_wifi(const char ssid[], const char password[])
{
    if (!wifi_is_initialized && cyw43_arch_init() != 0) {
        return WIFI_INIT_ERROR;
    }
    wifi_is_initialized = 1;

    cyw43_arch_enable_sta_mode();

    absolute_time_t start_time_us = get_absolute_time();
    int bad_auth_count = 0;
    while (true) {
        watchdog_update();
        int ret = cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECT_TIMEOUT_MS);

        if (ret == 0) {
            CLOCK_DEBUG("Wi-Fi: connected to %s\r\n", ssid);
            return WIFI_OK;
        } else if (ret == PICO_ERROR_TIMEOUT) {
            if (absolute_time_diff_us(start_time_us, get_absolute_time()) >= (WIFI_ABANDON_TIMEOUT_MS * 1000)) {
                CLOCK_DEBUG("Wi-Fi: exceeded maximum timeout; giving up\r\n");
                return WIFI_TIMEOUT_ERROR;
            }
            CLOCK_DEBUG("Wi-Fi: timeout; trying again\r\n");
        } else if (ret == PICO_ERROR_BADAUTH) {
            bad_auth_count++;
            if (bad_auth_count >= WIFI_BAD_AUTH_RETRY_COUNT) {
                CLOCK_DEBUG("Wi-Fi: too many bad authentication failures; giving up\r\n");
                return WIFI_AUTH_ERROR;
            } else {
                CLOCK_DEBUG("Wi-Fi: invalid credentials; retrying...\r\n");
                sleep_ms(WIFI_BAD_AUTH_RETRY_DELAY_MS);
            }
        } else if (ret == PICO_ERROR_CONNECT_FAILED) {
            CLOCK_DEBUG("Wi-Fi: connection failed for unknown reason; giving up\r\n");
            return WIFI_CONNECT_ERROR;
        } else {
            CLOCK_DEBUG("Wi-Fi: unknown error %d; giving up\r\n", ret);
            return WIFI_UNKNOWN_ERROR;
        }
    }
}

#ifndef TEST_MODE

typedef struct TCP_SERVER_T_
{
    struct tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
} TCP_SERVER_T;

typedef struct TCP_CONNECT_STATE_T_
{
    struct tcp_pcb *pcb;
    int sent_len;
    char headers[256];
    char result[8192];
    int header_len;
    int result_len;
    ip_addr_t *gw;
} TCP_CONNECT_STATE_T;

static err_t tcp_close_client_connection(TCP_CONNECT_STATE_T *con_state, struct tcp_pcb *client_pcb, err_t close_err)
{
    if (client_pcb) {
        assert(con_state && con_state->pcb == client_pcb);
        tcp_arg(client_pcb, NULL);
        tcp_poll(client_pcb, NULL, 0);
        tcp_sent(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);
        err_t err = tcp_close(client_pcb);
        if (err != ERR_OK) {
            CLOCK_DEBUG("close failed %d, calling abort\n", err);
            tcp_abort(client_pcb);
            close_err = ERR_ABRT;
        }
        if (con_state) {
            free(con_state);
        }
    }
    return close_err;
}

static void tcp_server_close(TCP_SERVER_T *state)
{
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T *)arg;
    CLOCK_DEBUG("tcp_server_sent %u\n", len);
    con_state->sent_len += len;
    if (con_state->sent_len >= con_state->header_len + con_state->result_len) {
        CLOCK_DEBUG("all done\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    return ERR_OK;
}

static int test_server_content(const char *request, const char *params, char *result, size_t max_result_len)
{
    int len = 0;
    if (strncmp(request, HTTP_CONFIG_URL, sizeof(HTTP_CONFIG_URL) - 1) == 0) {
        CLOCK_DEBUG("Found clock request\r\n");
        strncpy(result, form_html, max_result_len);
        len = form_html_len;
    } else {
        CLOCK_DEBUG("Not handling request: %s\r\n", request);
    }
    return len;
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T *)arg;
    if (!p) {
        CLOCK_DEBUG("connection closed\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    assert(con_state && con_state->pcb == pcb);
    if (p->tot_len > 0) {
        CLOCK_DEBUG("tcp_server_recv %d err %d\n", p->tot_len, err);
        // Copy the request into the buffer
        pbuf_copy_partial(p, con_state->headers,
                          p->tot_len > sizeof(con_state->headers) - 1 ? sizeof(con_state->headers) - 1 : p->tot_len, 0);

        // Handle GET request
        if (strncmp("GET", con_state->headers, 3) == 0) {
            char *request = con_state->headers + 4; // + space
            char *params = strchr(request, '?');
            if (params) {
                if (*params) {
                    char *space = strchr(request, ' ');
                    *params++ = 0;
                    if (space) {
                        *space = 0;
                    }
                } else {
                    params = NULL;
                }
            }

            // Generate content
            con_state->result_len = test_server_content(request, params, con_state->result, sizeof(con_state->result));
            CLOCK_DEBUG("Request: %s?%s\n", request, params);
            CLOCK_DEBUG("Result: %d\n", con_state->result_len);

            // Check we had enough buffer space
            if (con_state->result_len > sizeof(con_state->result) - 1) {
                CLOCK_DEBUG("Too much result data %d\n", con_state->result_len);
                return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
            }

            // Generate web page
            if (con_state->result_len > 0) {
                con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers),
                                                 HTTP_RESPONSE_OK_HEADER, 200, con_state->result_len);
                if (con_state->header_len > sizeof(con_state->headers) - 1) {
                    CLOCK_DEBUG("Too much header data %d\n", con_state->header_len);
                    return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
                }
            } else {
                // Send redirect
                con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers),
                                                 HTTP_RESPONSE_REDIRECT_HEADER, ipaddr_ntoa(con_state->gw));
                CLOCK_DEBUG("Sending redirect %s", con_state->headers);
            }

            // Send the headers to the client
            con_state->sent_len = 0;
            err_t err = tcp_write(pcb, con_state->headers, con_state->header_len, 0);
            if (err != ERR_OK) {
                CLOCK_DEBUG("failed to write header data %d\n", err);
                return tcp_close_client_connection(con_state, pcb, err);
            }

            // Send the body to the client
            if (con_state->result_len) {
                err = tcp_write(pcb, con_state->result, con_state->result_len, 0);
                if (err != ERR_OK) {
                    CLOCK_DEBUG("failed to write result data %d\n", err);
                    return tcp_close_client_connection(con_state, pcb, err);
                }
            }
        }
        tcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
}

static err_t tcp_server_poll(void *arg, struct tcp_pcb *pcb)
{
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T *)arg;
    CLOCK_DEBUG("tcp_server_poll_fn\n");
    return tcp_close_client_connection(con_state, pcb, ERR_OK); // Just disconnect clent?
}

static void tcp_server_err(void *arg, err_t err)
{
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T *)arg;
    if (err != ERR_ABRT) {
        CLOCK_DEBUG("tcp_client_err_fn %d\n", err);
        tcp_close_client_connection(con_state, con_state->pcb, err);
    }
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err)
{
    TCP_SERVER_T *state = (TCP_SERVER_T *)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        CLOCK_DEBUG("failure in accept\n");
        return ERR_VAL;
    }
    CLOCK_DEBUG("client connected\n");

    // Create the state for the connection
    TCP_CONNECT_STATE_T *con_state = calloc(1, sizeof(TCP_CONNECT_STATE_T));
    if (!con_state) {
        CLOCK_DEBUG("failed to allocate connect state\n");
        return ERR_MEM;
    }
    con_state->pcb = client_pcb; // for checking
    con_state->gw = &state->gw;

    // setup connection to client
    tcp_arg(client_pcb, con_state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, HTTP_POLL_TIME_SEC * 2);
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK;
}

static bool tcp_server_open(void *arg, const char *ssid)
{
    TCP_SERVER_T *state = (TCP_SERVER_T *)arg;
    CLOCK_DEBUG("starting server on port %d\n", HTTP_TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        CLOCK_DEBUG("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, IP_ANY_TYPE, HTTP_TCP_PORT);
    if (err) {
        CLOCK_DEBUG("failed to bind to port %d\n", HTTP_TCP_PORT);
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        CLOCK_DEBUG("failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    printf("Try connecting to '%s' (press 'd' to disable access point)\n", ssid);
    return true;
}

char *get_ssid()
{
    // Get the MAC address for the Access Point interface
    uint8_t mac[6];
    static char ssid[32];
    cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_AP, mac);
    snprintf(ssid, sizeof(ssid), "Clock-%02X%02X%02X", mac[3], mac[4], mac[5]);
    return ssid;
}

int start_wifi_access_point()
{
    CLOCK_DEBUG("Starting access point for configuration\r\n");

    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        CLOCK_DEBUG("Failed to allocate Wi-Fi state\r\n");
        return WIFI_INIT_ERROR;
    }

    if (!wifi_is_initialized && cyw43_arch_init() != 0) {
        CLOCK_DEBUG("Failed to init Wi-Fi\r\n");
        return WIFI_INIT_ERROR;
    }
    wifi_is_initialized = 1;

    char *ssid = get_ssid();
    cyw43_arch_disable_sta_mode();
    cyw43_arch_enable_ap_mode(ssid, NULL, CYW43_AUTH_WPA2_AES_PSK);

    // ip4_addr_t ip, mask, gw;

    // ip4_addr_set_u32(&ip, PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS));
    // ip4_addr_set_u32(&mask, PP_HTONL(CYW43_DEFAULT_IP_MASK));
    // ip4_addr_set_u32(&gw, PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS));

#if LWIP_IPV6
#define IP(x) ((x).u_addr.ip4)
#else
#define IP(x) (x)
#endif

    ip4_addr_t mask;
    IP(state->gw).addr = PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS);
    IP(mask).addr = PP_HTONL(CYW43_DEFAULT_IP_MASK);

#undef IP

    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &state->gw, &mask);

    dns_server_t dns_server;
    dns_server_init(&dns_server, &state->gw);

    if (!tcp_server_open(state, ssid)) {
        CLOCK_DEBUG("Failed to open server\n");
        return WIFI_INIT_ERROR;
    }

    state->complete = false;
    while (!state->complete) {
        watchdog_update();
        sleep_ms(1000);
    }

    tcp_server_close(state);
    dns_server_deinit(&dns_server);
    dhcp_server_deinit(&dhcp_server);

    return STATUS_WIFI_OK;
}

#endif /* !TEST_MODE */
