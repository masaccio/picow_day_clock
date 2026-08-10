#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "clock.h"
#include "dhcpserver.h"
#include "dnsserver.h"
#include "lcd.h"
#include "mock.h"
#include "test.h"

// LCD functions
lcd_state_t *lcd_init(lcd_state_t *state, uint16_t lcd_num, bool reset)
{
    (void)lcd_num;
    (void)reset;
    state->smoothed_adc = 0;
    return state;
}

void lcd_print_line(lcd_state_t *state, color_t color, lcd_status_message_t msg)
{
    (void)state;
    (void)color;

    lcd_message_event_t event = {.timestamp_ms = mock_ctx.spy.system_time_ms, .color = color, .msg = msg};
    lcd_message_queue_t_push(&mock_ctx.spy.lcd_msg_history, event);
    test_printf("LCD: color=%d, msg=%d\n", (int)color, (int)msg);
}

void lcd_clear_screen(lcd_state_t *state, uint16_t color)
{
    (void)state;
    (void)color;
}

void lcd_print_clock_digit(lcd_state_t *state, color_t color, const char ascii_char)
{
    (void)state;
    (void)color;
    (void)ascii_char;
}

void lcd_update_icons(lcd_state_t *state, watchdog_error_t wd, ntp_error_t ntp, wifi_error_t wifi)
{
    (void)state;

    mock_ctx.spy.watchdog_icon = wd;
    mock_ctx.spy.ntp_icon = ntp;
    mock_ctx.spy.wifi_icon = wifi;

    icon_event_t event = {.timestamp_ms = mock_ctx.spy.system_time_ms, .watchdog = wd, .ntp = ntp, .wifi = wifi};
    icon_queue_t_push(&mock_ctx.spy.icon_history, event);
}

void lcd_set_backlight(lcd_state_t *state, uint8_t level)
{
    (void)state;
    mock_ctx.spy.lcd_brightness = level;
}

// These system calls have been redefined in mock.h so undef them here
// so that we can call into the system libraries as needed.
#undef printf
#undef calloc
#undef free

// GPIO functions
void gpio_init(uint gpio)
{
    (void)gpio;
}

void gpio_set_dir(uint gpio, int out)
{
    (void)gpio;
    (void)out;
}

void gpio_pull_up(uint gpio)
{
    (void)gpio;
}

int gpio_get(uint gpio)
{
    if (gpio == FACTORY_RESET_GPIO)
        // 0 means pressed due to pull-up
        return mock_ctx.inject.factory_reset_pressed == 0;
    return 0;
}

// ADC functions
int adc_init(void)
{
    return ERR_OK;
}

void adc_gpio_init(uint gpio)
{
    (void)gpio;
}
void adc_select_input(uint input)
{
    (void)input;
}

uint16_t adc_read(void)
{
    return mock_ctx.inject.adc_level;
}

// Wi-Fi functions
int cyw43_arch_init(void)
{
    mock_ctx.spy.cyw43_arch_init_fail = mock_ctx.inject.cyw43_arch_init_fail;
    return mock_ctx.inject.cyw43_arch_init_fail ? 1 : 0;
}

void cyw43_arch_enable_sta_mode(void)
{
}

void cyw43_arch_enable_ap_mode(const char *ssid, const char *password, uint32_t auth)
{
    (void)ssid;
    (void)password;
    (void)auth;
}

void cyw43_arch_disable_ap_mode(void)
{
}

void cyw43_arch_disable_sta_mode(void)
{
}

void cyw43_arch_lwip_begin(void)
{
}

void cyw43_arch_lwip_end(void)
{
}

int cyw43_arch_wifi_connect_timeout_ms(const char *ssid, const char *password, uint auth, uint timeout_ms)
{
    (void)ssid;
    (void)password;
    (void)auth;

    if (mock_ctx.inject.cyw43_auth_error_count > 0) {
        mock_ctx.inject.cyw43_auth_error_count--;
        return PICO_ERROR_BADAUTH;
    }

    if (mock_ctx.inject.cyw43_auth_timeout_count > 0) {
        mock_ctx.inject.cyw43_auth_timeout_count--;
        // Use the requested timeout so the simulator accurately fast-forwards
        // the exact amount of time the main thread was meant to be blocked.
        sleep_ms(timeout_ms);

        return PICO_ERROR_TIMEOUT;
    }

    return mock_ctx.inject.cyw43_arch_wifi_connect_status;
}

static uint32_t cyw43_state_storage = 0xffffffff;
void *cyw43_state = &cyw43_state_storage;

int cyw43_wifi_get_mac(void *self, int itf, uint8_t *mac)
{
    (void)self;
    (void)itf;
    static uint8_t test_mac[6] = {0x00, 0x00, 0xde, 0xad, 0xbe, 0xef};
    memcpy(mac, test_mac, sizeof(test_mac));
    return 0;
}

// -----------------------------------------------------------------------------
// Event-Driven Time Pump
// -----------------------------------------------------------------------------
void sleep_ms(uint32_t ms)
{
    time_t target_time = (time_t)mock_ctx.spy.boot_time_ms + ms;

    while (mock_ctx.spy.boot_time_ms < target_time) {
        time_t next_event = target_time;

        // --- 1. Find the closest upcoming chronological event ---
        if (mock_ctx.sim.dns_pending && mock_ctx.sim.dns_fire_time > mock_ctx.spy.boot_time_ms &&
            mock_ctx.sim.dns_fire_time < next_event)
            next_event = mock_ctx.sim.dns_fire_time;

        if (mock_ctx.sim.udp_pending && mock_ctx.sim.udp_fire_time > mock_ctx.spy.boot_time_ms &&
            mock_ctx.sim.udp_fire_time < next_event)
            next_event = mock_ctx.sim.udp_fire_time;

        if (mock_ctx.sim.tcp_accept_cb && mock_ctx.sim.tcp_payload_idx < mock_ctx.sim.tcp_payload_count &&
            mock_ctx.sim.tcp_next_fire_time < mock_ctx.spy.boot_time_ms && mock_ctx.sim.tcp_next_fire_time < next_event)
            next_event = mock_ctx.sim.tcp_next_fire_time;

        if (mock_ctx.sim.tcp_poll_cb && mock_ctx.sim.tcp_poll_interval > 0 &&
            mock_ctx.sim.tcp_next_poll_time > mock_ctx.spy.boot_time_ms && mock_ctx.sim.tcp_next_poll_time < next_event)
            next_event = mock_ctx.sim.tcp_next_poll_time;

        if (mock_ctx.sim.tcp_sent_pending && mock_ctx.sim.tcp_sent_fire_time > mock_ctx.spy.boot_time_ms &&
            mock_ctx.sim.tcp_sent_fire_time < next_event)
            next_event = mock_ctx.sim.tcp_sent_fire_time;

        if (mock_ctx.sim.tcp_err_pending && mock_ctx.sim.tcp_err_fire_time > mock_ctx.spy.boot_time_ms &&
            mock_ctx.sim.tcp_err_fire_time < next_event)
            next_event = mock_ctx.sim.tcp_err_fire_time;

        if (mock_ctx.sim.clock_timer_active && mock_ctx.sim.clock_timer_next_fire > mock_ctx.spy.boot_time_ms &&
            mock_ctx.sim.clock_timer_next_fire < next_event)
            next_event = mock_ctx.sim.clock_timer_next_fire;

        if (mock_ctx.sim.backlight_timer_active && mock_ctx.sim.backlight_timer_next_fire > mock_ctx.spy.boot_time_ms &&
            mock_ctx.sim.backlight_timer_next_fire < next_event)
            next_event = mock_ctx.sim.backlight_timer_next_fire;

        // Fast-forward system time to the event
        time_t delta = next_event - mock_ctx.spy.boot_time_ms;
        mock_ctx.spy.system_time_ms += delta;
        mock_ctx.spy.boot_time_ms += delta;
        mock_ctx.spy.watchdog_time_ms += delta;

        // 1. Process Hardware Timers
        if (mock_ctx.sim.clock_timer_active && mock_ctx.spy.boot_time_ms == mock_ctx.sim.clock_timer_next_fire) {
            mock_ctx.sim.clock_timer_next_fire += mock_ctx.sim.clock_timer_delay;
            if (!mock_ctx.sim.clock_timer_cb(mock_ctx.sim.clock_timer_arg)) {
                mock_ctx.sim.clock_timer_active = false;
            }
        }
        if (mock_ctx.sim.backlight_timer_active &&
            mock_ctx.spy.boot_time_ms == mock_ctx.sim.backlight_timer_next_fire) {
            mock_ctx.sim.backlight_timer_next_fire += mock_ctx.sim.backlight_timer_delay;
            if (!mock_ctx.sim.backlight_timer_cb(mock_ctx.sim.backlight_timer_arg)) {
                mock_ctx.sim.backlight_timer_active = false;
            }
        }

        // 2. Process DNS Lookups
        if (mock_ctx.sim.dns_pending && mock_ctx.spy.boot_time_ms == mock_ctx.sim.dns_fire_time) {
            mock_ctx.sim.dns_pending = false;
            if (mock_ctx.sim.dns_cb) {
                ip_addr_t addr = {.addr = 0x08080808}; // 8.8.8.8
                err_t status = mock_ctx.inject.dns_lookup_fail ? ERR_ABRT : ERR_OK;
                if (status != ERR_OK)
                    addr.addr = 0; // Clear on fail
                mock_ctx.sim.dns_cb(mock_ctx.sim.dns_hostname, &addr, mock_ctx.sim.dns_arg);
            }
        }

        // 3. Process UDP/NTP Responses
        if (mock_ctx.sim.udp_pending && mock_ctx.spy.boot_time_ms == mock_ctx.sim.udp_fire_time) {
            mock_ctx.sim.udp_pending = false;
            if (mock_ctx.inject.udp_response_type != UDP_TIMEOUT) {
                if (mock_ctx.sim.udp_recv_cb) {
                    struct pbuf p;
                    memset(&p, 0, sizeof(p));
                    if (mock_ctx.inject.udp_response_type == UDP_NTP_BAD_LEN) {
                        p.tot_len = 0x0;
                        p.len = 0x0;
                    } else {
                        p.tot_len = NTP_MSG_LEN;
                        p.len = NTP_MSG_LEN;
                    }
                    uint8_t *req = (uint8_t *)p.payload;

                    if (mock_ctx.inject.udp_response_type == UDP_NTP_KOD) {
                        // 0xdc = Leap Indicator 3 (11), Version 3 (011), Mode 4 (100)
                        req[0] = 0xdc;
                        req[1] = 0;
                        memcpy(&req[12], "RATE", 4);
                    } else if (mock_ctx.inject.udp_response_type == UDP_NTP_LEAP3) {
                        req[0] = (0x3 << 6);
                        req[1] = 2;
                    } else {
                        if (mock_ctx.inject.udp_response_type == UDP_NTP_INVALID) {
                            req[0] = 0x0;
                        } else {
                            req[0] = 0x1c;
                        }
                        req[1] = 2; // Stratum 2
                        uint32_t sec_net = (uint32_t)mock_ctx.spy.ntp_seconds;
                        req[40] = (sec_net >> 24) & 0xFF;
                        req[41] = (sec_net >> 16) & 0xFF;
                        req[42] = (sec_net >> 8) & 0xFF;
                        req[43] = sec_net & 0xFF;
                    }

                    ip_addr_t src_addr = {.addr = 0x08080808}; // 8.8.8.8
                    u16_t port =
                        (mock_ctx.inject.udp_response_type == UDP_NTP_BAD_PORT) ? 0x0 : mock_ctx.config.udp_port;
                    if (mock_ctx.inject.udp_invalid_addr)
                        src_addr.addr = 0xffffffff;
                    mock_ctx.sim.udp_recv_cb(mock_ctx.sim.udp_recv_arg, mock_ctx.sim.udp_pcb, &p, &src_addr, port);
                    mock_ctx.spy.ntp_packet_sent = 1;
                }
            }
        }

        // 4. Process TCP/Captive Portal Requests
        if (mock_ctx.sim.tcp_accept_cb && mock_ctx.sim.tcp_payload_idx < mock_ctx.sim.tcp_payload_count &&
            mock_ctx.spy.boot_time_ms == mock_ctx.sim.tcp_next_fire_time) {

            // If the server left the previous connection open, simulate the client closing
            // the socket BEFORE we reuse the PCB for the next request.
            if (mock_ctx.sim.conn_arg != NULL && mock_ctx.sim.tcp_recv_cb) {
                // Sending pbuf = NULL is the lwIP standard for "Client Disconnected"
                mock_ctx.sim.tcp_recv_cb(mock_ctx.sim.conn_arg, &mock_ctx.sim.conn_pcb, NULL, ERR_OK);
            }

            err_t accept_err = mock_ctx.sim.tcp_accept_cb(mock_ctx.sim.listen_arg, &mock_ctx.sim.conn_pcb, ERR_OK);

            if (accept_err == ERR_OK && mock_ctx.sim.tcp_recv_cb) {
                struct pbuf p;
                memset(&p, 0, sizeof(p));

                strncpy((char *)p.payload, mock_ctx.sim.tcp_payloads[mock_ctx.sim.tcp_payload_idx],
                        sizeof(p.payload) - 1);
                p.tot_len = (u16_t)strlen((char *)p.payload);
                p.len = p.tot_len;

                // Deliver the payload to the CONN arg
                mock_ctx.sim.tcp_recv_cb(mock_ctx.sim.conn_arg, &mock_ctx.sim.conn_pcb, &p, ERR_OK);
            }

            // Advance the context index
            mock_ctx.sim.tcp_payload_idx++;
            mock_ctx.sim.tcp_next_fire_time = mock_ctx.spy.boot_time_ms + 100;
        }

        // 5. Process TCP Sent (Ensures the server cleanly frees con_state)
        if (mock_ctx.sim.tcp_sent_pending && mock_ctx.spy.boot_time_ms == mock_ctx.sim.tcp_sent_fire_time) {
            mock_ctx.sim.tcp_sent_pending = false;
            if (mock_ctx.sim.tcp_sent_cb) {
                mock_ctx.sim.tcp_sent_cb(mock_ctx.sim.conn_arg, &mock_ctx.sim.conn_pcb, mock_ctx.sim.tcp_sent_len);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Network Stubs (State Only)
// -----------------------------------------------------------------------------
err_t dns_gethostbyname(const char *hostname, ip_addr_t *addr,
                        void (*found)(const char *name, const ip_addr_t *ipaddr, void *callback_arg),
                        void *callback_arg)
{
    (void)addr;
    if (mock_ctx.inject.dns_bad_arg)
        return ERR_ARG;

    mock_ctx.sim.dns_pending = true;
    uint32_t latency = mock_ctx.inject.dns_latency_ms ? mock_ctx.inject.dns_latency_ms : 50;
    mock_ctx.sim.dns_fire_time = mock_ctx.spy.boot_time_ms + latency;
    mock_ctx.sim.dns_cb = found;
    mock_ctx.sim.dns_arg = callback_arg;
    strncpy(mock_ctx.sim.dns_hostname, hostname, 255);
    return ERR_INPROGRESS;
}

void udp_recv(struct udp_pcb *pcb, udp_recv_fn recv, void *recv_arg)
{
    mock_ctx.sim.udp_pcb = pcb;
    mock_ctx.sim.udp_recv_cb = recv;
    mock_ctx.sim.udp_recv_arg = recv_arg;
}

struct udp_pcb *udp_new(void)
{
    return udp_new_ip_type(IPADDR_TYPE_ANY);
}

void udp_remove(struct udp_pcb *pcb)
{
    (void)pcb;
}

err_t udp_bind(struct udp_pcb *pcb, const ip_addr_t *ipaddr, u16_t port)
{
    (void)pcb;
    (void)ipaddr;
    (void)port;
    return ERR_OK;
}

err_t udp_sendto(struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *dst_ip, u16_t dst_port)
{
    (void)pcb;
    (void)dst_ip;
    (void)dst_port;
    mock_ctx.spy.udp_sendto_calls++;
    mock_ctx.spy.udp_last_send_len = p ? p->tot_len : 0;
    if (mock_ctx.inject.udp_sendto_fail) {
        return -1;
    }
    mock_ctx.sim.udp_pending = true;
    uint32_t latency = mock_ctx.inject.udp_latency_ms ? mock_ctx.inject.udp_latency_ms : 50;
    mock_ctx.sim.udp_fire_time = mock_ctx.spy.boot_time_ms + latency;
    return ERR_OK;
}

err_t udp_sendto_if(struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *dst_ip, u16_t dst_port, struct netif *nif)
{
    (void)nif;
    return udp_sendto(pcb, p, dst_ip, dst_port);
}

struct netif *ip_current_input_netif(void)
{
    static struct netif nif = {0};
    return &nif;
}

struct tcp_pcb *tcp_new(void)
{
    return &mock_ctx.sim.listen_pcb;
}
void tcp_arg(struct tcp_pcb *pcb, void *arg)
{
    if (pcb == &mock_ctx.sim.listen_pcb) {
        mock_ctx.sim.listen_arg = arg;
    } else {
        mock_ctx.sim.conn_arg = arg;
    }
}

void tcp_accept(struct tcp_pcb *pcb, tcp_accept_fn accept)
{
    (void)pcb;
    mock_ctx.sim.tcp_accept_cb = accept;
}
void tcp_recv(struct tcp_pcb *pcb, tcp_recv_fn recv)
{
    (void)pcb;
    mock_ctx.sim.tcp_recv_cb = recv;
}
void tcp_err(struct tcp_pcb *pcb, tcp_err_fn err)
{
    (void)pcb;
    mock_ctx.sim.tcp_err_cb = err;
}

// Allow your tests to simulate a sudden "Connection Reset by Peer" (ERR_RST)
void mock_inject_tcp_error(err_t code, uint32_t delay_ms)
{
    if (mock_ctx.sim.tcp_err_cb) {
        mock_ctx.sim.tcp_err_pending = true;
        mock_ctx.sim.tcp_err_code = code;
        mock_ctx.sim.tcp_err_fire_time = (time_t)mock_ctx.spy.boot_time_ms + delay_ms;
    }
}
void tcp_poll(struct tcp_pcb *pcb, tcp_poll_fn poll, u8_t interval)
{
    (void)pcb;
    mock_ctx.sim.tcp_poll_cb = poll;
    mock_ctx.sim.tcp_poll_interval = interval;
    if (poll && interval > 0) {
        // lwIP poll intervals are specified in 500ms ticks
        mock_ctx.sim.tcp_next_poll_time = mock_ctx.spy.boot_time_ms + (interval * 500);
    }
}

void tcp_sent(struct tcp_pcb *pcb, tcp_sent_fn sent)
{
    (void)pcb;
    mock_ctx.sim.tcp_sent_cb = sent;
}

// UPDATE your existing tcp_write to trigger the sent callback
err_t tcp_write(struct tcp_pcb *pcb, const void *dataptr, u16_t len, u8_t apiflags)
{
    (void)pcb;
    (void)apiflags;
    memcpy(mock_ctx.sim.tcp_write_buffer, dataptr, len);

    // Simulate the network sending the packet and the client acknowledging it.
    // We queue it to fire 10ms in the future.
    if (mock_ctx.sim.tcp_sent_cb) {
        mock_ctx.sim.tcp_sent_pending = true;
        mock_ctx.sim.tcp_sent_len = len;
        mock_ctx.sim.tcp_sent_fire_time = mock_ctx.spy.boot_time_ms + 10;
    }
    return ERR_OK;
}

err_t tcp_recved(struct tcp_pcb *pcb, u16_t len)
{
    (void)pcb;
    (void)len;
    return ERR_OK;
}
struct tcp_pcb *tcp_new_ip_type(u8_t type)
{
    (void)type;
    if (mock_ctx.inject.tcp_open_fail) {
        return NULL;
    }
    return tcp_new(); // Just return the standard mock PCB instance
}

err_t tcp_bind(struct tcp_pcb *pcb, const ip_addr_t *ipaddr, u16_t port)
{
    (void)pcb;
    (void)ipaddr;
    (void)port;
    if (mock_ctx.inject.tcp_bind_fail) {
        return ERR_VAL;
    }
    return ERR_OK;
}

struct tcp_pcb *tcp_listen_with_backlog(struct tcp_pcb *pcb, u8_t backlog)
{
    (void)pcb;
    (void)backlog;
    if (mock_ctx.inject.tcp_listen_fail) {
        return NULL;
    }
    return &mock_ctx.sim.listen_pcb;
}
err_t tcp_close(struct tcp_pcb *pcb)
{
    (void)pcb;
    if (mock_ctx.inject.tcp_close_fail) {
        return ERR_MEM;
    }
    return ERR_OK;
}

void tcp_abort(struct tcp_pcb *pcb)
{
    (void)pcb;
}

void mock_queue_tcp_payload(const char *body)
{
    if (mock_ctx.sim.tcp_payload_count < 10)
        snprintf(mock_ctx.sim.tcp_payloads[mock_ctx.sim.tcp_payload_count++], 2048, "%s", body);
}

void mock_clear_tcp_payloads(void)
{
    mock_ctx.sim.tcp_payload_count = 0;
    mock_ctx.sim.tcp_payload_idx = 0;
    memset(mock_ctx.sim.tcp_write_buffer, 0, TCP_IP_BUFFER_SIZE);
}

// -----------------------------------------------------------------------------
// Hardware Timer Stubs
// -----------------------------------------------------------------------------
bool add_repeating_timer_ms(uint32_t delay_ms, bool (*callback)(repeating_timer_t *rt), void *user_data,
                            repeating_timer_t *out)
{
    // Scales very badly to multiple timers
    if (callback == clock_timer_callback) {
        mock_ctx.sim.clock_timer_active = true;
        mock_ctx.sim.clock_timer_delay = delay_ms;
        mock_ctx.sim.clock_timer_cb = callback;
        mock_ctx.sim.clock_timer_arg = out;
        mock_ctx.sim.clock_timer_next_fire = mock_ctx.spy.boot_time_ms + delay_ms;
    } else {
        mock_ctx.sim.backlight_timer_active = true;
        mock_ctx.sim.backlight_timer_delay = delay_ms;
        mock_ctx.sim.backlight_timer_cb = callback;
        mock_ctx.sim.backlight_timer_arg = out;
        mock_ctx.sim.backlight_timer_next_fire = mock_ctx.spy.boot_time_ms + delay_ms;
    }
    out->user_data = user_data;
    return true;
}

absolute_time_t get_absolute_time(void)
{
    return (absolute_time_t)(mock_ctx.spy.system_time_ms * 1000);
}

int64_t absolute_time_diff_us(absolute_time_t from, absolute_time_t to)
{
    return (int64_t)to - (int64_t)from;
}

uint32_t to_ms_since_boot(absolute_time_t t)
{
    return (uint32_t)(t / 1000ULL);
}

time_t mock_time(time_t *tloc)
{
    (void)tloc; // Always called with NULL
    return (time_t)(mock_ctx.spy.system_time_ms / 1000);
}

int mock_settimeofday(const struct timeval *tp, void *tzp)
{
    (void)tzp;
    mock_ctx.spy.system_time_ms = (time_t)(tp->tv_sec * 1000 + (tp->tv_usec * 1000));
    return 0;
}

// Utility functions
void stdio_init_all(void)
{
}

int test_printf(const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    (void)vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (mock_ctx.config.test_verbose)
        printf("DEBUG: %s", buffer);
    return 0;
}

// lwIP functions
struct pbuf *pbuf_alloc(pbuf_layer l, u16_t length, pbuf_type type)
{
    (void)l;
    (void)length;
    (void)type;

    if (mock_ctx.inject.pbuf_alloc_fail_at != 0) {
        mock_ctx.spy.pbuf_alloc_counter++;
        if (mock_ctx.spy.pbuf_alloc_counter >= mock_ctx.inject.pbuf_alloc_fail_at) {
            return NULL;
        }
    }

    static struct pbuf p;
    return &p;
}

u8_t pbuf_free(struct pbuf *p)
{
    (void)p;
    return 1;
}

u16_t pbuf_copy_partial(const struct pbuf *p, void *dataptr, u16_t len, u16_t offset)
{
    memcpy(dataptr, p->payload + offset, len);
    return len;
}

u8_t pbuf_get_at(const struct pbuf *p, u16_t offset)
{
    return p->payload[offset];
}

struct udp_pcb *udp_new_ip_type(u8_t type)
{
    (void)type;
    if (mock_ctx.inject.udp_new_ip_type_fail) {
        return NULL;
    }
    static struct udp_pcb buffer = {0xdeadbeef};
    return &buffer;
}

int ip_addr_cmp(const ip_addr_t *addr1, const ip_addr_t *addr2)
{
    return addr1->addr == addr2->addr;
}

// Watchdog
void watchdog_update(void)
{
    mock_ctx.spy.watchdog_time_ms = 0;
}

int watchdog_caused_reboot(void)
{
    return mock_ctx.inject.watchdog_caused_reboot || mock_ctx.spy.watchdog_reboot_called;
}

void watchdog_reboot(uint32_t pc, uint32_t sp, uint32_t delay_ms)
{
    (void)pc;
    (void)sp;
    (void)delay_ms;
    mock_ctx.spy.watchdog_reboot_called = 1;
    mock_ctx.inject.factory_reset_pressed = 0;

    if (!mock_ctx.inject.fatal_reset_no_longjmp)
        longjmp(fatal_reset_jmp_buf, 1);
}

void watchdog_enable(uint32_t delay_ms, int pause_on_debug)
{
    (void)delay_ms;
    (void)pause_on_debug;
}

// Flash
static uint32_t interrupt_status;

uint32_t save_and_disable_interrupts(void)
{
    return interrupt_status;
}

void restore_interrupts(uint32_t status)
{
    interrupt_status = status;
}

static flash_config_t flash_clock_config_storage = {.magic_marker = CONFIG_MAGIC, .ntp_port = 8123};

void *mock_flash_config = (void *)&flash_clock_config_storage;

static mock_struct_t mock_tcp_server_state_storage;
void *mock_tcp_server_state = (void *)&mock_tcp_server_state_storage;

void flash_range_erase(uint32_t flash_offs, size_t count)
{
    (void)flash_offs;
    (void)count;
    memset(mock_flash_config, 0, sizeof(flash_config_t));
}

void flash_range_program(uint32_t flash_offs, const uint8_t *data, size_t count)
{
    (void)flash_offs;
    (void)count;
    memcpy(mock_flash_config, data, sizeof(flash_config_t));
}

err_t tcp_output(struct tcp_pcb *pcb)
{
    (void)pcb;
    return 0;
}

const char *ip4addr_ntoa(const ip_addr_t *ipaddr)
{
    (void)ipaddr;
    return "192.168.4.1";
}

void dhcp_server_deinit(dhcp_server_t *d)
{
    (void)d;
}

void dhcp_server_init(dhcp_server_t *d, ip_addr_t *ip, ip_addr_t *nm)
{
    (void)d;
    (void)ip;
    (void)nm;
}

void dns_server_deinit(dns_server_t *d)
{
    (void)d;
}

void dns_server_init(dns_server_t *d, ip_addr_t *ip)
{
    (void)d;
    (void)ip;
}
