#include "hardware/sync.h"
#include "hardware/watchdog.h"

#include "clock.h"

persistent_state_t persistent_state __attribute__((section(".uninitialized_data")));

void __attribute__((noreturn)) fatal_reset(clock_state_t *state, ntp_error_t ntp_error, wifi_error_t wifi_error)
{
    lcd_update_icons(state->lcd_states[0], WATCHDOG_RESET, ntp_error, wifi_error);
    persistent_state.ntp_error = ntp_error;
    persistent_state.wifi_error = wifi_error;
    watchdog_reboot((uint32_t)0, SRAM_END, (uint32_t)0 /* delay_ms */);
    while (1)
        __wfi(); // hang until reset
}

void on_clock_alloc_failed(void)
{
    // TODO: blink an LED to indicate startup has failed
    // while (1) {
    //     gpio_xor_mask(1 << LED_PIN);
    //     sleep_ms(100);
    // }
}

void on_lcd_init_failed(clock_state_t *state, unsigned lcd_num)
{
    (void)state;
    (void)lcd_num;
    // TODO: blink an LED to indicate startup has failed
    // while (1) {
    //     gpio_xor_mask(1 << LED_PIN);
    //     sleep_ms(100);
    // }
}

// static volatile uint32_t press_start_time = 0;
// volatile int factory_reset = 0;

// void gpio_callback(uint gpio, uint32_t events)
// {
//     if (gpio == CONFIG_BUTTON_GPIO) {
//         if (events & GPIO_IRQ_EDGE_FALL) {
//             press_start_time = to_ms_since_boot(get_absolute_time());
//         } else if (events & GPIO_IRQ_EDGE_RISE) {
//             uint32_t press_end_time = to_ms_since_boot(get_absolute_time());
//             if (press_start_time != 0 && (press_end_time - press_start_time) >= 5000) {
//                 factory_reset = true;
//             }
//             press_start_time = 0;
//         }
//     }
// }

int main(void)
{
    // gpio_set_irq_enabled_with_callback(CONFIG_BUTTON_GPIO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true,
    //                                    &gpio_callback);

    clock_state_t *state = clock_init();
    if (!state)
        return 1;

    int status = clock_main_loop(state);

    while (1) {
        if (trigger_ap_mode) {
            trigger_ap_mode = 0;
            printf("Starting access point...\n");
            start_wifi_access_point(config_store_handler);
        }
        sleep_ms(1000);
    }

    return 0;
}