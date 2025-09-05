#include "tests/mock.h"

/* SPI functions */
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

/* GPIO functions */
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

/* PWM functions */
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

/* Wi-Fi functions */
int cyw43_arch_init(void)
{
    return 0;
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
    if (strcmp(ssid, WIFI_SSID) != 0 || strcmp(password, WIFI_PASSWORD) != 0) {
        return PICO_ERROR_BADAUTH;
    }
    return 0;
}

void cyw43_arch_deinit(void)
{
}

/* Utility functions */
void sleep_ms(uint32_t ms)
{
    (void)ms;
}

void stdio_init_all(void)
{
}

/* lwIP functions */
struct pbuf *pbuf_alloc(pbuf_layer l, u16_t length, pbuf_type type)
{
    (void)l;
    (void)length;
    (void)type;
    return NULL;
}

err_t udp_sendto(struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *dst_ip, u16_t dst_port)
{
    (void)pcb;
    (void)p;
    (void)dst_ip;
    (void)dst_port;
    return ERR_OK;
}

u8_t pbuf_free(struct pbuf *p)
{
    (void)p;
    return 0;
}

u16_t pbuf_copy_partial(const struct pbuf *p, void *dataptr, u16_t len, u16_t offset)
{
    (void)p;
    (void)dataptr;
    (void)len;
    (void)offset;
    return 0;
}

u8_t pbuf_get_at(const struct pbuf *p, u16_t offset)
{
    (void)p;
    (void)offset;
    return 0;
}

int dns_gethostbyname(const char *hostname, ip_addr_t *addr,
                      void (*found)(const char *name, const ip_addr_t *ipaddr, void *arg), void *arg)
{
    (void)hostname;
    (void)addr;
    (void)found;
    (void)arg;
    return ERR_OK;
}

void udp_recv(struct udp_pcb *pcb, udp_recv_fn recv, void *recv_arg)
{
    (void)pcb;
    (void)recv;
    (void)recv_arg;
}

struct udp_pcb *udp_new_ip_type(u8_t type)
{
    (void)type;
    return NULL;
}

int ip_addr_cmp(const ip_addr_t *addr1, const ip_addr_t *addr2)
{
    (void)addr1;
    (void)addr2;
    return 0;
}

/* Timer functions */
void powman_timer_start(void)
{
}

void powman_timer_stop(void)
{
}

void powman_timer_set_ms(uint32_t ms)
{
    (void)ms;
}

uint32_t powman_timer_get_ms(void)
{
    return 0;
}

absolute_time_t get_absolute_time(void)
{
    return (absolute_time_t){0};
}

int64_t absolute_time_diff_us(absolute_time_t t1, absolute_time_t t2)
{
    (void)t1;
    (void)t2;
    return 0;
}

int add_repeating_timer_ms(uint32_t ms, bool (*callback)(repeating_timer_t *), void *user_data,
                           repeating_timer_t *out_timer)
{
    (void)ms;
    (void)callback;
    (void)user_data;
    (void)out_timer;
    return 0;
}
