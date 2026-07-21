#include "hardware/sync.h"
#include "hardware/watchdog.h"

#include "clock.h"

persistent_state_t persistent_state __attribute__((section(".uninitialized_data")));

void __attribute__((noreturn)) fatal_reset(clock_state_t *state, ntp_error_t ntp_error, wifi_error_t wifi_error)
{
    lcd_update_icons(&state->lcd_states[0], WATCHDOG_RESET, ntp_error, wifi_error);
    persistent_state.ntp_error = ntp_error;
    persistent_state.wifi_error = wifi_error;
    watchdog_reboot((uint32_t)0, SRAM_END, (uint32_t)0 /* delay_ms */);
    while (1)
        __wfi(); // hang until reset
}

bool startup_led_callback(struct repeating_timer *t)
{
    (void)t;
    gpio_xor_mask(1 << DIAG_GREEN_LED_GPIO);
    return true;
}

static bool init_diagnostic_led(struct repeating_timer *t)
{
    gpio_init(DIAG_GREEN_LED_GPIO);
    gpio_set_dir(DIAG_GREEN_LED_GPIO, GPIO_OUT);
    gpio_put(DIAG_GREEN_LED_GPIO, 0);
    gpio_init(DIAG_RED_LED_GPIO);
    gpio_set_dir(DIAG_RED_LED_GPIO, GPIO_OUT);
    gpio_put(DIAG_RED_LED_GPIO, 0);

    return add_repeating_timer_ms(-500 /* 1Hz pulse */, startup_led_callback, NULL, t);
}

int main(void)
{
    clock_state_t state;
    clock_init(&state);

    struct repeating_timer startup_led_timer;
    bool startup_led_active = init_diagnostic_led(&startup_led_timer);

    int status = clock_start(&state);

    while (1) {
        clock_task(&state);

        if (state.ap_mode_triggered) {
            state.ap_mode_triggered = 0;
            start_wifi_access_point(&state.flash_config, config_store_handler, &state.wifi_initialized);
            system_reboot();
        }

        if (startup_led_active && state.ntp_state.status == NTP_IDLE) {
            cancel_repeating_timer(&startup_led_timer);
            gpio_put(DIAG_RED_LED_GPIO, 0);
            gpio_put(DIAG_GREEN_LED_GPIO, state.flash_config.led_always_on);
            startup_led_active = false;
        }

        sleep_ms(10);
    }

    return 0;
}