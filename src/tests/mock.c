#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "mock.h"
#include "test.h"

// These system calls have been redefined in mock.h so undef them here
// so that we can call into the system libraries as needed.
#undef printf
#undef calloc

extern unsigned int log_buffer_size;
extern char **log_buffer;

// SPI functions
uint spi_init(spi_inst_t *spi, int baudrate)
{
    (void)spi;
    (void)baudrate;
    return (uint)0;
}

int spi_write_blocking(spi_inst_t *spi, const uint8_t *src, size_t len)
{
    (void)spi;
    (void)src;
    (void)len;
    return 0;
}

// GPIO functions
void gpio_set_function(uint gpio, uint func)
{
    (void)gpio;
    (void)func;
}

void gpio_init(uint gpio)
{
    (void)gpio;
}

void gpio_set_dir(uint gpio, bool out)
{
    (void)gpio;
    (void)out;
}

void gpio_put(uint gpio, int value)
{
    (void)gpio;
    (void)value;
}

// PWM functions
int pwm_gpio_to_slice_num(uint gpio)
{
    (void)gpio;
    return 0;
}

void pwm_set_wrap(uint slice_num, uint32_t wrap)
{
    (void)slice_num;
    (void)wrap;
}

void pwm_set_chan_level(uint slice_num, uint chan, uint32_t level)
{
    (void)slice_num;
    (void)chan;
    (void)level;
}

void pwm_set_clkdiv(uint slice_num, float divider)
{
    (void)slice_num;
    (void)divider;
}

void pwm_set_enabled(uint slice_num, bool enabled)
{
    (void)slice_num;
    (void)enabled;
}

// Wi-Fi functions
int cyw43_arch_init(void)
{
    return test_config.cyw43_arch_init_fail ? 1 : 0;
}

void cyw43_arch_lwip_begin(void)
{
}

void cyw43_arch_lwip_end(void)
{
}

void cyw43_arch_enable_sta_mode(void)
{
}

int cyw43_arch_wifi_connect_timeout_ms(const char *ssid, const char *password, uint auth, uint timeout_ms)
{
    (void)ssid;
    (void)password;
    (void)auth;
    (void)timeout_ms;
    if (test_config.cyw43_auth_error_count > 0) {
        test_config.cyw43_auth_error_count--;
        return PICO_ERROR_BADAUTH;
    }
    if (test_config.cyw43_auth_timeout_count > 0) {
        test_config.cyw43_auth_timeout_count--;
        sleep_ms(10 * 1000);
        return PICO_ERROR_TIMEOUT;
    }
    return test_config.cyw43_arch_wifi_connect_status;
}

void cyw43_arch_deinit(void)
{
}

// Utility functions
void stdio_init_all(void)
{
}

// lwIP functions
static udp_recv_fn udp_recv_callback;
static void *udp_recv_callback_arg;
unsigned mock_ntp_seconds = 0;

struct pbuf *pbuf_alloc(pbuf_layer l, u16_t length, pbuf_type type)
{
    (void)l;
    (void)length;
    (void)type;
    static struct pbuf p;
    return &p;
}

err_t udp_sendto(struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *dst_ip, u16_t dst_port)
{
    (void)pcb;
    (void)p;
    (void)dst_ip;
    (void)dst_port;
    // Calling the recv() callback right away is harmless for this implementation.
    // And we hard-code a hacked up NTP packet as that's all that will ever be used
    p->tot_len = NTP_MSG_LEN;
    p->payload[0] = 0x4; // mode
    p->payload[1] = 0x1; // stratum
    p->payload[40] = mock_ntp_seconds >> 24;
    p->payload[41] = (mock_ntp_seconds >> 16) & 0xff;
    p->payload[42] = (mock_ntp_seconds >> 8) & 0xff;
    p->payload[43] = mock_ntp_seconds & 0xff;
    udp_recv_callback(udp_recv_callback_arg, pcb, p, dst_ip, NTP_PORT);
    return ERR_OK;
}

u8_t pbuf_free(struct pbuf *p)
{
    (void)p;
    return 1;
}

u16_t pbuf_copy_partial(const struct pbuf *p, void *dataptr, u16_t len, u16_t offset)
{
    memcpy(dataptr, p->payload + offset, len);
    return 0;
}

u8_t pbuf_get_at(const struct pbuf *p, u16_t offset)
{
    return p->payload[offset];
}

int dns_gethostbyname(const char *hostname, ip_addr_t *addr,
                      void (*found)(const char *name, const ip_addr_t *ipaddr, void *arg), void *arg)
{
    static const ip_addr_t ipaddr = {0xdeadbeef};
    if (test_config.dns_lookup_fail) {
        addr->addr = 0;
        found(hostname, NULL, arg);

    } else {
        found(hostname, &ipaddr, arg);
    }
    return ERR_OK;
}
void udp_recv(struct udp_pcb *pcb, udp_recv_fn recv, void *recv_arg)
{
    (void)pcb;
    udp_recv_callback = recv;
    udp_recv_callback_arg = recv_arg;
}

struct udp_pcb *udp_new_ip_type(u8_t type)
{
    (void)type;
    if (test_config.udp_new_ip_type_fail) {
        return NULL;
    }
    static struct udp_pcb buffer = {0xdeadbeef};
    return &buffer;
}

int ip_addr_cmp(const ip_addr_t *addr1, const ip_addr_t *addr2)
{
    return addr1->addr == addr2->addr;
}

unsigned long long mock_system_time_ms = 0;
unsigned long long mock_boot_time_ms = 0;

// Timer functions
void sleep_ms(uint32_t ms)
{
    mock_system_time_ms += ms;
    mock_boot_time_ms += ms;
}

absolute_time_t get_absolute_time(void)
{
    return (absolute_time_t)(mock_boot_time_ms * 1000);
}

int64_t absolute_time_diff_us(absolute_time_t from, absolute_time_t to)
{
    return to - from;
}

int add_repeating_timer_ms(uint32_t ms, bool (*callback)(repeating_timer_t *), void *user_data,
                           repeating_timer_t *out_timer)
{
    (void)ms;
    (void)callback;
    out_timer->user_data = user_data;
    return 0;
}

int mock_printf(const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    int buffer_len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (log_buffer_size >= LOG_BUFFER_SIZE) {
        printf("*** MOCK BUFFER OVERFLOW!\n");
        return 1;
    } else {
        log_buffer[log_buffer_size] = calloc(1, buffer_len + 1);
        strncpy(log_buffer[log_buffer_size], buffer, buffer_len);
        log_buffer_size += 1;
        return 0;
    }
}

int test_printf(const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    (void)vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    printf("DEBUG: %s", buffer);
    return 0;
}

extern unsigned int calloc_fail_at;

void *mock_calloc(size_t num, size_t size)
{
    static unsigned int calloc_counter = 0;
    if (calloc_fail_at != 0) {
        calloc_counter++;
        if (calloc_counter > calloc_fail_at) {
            return NULL;
        }
    }
    return calloc(num, size);
}

time_t mock_time(time_t *tloc)
{
    (void)tloc; // Always called with NULL
    return (time_t)(mock_system_time_ms / 1000);
}

int mock_settimeofday(const struct timeval *tp, const struct timezone *tzp)
{
    (void)tzp; // Implementation has ignores timezones
    mock_system_time_ms = tp->tv_sec * 1000 + (tp->tv_usec * 1000);
    return 0;
}
