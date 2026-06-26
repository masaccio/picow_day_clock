// Pico SDK
#ifndef TEST_MODE
#include "dhcpserver.h"
#include "dnsserver.h"
#include "hardware/watchdog.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#else
#include "mock.h"
#include "test.h"
#endif
#include "html_form.h"

#include <string.h>

// Local includes
#include "clock.h"
#include "config.h"

uint wifi_is_initialized = 0;
err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);

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

typedef struct
{
    struct tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
    store_config_handler_t store_config;
} tcp_server_t;

typedef struct
{
    struct tcp_pcb *pcb;
    int sent_len;
    char headers[8192];
    char result[8192];
    int header_len;
    int result_len;
    ip_addr_t *gw;
    store_config_handler_t store_config;
    tcp_server_t *server_state;
} tcp_connect_state_t;

// For testing we just call tcp_server_recv() directly with payloads to avoid mocking
// a lot of lwIP infrastructure. We can also ignore DNS and DHCP.
#ifndef TEST_MODE
static err_t tcp_close_client_connection(tcp_connect_state_t *con_state, struct tcp_pcb *client_pcb, err_t close_err)
{
    if (client_pcb) {
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

static void tcp_server_close(tcp_server_t *state)
{
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
    tcp_connect_state_t *con_state = (tcp_connect_state_t *)arg;
    CLOCK_DEBUG("tcp_server_sent %u\n", len);
    con_state->sent_len += len;
    if (con_state->sent_len >= con_state->header_len + con_state->result_len) {
        CLOCK_DEBUG("all done\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    return ERR_OK;
}

static err_t tcp_server_poll(void *arg, struct tcp_pcb *pcb)
{
    tcp_connect_state_t *con_state = (tcp_connect_state_t *)arg;
    CLOCK_DEBUG("tcp_server_poll_fn\n");
    return tcp_close_client_connection(con_state, pcb, ERR_OK); // Just disconnect client?
}

static void tcp_server_err(void *arg, err_t err)
{
    tcp_connect_state_t *con_state = (tcp_connect_state_t *)arg;
    if (err != ERR_ABRT) {
        CLOCK_DEBUG("tcp_client_err_fn %d\n", err);
        tcp_close_client_connection(con_state, con_state->pcb, err);
    }
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err)
{
    tcp_server_t *state = (tcp_server_t *)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        CLOCK_DEBUG("failure in accept\n");
        return ERR_VAL;
    }
    CLOCK_DEBUG("client connected\n");

    // Create the state for the connection
    tcp_connect_state_t *con_state = calloc(1, sizeof(tcp_connect_state_t));
    if (!con_state) {
        CLOCK_DEBUG("failed to allocate connect state\n");
        return ERR_MEM;
    }
    con_state->store_config = state->store_config;
    con_state->server_state = state;
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
    tcp_server_t *state = (tcp_server_t *)arg;
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

    return true;
}
#else

static err_t tcp_close_client_connection(tcp_connect_state_t *con_state, struct tcp_pcb *client_pcb, err_t close_err)
{
    (void)con_state;
    (void)client_pcb;
    return close_err;
}

extern mock_context_t mock_ctx;

static bool tcp_server_open(void *arg, const char *ssid)
{
    (void)arg;
    (void)ssid;
    if (mock_ctx.inject.tcp_open_fail) {
        return false;
    } else {
        return true;
    }
}

static void tcp_server_close(tcp_server_t *state)
{
    (void)state;
}
#endif // !TEST_MODE

static char hex_to_int(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return (char)0;
}

static void urldecode_inplace(char *str)
{
    char *dst = str;
    char *src = str;

    while (*src) {
        if ((*src == '%') && src[1] && src[2]) {
            // Cast the final result of the integer math to (char)
            *dst++ = (char)((hex_to_int(src[1]) << 4) | hex_to_int(src[2]));
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    (void)err;
    tcp_connect_state_t *con_state = (tcp_connect_state_t *)arg;
    if (!p) {
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }

    if (p->tot_len > 0) {
        size_t current_len = strlen(con_state->headers);
        size_t available_space = sizeof(con_state->headers) - current_len - 1;
        size_t copy_len = p->tot_len > available_space ? available_space : p->tot_len;

        pbuf_copy_partial(p, con_state->headers + current_len, (u16_t)copy_len, 0);
        con_state->headers[current_len + copy_len] = '\0';

        tcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);

    char *request = con_state->headers;

    if (strncmp(request, "POST ", 5) == 0) {
        char *content_len_ptr = strstr(request, "Content-Length: ");
        int expected_body_len = 0;
        if (content_len_ptr) {
            expected_body_len = atoi(content_len_ptr + 16);
        }

        char *body = strstr(request, "\r\n\r\n");

        // If the double line-break hasn't arrived, or the body payload has't arrived yet
        if (!body || (int)strlen(body + 4) < expected_body_len) {
            CLOCK_DEBUG("POST incomplete (%d/%d bytes accumulated)\r\n", body ? (int)strlen(body + 4) : 0,
                        expected_body_len);
            return ERR_OK;
        }
    } else {
        // For GET or other single-packet requests, make sure we at least have the HTTP line terminator
        if (!strstr(request, "\r\n")) {
            CLOCK_DEBUG("Incomplete HTTP headers. Waiting for next packet.\n");
            return ERR_OK;
        }
    }

    // Route A: Form submitted with new configuration parameters
    if (strncmp(request, "POST ", 5) == 0) {
        clock_config_t config = {0};
        config.dst_rule = DST_RULE_EU;

        char *body = strstr(request, "\r\n\r\n");
        if (body) {
            // Step past the double line-break boundary
            body += 4;

            char *saveptr;
            char *pair = strtok_r(body, "&", &saveptr);
            while (pair != NULL) {
                char *eq = strchr(pair, '=');
                if (eq) {
                    *eq = '\0';
                    char *key = pair;
                    char *value = eq + 1;

                    urldecode_inplace(value);
                    CLOCK_DEBUG("Config: %s=%s\r\n", key, value);

                    if (strcmp(key, "ssid") == 0)
                        strncpy(config.wifi_ssid, value, sizeof(config.wifi_ssid) - 1);
                    else if (strcmp(key, "pwd") == 0)
                        strncpy(config.wifi_password, value, sizeof(config.wifi_password) - 1);
                    else if (strcmp(key, "ntp") == 0)
                        strncpy(config.ntp_server, value, sizeof(config.ntp_server) - 1);
                    else if (strcmp(key, "tz") == 0)
                        config.tz_offset_mins = (int16_t)atoi(value);
                    else if (strcmp(key, "dst") == 0) {
                        if (strcmp(value, "NA") == 0)
                            config.dst_rule = DST_RULE_NA;
                        else if (strcmp(value, "EU") == 0)
                            config.dst_rule = DST_RULE_EU;
                        else if (strcmp(value, "AU") == 0)
                            config.dst_rule = DST_RULE_AU;
                        else if (strcmp(value, "NZ") == 0)
                            config.dst_rule = DST_RULE_NZ;
                        else if (strcmp(value, "CL") == 0)
                            config.dst_rule = DST_RULE_CL;
                        else if (strcmp(value, "IL") == 0)
                            config.dst_rule = DST_RULE_IL;
                        else
                            config.dst_rule = DST_RULE_NONE;
                    } else if (strcmp(key, "cto") == 0)
                        config.timeout = atoi(value);
                }
                pair = strtok_r(NULL, "&", &saveptr);
            }
        }

        // Pass structured variables down into your custom configuration callback
        if (con_state->store_config(&config) == 0) {
            const char *success_msg = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\nSaved! Rebooting...";
            tcp_write(pcb, success_msg, (u16_t)strlen(success_msg), 0);
            con_state->server_state->complete = true;
        } else {
            const char *fail_msg = "HTTP/1.1 500 Error\r\nConnection: close\r\n\r\nFailed to save to Flash.";
            tcp_write(pcb, fail_msg, (u16_t)strlen(fail_msg), 0);
        }
    }
    // Route B: Configuration page
    else if (strncmp(request, "GET " HTTP_CONFIG_URL, 4 + sizeof(HTTP_CONFIG_URL) - 1) == 0 ||
             strncmp(request, "GET / ", 6) == 0) {
        con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_OK_HEADER);

        tcp_write(pcb, con_state->headers, (u16_t)con_state->header_len, TCP_WRITE_FLAG_MORE);
        tcp_write(pcb, form_html, (u16_t)form_html_len, 0);
    }
    // Route C: Background OS pages / captive portal
    else {
        con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_REDIRECT_HEADER,
                                         ipaddr_ntoa(con_state->gw));
        tcp_write(pcb, con_state->headers, (u16_t)con_state->header_len, 0);
    }

    tcp_output(pcb);

    return ERR_OK;
}

static char *get_ssid(void)
{
    uint8_t mac[6];

    cyw43_arch_enable_sta_mode();
    cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac);
    cyw43_arch_disable_sta_mode();

    static char ssid[32];
    snprintf(ssid, sizeof(ssid), "%s-%02X%02X%02X", WIFI_AP_SSID_PREFIX, mac[3], mac[4], mac[5]);
    CLOCK_DEBUG("SSID=%s\n", ssid);
    return ssid;
}

#ifdef TEST_MODE
void *create_test_config(void *arg)
{
    static tcp_connect_state_t *state;
    state = (tcp_connect_state_t *)calloc(1, sizeof(tcp_connect_state_t));
    state->server_state = (tcp_server_t *)calloc(1, sizeof(tcp_server_t));
    state->store_config = (store_config_handler_t)arg;
    return (void *)state;
}

void clear_test_config(void *arg)
{
    tcp_connect_state_t *con_state = (tcp_connect_state_t *)arg;
    con_state->headers[0] = '\0';
    con_state->result[0] = '\0';
    con_state->header_len = 0;
    con_state->result_len = 0;
}

void free_test_config(void *arg)
{
    tcp_connect_state_t *con_state = (tcp_connect_state_t *)arg;

    if (con_state) {
        free(con_state->server_state);
        free(con_state);
    }
}
#endif

wifi_error_t start_wifi_access_point(store_config_handler_t store_config)
{
    CLOCK_DEBUG("Starting access point for configuration\r\n");

    tcp_server_t *state = (tcp_server_t *)calloc(1, sizeof(tcp_server_t));
    if (!state) {
        CLOCK_DEBUG("Failed to allocate Wi-Fi state\r\n");
        return WIFI_INIT_ERROR;
    }
    state->store_config = store_config;
    state->complete = false;

    if (!wifi_is_initialized && cyw43_arch_init() != 0) {
        CLOCK_DEBUG("Failed to init Wi-Fi\r\n");
        free(state);
        return WIFI_INIT_ERROR;
    }
    wifi_is_initialized = 1;

    char *ssid = get_ssid();
    cyw43_arch_enable_ap_mode(ssid, NULL, CYW43_AUTH_WPA2_AES_PSK);

    ip4_addr_t mask;
    state->gw.addr = PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS);
    mask.addr = PP_HTONL(CYW43_DEFAULT_IP_MASK);

    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &state->gw, &mask);

    dns_server_t dns_server;
    dns_server_init(&dns_server, &state->gw);

    if (!tcp_server_open(state, ssid)) {
        CLOCK_DEBUG("Failed to open server\n");
        free(state);
        return WIFI_INIT_ERROR;
    }

    while (!state->complete) {
        watchdog_update();
        sleep_ms(1000);
    }

    cyw43_arch_disable_ap_mode();
    tcp_server_close(state);
    dns_server_deinit(&dns_server);
    dhcp_server_deinit(&dhcp_server);

    CLOCK_DEBUG("start_wifi_access_point DONE\r\n");
    free(state);
    return WIFI_OK;
}
