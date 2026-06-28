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

int main(void)
{
    clock_state_t *state = clock_init();
    if (!state)
        return 1;

    // Sets up WiFi, NTP, and the repeating hardware timer
    int status = clock_start(state);

    while (1) {
        clock_task(state);

        if (trigger_ap_mode) {
            trigger_ap_mode = 0;
            start_wifi_access_point(config_store_handler);
        }

        // Yield to allow  handling of 1Hz tick and NTP responses
        sleep_ms(10);
    }

    return 0;
}