/**
 * Copyright (c) 2025 Jon Connell
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef WIFI_SSID
#error "WIFI_SSID is not defined. Please define it in secrets.cmake or via a compile flag."
#endif

#ifndef WIFI_PASSWORD
#error "WIFI_PASSWORD is not defined. Please define it in secrets.cmake or via a compile flag."
#endif

#include <string.h>
#include <time.h>

/* Pico SDK includes */
#include "lwip/dns.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

/* Waveshare SDK includes */
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "LCD_1in47.h"

/* Local includes */
#include "fonts_extra.h"
#include "gui_paint_extra.h"
#include "ntp.h"

// Runs ntp test forever
void run_ntp_test(void) {
    NTP_T *state = ntp_init();
    if (!state) return;
    while (true) {
        if (absolute_time_diff_us(get_absolute_time(), state->ntp_test_time) < 0 && !state->dns_request_sent) {
            // Set alarm in case udp requests are lost
            state->ntp_resend_alarm = add_alarm_in_ms(NTP_RESEND_TIME, ntp_failed_handler, state, true);

            cyw43_arch_lwip_begin();
            int err = dns_gethostbyname(NTP_SERVER, &state->ntp_server_address, ntp_dns_found, state);
            cyw43_arch_lwip_end();

            state->dns_request_sent = true;
            if (err == ERR_OK) {
                ntp_request(state);             // Cached result
            } else if (err != ERR_INPROGRESS) { // ERR_INPROGRESS means expect a callback
                printf("DNS request failed\n");
                ntp_result(state, -1, NULL);
            }
        }
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of
        // some (blocking) work you might be doing.
        sleep_ms(1000);
    }
    free(state);
}

int LCD_1in47_test(void) {
    if (DEV_Module_Init() != 0) {
        return -1;
    }

    LCD_1IN47_Init(VERTICAL);
    LCD_1IN47_Clear(BLACK);
    DEV_SET_PWM(0);
    UDOUBLE Imagesize = LCD_1IN47_HEIGHT * LCD_1IN47_WIDTH * 2;
    UWORD *BlackImage;
    if ((BlackImage = (UWORD *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        exit(0);
    }
    Paint_NewImage((UBYTE *)BlackImage, LCD_1IN47.WIDTH, LCD_1IN47.HEIGHT, 0, BLACK);
    // Paint_SetScale(65);
    Paint_Clear(BLACK);
    // Paint_SetRotate(ROTATE_90);
    DEV_SET_PWM(100);

    const char *text = "Hello, World!";
    for (char *p = (char *)text; *p; p++) {
        Paint_Clear(BLACK);
        Paint_DrawVariableWidthChar(0, 50, *p, &Roboto_Medium200, WHITE, BLACK);
        LCD_1IN47_Display(BlackImage);
        sleep_ms(500);
    }

    free(BlackImage);
    BlackImage = NULL;

    DEV_Module_Exit();
    return 0;
}

int main() {
    stdio_init_all();

    sleep_ms(2000);

    if (cyw43_arch_init()) {
        printf("Failed to initialise\n");
        return 1;
    } else {
        printf("Initialised\n");
    }

    cyw43_arch_enable_sta_mode();

    while (
        cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, /* timeout ms */ 10000)) {
        printf("Timeout: trying again\n");
    }
    printf("Connected to %s\n", WIFI_SSID);

    LCD_1in47_test();

    run_ntp_test();
    cyw43_arch_deinit();
    return 0;
}
