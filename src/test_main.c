#include <setjmp.h>

#include "clock.h"
#include "mock.h"
#include "ntp.h"
#include "test.h"
#include "watchdog.h"
#include "wifi.h"

extern int test_main(void);

static jmp_buf fatal_jmp_buf;
persistent_state_t persistent_state;

static void free_memory(clock_state_t *state)
{
    if (!state)
        return;

    free(state->ntp_state);
    for (unsigned int ii = 0; ii < NUM_LCDS; ii++)
        if (state->lcd_states[ii])
            free(state->lcd_states[ii]);
    free(state);
}

void fatal_reset(clock_state_t *state, ntp_error_t ntp_error, wifi_error_t wifi_error)
{
    persistent_state.ntp_error = ntp_error;
    persistent_state.wifi_error = wifi_error;
    mock_ctx.spy.fatal_ntp_error = ntp_error;
    mock_ctx.spy.fatal_wifi_error = wifi_error;
    watchdog_reboot((uint32_t)0, SRAM_END, (uint32_t)0 /* delay_ms */);

    if (mock_ctx.inject.fatal_reset_no_longjmp)
        return;

    free_memory(state);

    // Returns into main() which will then exit with status=1
    longjmp(fatal_jmp_buf, 1);
}

void on_clock_alloc_failed(void)
{
    mock_ctx.spy.clock_state_alloc_failed = 1;
}

void on_lcd_init_failed(clock_state_t *state, unsigned lcd_num)
{
    mock_ctx.spy.lcd_init_failed = lcd_num;
    free_memory(state);
}

int test_main(void)
{
    test_printf("Test init\n");

    mock_ctx.spy.watchdog_reboot_called = 0;

    if (setjmp(fatal_jmp_buf)) {
        mock_ctx.spy.fatal_reset_caught = 1;
        // Reboot the clock to ensure that fault state is captured.
        // Don't enter the main loop and instead return to test harness.
        clock_state_t *state = clock_init();
        free_memory(state);
        return 1;
    }

    clock_state_t *state = clock_init();
    if (!state)
        return 1;

    int status = clock_start(state);
    if (status != 0) {
        free_memory(state);
        return 1;
    }

    status = 0;
    while (1) {
        if (mock_ctx.spy.fatal_reset_caught) {
            status = 1;
            break;
        } else if (state->ntp_state->status == NTP_FAILED) {
            fatal_reset(state, state->ntp_state->error, WIFI_OK);
        } else if (state->ntp_state->status != NTP_SUCCESS && state->ntp_state->status != NTP_PENDING) {
            break;
        } else if (mock_ctx.inject.exit_on_ntp_success && state->ntp_state->status == NTP_SUCCESS) {
            // Some tests expect normal operation, but we still need to return to the test harness
            break;
        } else if (mock_ctx.inject.exit_after_ms && mock_ctx.spy.boot_time_ms >= mock_ctx.inject.exit_after_ms) {
            break;
        }
        clock_task(state);
        sleep_ms(10);
    }

    free_memory(state);
    return status;
}
