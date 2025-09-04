// Hardware Abstraction Layer for Pico SDK
#include "hal.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"

uint hal_spi_init(uint baudrate)
{
    return spi_init(spi1, baudrate);
}

int hal_spi_write_blocking(const uint8_t *src, size_t len)
{
    return spi_write_blocking(spi1, src, len);
}
void hal_gpio_set_function(uint gpio, uint func)
{
    gpio_set_function(gpio, func);
}
void hal_gpio_init(uint gpio)
{
    gpio_init(gpio);
}
void hal_gpio_set_dir(uint gpio, bool out)
{
    gpio_set_dir(gpio, out);
}
void hal_gpio_put(uint gpio, int value)
{
    gpio_put(gpio, value);
}
int hal_pwm_gpio_to_slice_num(uint gpio)
{
    return pwm_gpio_to_slice_num(gpio);
}
void hal_pwm_set_wrap(uint slice_num, uint32_t wrap)
{
    pwm_set_wrap(slice_num, wrap);
}
void hal_pwm_set_chan_level(uint slice_num, uint chan, uint32_t level)
{
    pwm_set_chan_level(slice_num, chan, level);
}
void hal_pwm_set_clkdiv(uint slice_num, float divider)
{
    pwm_set_clkdiv(slice_num, divider);
}
void hal_pwm_set_enabled(uint slice_num, bool enabled)
{
    pwm_set_enabled(slice_num, enabled);
}
void hal_sleep_ms(uint32_t ms)
{
    sleep_ms(ms);
}

// HAL instance
hal_t pico_hal = {
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
};
