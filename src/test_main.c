#include <setjmp.h>

#include "mock.h"
#include "test.h"

extern int test_main(void);

static jmp_buf fatal_jmp_buf;
persistent_state_t persistent_state;

extern void mock_reset_icons(void);

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

void __attribute__((noreturn)) fatal_reset(clock_state_t *state, ntp_error_t ntp_error, wifi_error_t wifi_error)
{
    persistent_state.ntp_error = ntp_error;
    persistent_state.wifi_error = wifi_error;
    mock_ctx.spy.fatal_ntp_error = ntp_error;
    mock_ctx.spy.fatal_wifi_error = wifi_error;
    watchdog_reboot((uint32_t)0, SRAM_END, (uint32_t)0 /* delay_ms */);

    free_memory(state);

    // Returns into main() which will then exit with status=1
    longjmp(fatal_jmp_buf, 1);
}

void on_clock_alloc_failed(void)
{
    mock_ctx.spy.clock_state_alloc_failed = 1;
}

void on_lcd_init_failed(unsigned lcd_num)
{
    mock_ctx.spy.lcd_init_failed = lcd_num;
}

int test_main(void)
{
    mock_reset_icons();
    mock_printf("Test init\n");

    mock_ctx.spy.watchdog_reboot_called = 0;

    if (setjmp(fatal_jmp_buf)) {
        mock_ctx.spy.fatal_reset_caught = 1;
        // Restart the clock to ensure that fault state is captured
        // but don't enter the main loop and return to test harness.
        (void)clock_init();
        return 1;
    }

    clock_state_t *state = clock_init();
    if (!state)
        return 1;

    int status = clock_main_loop(state);
    free_memory(state);
    return status != 0;
}
