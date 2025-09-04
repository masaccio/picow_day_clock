// Hardware Abstraction Layer header for Pico SDK
#ifndef HAL_H
#define HAL_H

#include <stdbool.h>
#include <stdint.h>

#include "pico/stdlib.h"

// Forward declaration of HAL struct
struct hal_t;
typedef struct hal_t hal_t;

struct hal_t
{
    uint (*spi_init)(uint baudrate);
    int (*spi_write_blocking)(const uint8_t *src, size_t len);
    void (*gpio_set_function)(uint gpio, uint func);
    void (*gpio_init)(uint gpio);
    void (*gpio_set_dir)(uint gpio, bool out);
    void (*gpio_put)(uint gpio, int value);
    int (*pwm_gpio_to_slice_num)(uint gpio);
    void (*pwm_set_wrap)(uint slice_num, uint32_t wrap);
    void (*pwm_set_chan_level)(uint slice_num, uint chan, uint32_t level);
    void (*pwm_set_clkdiv)(uint slice_num, float divider);
    void (*pwm_set_enabled)(uint slice_num, bool enabled);
    void (*sleep_ms)(uint32_t ms);
    // Add more Pico SDK functions as needed
};

extern hal_t pico_hal;

#endif // HAL_H
