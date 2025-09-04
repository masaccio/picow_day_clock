// Hardware Abstraction Layer for Pico SDK
#include "hal.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

static uint hal_spi_init(uint baudrate)
{
    return spi_init(spi1, baudrate);
}

static int hal_spi_write_blocking(const uint8_t *src, size_t len)
{
    return spi_write_blocking(spi1, src, len);
}

static void hal_gpio_set_function(uint gpio, uint func)
{
    gpio_set_function(gpio, func);
}

static void hal_gpio_init(uint gpio)
{
    gpio_init(gpio);
}

static void hal_gpio_set_dir(uint gpio, bool out)
{
    gpio_set_dir(gpio, out);
}

static void hal_gpio_put(uint gpio, int value)
{
    gpio_put(gpio, value);
}

static int hal_pwm_gpio_to_slice_num(uint gpio)
{
    return pwm_gpio_to_slice_num(gpio);
}

static void hal_pwm_set_wrap(uint slice_num, uint32_t wrap)
{
    pwm_set_wrap(slice_num, wrap);
}

static void hal_pwm_set_chan_level(uint slice_num, uint chan, uint32_t level)
{
    pwm_set_chan_level(slice_num, chan, level);
}

static void hal_pwm_set_clkdiv(uint slice_num, float divider)
{
    pwm_set_clkdiv(slice_num, divider);
}

static void hal_pwm_set_enabled(uint slice_num, bool enabled)
{
    pwm_set_enabled(slice_num, enabled);
}

static void hal_sleep_ms(uint32_t ms)
{
    sleep_ms(ms);
}

static int hal_cyw43_arch_init(void)
{
    return cyw43_arch_init();
}

static void hal_cyw43_arch_enable_sta_mode(void)
{
    cyw43_arch_enable_sta_mode();
}

static int hal_cyw43_arch_wifi_connect_timeout_ms(const char *ssid, const char *password, uint auth, uint timeout_ms)
{
    return cyw43_arch_wifi_connect_timeout_ms(ssid, password, auth, timeout_ms);
}

static void hal_cyw43_arch_deinit(void)
{
    cyw43_arch_deinit();
}

// HAL instance
static hal_t _pico_hal = {
    .spi_init = hal_spi_init,
    .spi_write_blocking = hal_spi_write_blocking,
    .gpio_set_function = hal_gpio_set_function,
    .gpio_init = hal_gpio_init,
    .gpio_set_dir = hal_gpio_set_dir,
    .gpio_put = hal_gpio_put,
    .pwm_gpio_to_slice_num = hal_pwm_gpio_to_slice_num,
    .pwm_set_wrap = hal_pwm_set_wrap,
    .pwm_set_chan_level = hal_pwm_set_chan_level,
    .pwm_set_clkdiv = hal_pwm_set_clkdiv,
    .pwm_set_enabled = hal_pwm_set_enabled,
    .sleep_ms = hal_sleep_ms,
    .cyw43_arch_init = hal_cyw43_arch_init,
    .cyw43_arch_enable_sta_mode = hal_cyw43_arch_enable_sta_mode,
    .cyw43_arch_wifi_connect_timeout_ms = hal_cyw43_arch_wifi_connect_timeout_ms,
    .cyw43_arch_deinit = hal_cyw43_arch_deinit,
};

hal_t *pico_hal = &_pico_hal;