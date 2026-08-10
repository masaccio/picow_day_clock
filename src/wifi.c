// Pico SDK
#ifndef TEST_MODE
#include "hardware/watchdog.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#else
#include "mock.h"
#include "test.h"
#endif

#include "dhcpserver.h"
#include "dnsserver.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Local includes
#include "clock.h"
#include "html_form.h"

static tcp_server_t server_state;
static char html_form_expanded[HTML_FORM_MAX_LENGTH] = {0};

#define MIN_TZ_OFFSET_MINS (-12 * 60)
#define MAX_TZ_OFFSET_MINS (14 * 60)
#define MIN_NTP_TIMEOUT_MS 1000
#define MAX_NTP_TIMEOUT_MS 60000

static bool parse_long_bounded(const char *value, long min_val, long max_val, long *result)
{
    char *endptr = NULL;
    long parsed = strtol(value, &endptr, 10);
    if (!value || value[0] == '\0') {
        return false;
    }
    if (!endptr || *endptr != '\0') {
        return false;
    }
    if (parsed < min_val || parsed > max_val) {
        return false;
    }

    *result = parsed;
    return true;
}

const char *wifi_error_to_string(wifi_error_t status)
{
    switch (status) {
        STATUS_CASE(WIFI_OK)
        STATUS_CASE(WIFI_INIT_ERROR)
        STATUS_CASE(WIFI_TIMEOUT_ERROR)
        STATUS_CASE(WIFI_AUTH_ERROR)
        STATUS_CASE(WIFI_CONNECT_ERROR)
        STATUS_CASE(WIFI_UNKNOWN_ERROR)
    }
    return "UNKNOWN_STATUS";
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);

wifi_error_t connect_to_wifi(const char ssid[], const char password[], bool *wifi_initialized)
{
    if (!*wifi_initialized && cyw43_arch_init() != 0) {
        return WIFI_INIT_ERROR;
    }
    *wifi_initialized = true;

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
            if (absolute_time_diff_us(start_time_us, get_absolute_time()) > (WIFI_ABANDON_TIMEOUT_MS * 1000)) {
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

static err_t tcp_close_client_connection(tcp_connect_state_t *con_state, struct tcp_pcb *client_pcb, err_t close_err)
{
    if (client_pcb) {
        if (con_state && con_state->server_state) {
            tcp_server_t *state = (tcp_server_t *)con_state->server_state;
            state->active_connections--;
        }
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
            memset(con_state, 0, sizeof(tcp_connect_state_t));
        }
    } else if (con_state) {
        // If we got here via tcp_server_err, the PCB is gone.
        // Just decrement our counter and skip touching the dead pcb.
        if (con_state->server_state) {
            tcp_server_t *state = (tcp_server_t *)con_state->server_state;
            state->active_connections--;
        }
        memset(con_state, 0, sizeof(tcp_connect_state_t));
    }
    return close_err;
}

static void tcp_server_close(void)
{
    if (server_state.server_pcb) {
        tcp_arg(server_state.server_pcb, NULL);
        tcp_close(server_state.server_pcb);
        server_state.server_pcb = NULL;
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
    return tcp_close_client_connection(con_state, pcb, ERR_OK);
}

static void tcp_server_err(void *arg, err_t err)
{
    tcp_connect_state_t *con_state = (tcp_connect_state_t *)arg;
    if (con_state != NULL) {
        tcp_close_client_connection(con_state, NULL, err);
    }
}

err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err)
{
    tcp_server_t *state = (tcp_server_t *)arg;

    if (err != ERR_OK || client_pcb == NULL) {
        CLOCK_DEBUG("failure in accept\n");
        return ERR_VAL;
    }
    CLOCK_DEBUG("client connected\n");

    state->active_connections++;
    if (state->active_connections > TCP_IP_MAX_CONNECTIONS) {
        CLOCK_DEBUG("Number TCP/IP connections exceeded maximum of %d\n", TCP_IP_MAX_CONNECTIONS);
        state->active_connections--;
        return ERR_ABRT;
    }

    // Find a free connection state slot within the provided server state
    tcp_connect_state_t *con_state = NULL;
    for (int i = 0; i < TCP_IP_MAX_CONNECTIONS; i++) {
        if (state->connections[i].pcb == NULL) {
            con_state = &state->connections[i];
            break;
        }
    }

    if (!con_state) {
        CLOCK_DEBUG("failed to allocate connect state\n");
        return ERR_MEM;
    }

    // Clean the reused state
    memset(con_state, 0, sizeof(tcp_connect_state_t));

    con_state->server_state = state;
    con_state->store_config = state->store_config;
    con_state->flash_config = state->flash_config;
    con_state->pcb = client_pcb;
    con_state->gw = &state->gw;

    // setup connection to client
    tcp_arg(client_pcb, con_state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, HTTP_POLL_TIME_SEC * 2);
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK;
}
static bool tcp_server_open(void)
{
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

    server_state.server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!server_state.server_pcb) {
        CLOCK_DEBUG("failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(server_state.server_pcb, &server_state);
    tcp_accept(server_state.server_pcb, tcp_server_accept);

    return true;
}

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

    // Backtrack the destination pointer to strip any trailing \r or \n
    while (dst > str && (*(dst - 1) == '\r' || *(dst - 1) == '\n')) {
        dst--;
    }

    *dst = '\0';
}

static err_t expand_html_template(const char *buffer, char *buffer_out, struct flash_config_t *flash_config)
{
    size_t out_len = 0;
    const char *curr = buffer;

    while (*curr != '\0') {
        const char *start = strstr(curr, "{TEMPLATE:");
        if (!start) {
            size_t tail_len = strlen(curr);
            if (out_len + tail_len >= HTML_FORM_MAX_LENGTH) {
                return ERR_MEM;
            }
            memcpy(buffer_out + out_len, curr, tail_len);
            out_len += tail_len;
            break;
        }

        size_t prefix_len = (size_t)(start - curr);
        if (out_len + prefix_len >= HTML_FORM_MAX_LENGTH) {
            return ERR_MEM;
        }
        memcpy(buffer_out + out_len, curr, prefix_len);
        out_len += prefix_len;

        const char *key_start = start + strlen("{TEMPLATE:");
        const char *end = strchr(key_start, '}');
        if (!end) {
            return ERR_VAL;
        }

        size_t key_len = (size_t)(end - key_start);
        const char *replacement = "";
        if (flash_config && key_len > 0) {
            if (strncasecmp(key_start, "ssid", key_len) == 0) {
                replacement = flash_config->wifi_ssid;
            } else if (strncasecmp(key_start, "pwd", key_len) == 0) {
                replacement = flash_config->wifi_password;
            } else if (strncasecmp(key_start, "ntp", key_len) == 0) {
                replacement = flash_config->ntp_server;
            }
        }

        size_t repl_len = strlen(replacement);
        if (out_len + repl_len >= HTML_FORM_MAX_LENGTH) {
            return ERR_MEM;
        }
        memcpy(buffer_out + out_len, replacement, repl_len);
        out_len += repl_len;

        curr = end + 1;
    }

    buffer_out[out_len] = '\0';

    return ERR_OK;
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
            long parsed = 0;
            char *line_end = strstr(content_len_ptr, "\r\n");
            if (!line_end) {
                return tcp_close_client_connection(con_state, pcb, ERR_VAL);
            }

            char content_len_buf[16] = {0};
            size_t raw_len = (size_t)(line_end - (content_len_ptr + 16));
            if (raw_len == 0 || raw_len >= sizeof(content_len_buf)) {
                return tcp_close_client_connection(con_state, pcb, ERR_VAL);
            }
            memcpy(content_len_buf, content_len_ptr + 16, raw_len);

            if (!parse_long_bounded(content_len_buf, 0, INT_MAX, &parsed)) {
                return tcp_close_client_connection(con_state, pcb, ERR_VAL);
            }
            expected_body_len = (int)parsed;
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
        bool invalid_param = false;
        flash_config_t config = {0};
        if (con_state->flash_config) {
            memcpy(&config, con_state->flash_config, sizeof(flash_config_t));
        } else {
            config.dst_rule = DST_RULE_NONE;
            config.ntp_timeout = NTP_DEFAULT_TIMEOUT_MS;
            config.ntp_port = NTP_DEFAULT_PORT;
            config.led_always_on = false; // When unchecked the parameter will not be in the POST URL
            strncpy(config.ntp_server, NTP_DEFAULT_SERVER, sizeof(config.ntp_server) - 1);
        }

        char *body = strstr(request, "\r\n\r\n");
        if (body) {
            // Step past the double line-break boundary
            body += 4;

            size_t body_len = strlen(body);
            if (body_len >= TCP_IP_BUFFER_SIZE) {
                return tcp_close_client_connection(con_state, pcb, ERR_MEM);
            }
            char body_copy[TCP_IP_BUFFER_SIZE];
            memcpy(body_copy, body, body_len + 1);

            char *saveptr;
            char *pair = strtok_r(body_copy, "&", &saveptr);
            while (pair != NULL) {
                char *eq = strchr(pair, '=');
                if (eq) {
                    *eq = '\0';
                    char *key = pair;
                    char *value = eq + 1;

                    urldecode_inplace(value);
                    CLOCK_DEBUG("Config: %s=%s\r\n", key, value);

                    if (strcmp(key, "ssid") == 0) {
                        strncpy(config.wifi_ssid, value, sizeof(config.wifi_ssid) - 1);
                    } else if (strcmp(key, "pwd") == 0) {
                        strncpy(config.wifi_password, value, sizeof(config.wifi_password) - 1);
                    } else if (strcmp(key, "ntp") == 0) {
                        strncpy(config.ntp_server, value, sizeof(config.ntp_server) - 1);
                    } else if (strcmp(key, "tz") == 0) {
                        long tz_mins = 0;
                        if (!parse_long_bounded(value, MIN_TZ_OFFSET_MINS, MAX_TZ_OFFSET_MINS, &tz_mins)) {
                            invalid_param = true;
                            break;
                        }
                        config.tz_offset_mins = (int16_t)tz_mins;
                    } else if (strcmp(key, "dst") == 0) {
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
                    } else if (strcmp(key, "cto") == 0) {
                        long timeout_ms = 0;
                        if (!parse_long_bounded(value, MIN_NTP_TIMEOUT_MS, MAX_NTP_TIMEOUT_MS, &timeout_ms)) {
                            invalid_param = true;
                            break;
                        }
                        config.ntp_timeout = (uint32_t)timeout_ms;
                    } else if (strcmp(key, "port") == 0) {
                        long ntp_port = 0;
                        if (!parse_long_bounded(value, 1, 65535, &ntp_port)) {
                            invalid_param = true;
                            break;
                        }
                        config.ntp_port = (uint16_t)ntp_port;
                    } else if (strcmp(key, "led_on") == 0) {
                        config.led_always_on = *value == '1';
                    }
                }
                pair = strtok_r(NULL, "&", &saveptr);
            }
        }

        if (invalid_param) {
            const char *bad_req_msg = "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\nInvalid config fields.";
            tcp_write(pcb, bad_req_msg, (u16_t)strlen(bad_req_msg), 0);
            tcp_output(pcb);
            return ERR_OK;
        }

        if (con_state->store_config(&config, /* invalidate */ 0)) {
            const char *success_msg = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\nSaved! Rebooting...";
            tcp_write(pcb, success_msg, (u16_t)strlen(success_msg), 0);
            server_state.complete = true;
        } else {
            const char *fail_msg = "HTTP/1.1 500 Error\r\nConnection: close\r\n\r\nFailed to save to Flash.";
            tcp_write(pcb, fail_msg, (u16_t)strlen(fail_msg), 0);
        }
    }
    // Route B: Configuration page
    else if (strncmp(request, "GET " HTTP_CONFIG_URL, 4 + sizeof(HTTP_CONFIG_URL) - 1) == 0 ||
             strncmp(request, "GET / ", 6) == 0) {
        size_t html_form_len = strlen(html_form_expanded);
        con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_OK_HEADER);
        if (con_state->header_len <= 0 || con_state->header_len >= (int)sizeof(con_state->headers)) {
            return tcp_close_client_connection(con_state, pcb, ERR_MEM);
        }
        con_state->result_len = (int)html_form_len;
        tcp_write(pcb, con_state->headers, (u16_t)con_state->header_len, TCP_WRITE_FLAG_MORE);
        tcp_write(pcb, html_form_expanded, (u16_t)html_form_len, 0);
    }
    // Route C: Background OS pages / captive portal
    else {
        con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_REDIRECT_HEADER,
                                         ipaddr_ntoa(con_state->gw));
        if (con_state->header_len <= 0 || con_state->header_len >= (int)sizeof(con_state->headers)) {
            return tcp_close_client_connection(con_state, pcb, ERR_MEM);
        }
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

wifi_error_t start_wifi_access_point(struct flash_config_t *flash_config, store_config_handler_t store_config,
                                     bool *wifi_initialized)
{
    CLOCK_DEBUG("Starting access point for configuration\r\n");
    memset(&server_state, 0, sizeof(tcp_server_t));
    server_state.store_config = store_config;
    server_state.flash_config = flash_config;
    server_state.complete = false;

    if (!*wifi_initialized && cyw43_arch_init() != 0) {
        CLOCK_DEBUG("Failed to init Wi-Fi\r\n");
        return WIFI_INIT_ERROR;
    }
    *wifi_initialized = true;

    if (expand_html_template(html_form_template, (char *)html_form_expanded, flash_config) != ERR_OK) {
        CLOCK_DEBUG("Failed to parse HTML form\r\n");
        return WIFI_INIT_ERROR;
    }

    char *ssid = get_ssid();
    cyw43_arch_enable_ap_mode(ssid, NULL, CYW43_AUTH_WPA2_AES_PSK);

    ip4_addr_t mask;
    server_state.gw.addr = PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS);
    mask.addr = PP_HTONL(CYW43_DEFAULT_IP_MASK);

    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &server_state.gw, &mask);

    dns_server_t dns_server;
    dns_server_init(&dns_server, &server_state.gw);

    if (!tcp_server_open()) {
        CLOCK_DEBUG("Failed to open server\n");
        return WIFI_INIT_ERROR;
    }

    while (!server_state.complete) {
        watchdog_update();
        sleep_ms(1000);
    }

    cyw43_arch_disable_ap_mode();
    tcp_server_close();
    dns_server_deinit(&dns_server);
    dhcp_server_deinit(&dhcp_server);

    CLOCK_DEBUG("start_wifi_access_point DONE\r\n");
    return WIFI_OK;
}
