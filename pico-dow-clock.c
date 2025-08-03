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

#include "lwip/dns.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "ntp.h"

// Runs ntp test forever
void run_ntp_test(void) {
    NTP_T *state = ntp_init();
    if (!state) return;
    while (true) {
        if (absolute_time_diff_us(get_absolute_time(), state->ntp_test_time) < 0 && !state->dns_request_sent) {
            // Set alarm in case udp requests are lost
            state->ntp_resend_alarm = add_alarm_in_ms(NTP_RESEND_TIME, ntp_failed_handler, state, true);

            // cyw43_arch_lwip_begin/end should be used around calls into lwIP to
            // ensure correct locking. You can omit them if you are in a callback from
            // lwIP. Note that when using pico_cyw_arch_poll these calls are a no-op
            // and can be omitted, but it is a good practice to use them in case you
            // switch the cyw43_arch type later.
            cyw43_arch_lwip_begin();
            int err = dns_gethostbyname(NTP_SERVER, &state->ntp_server_address, ntp_dns_found, state);
            cyw43_arch_lwip_end();

            state->dns_request_sent = true;
            if (err == ERR_OK) {
                ntp_request(state);             // Cached result
            } else if (err != ERR_INPROGRESS) { // ERR_INPROGRESS means expect a callback
                printf("dns request failed\n");
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

#include "LCD_1in47.h"

#include "DEV_Config.h"
#include "Debug.h"
#include "GUI_Paint.h"
#include <stdlib.h> // malloc() free()

int LCD_1in47_test(void) {
    DEV_Delay_ms(2000);
    printf("LCD_1in47_test Demo\r\n");
    if (DEV_Module_Init() != 0) {
        return -1;
    }
    /* LCD Init */
    printf("Pico_LCD_1.47 demo...\r\n");
    LCD_1IN47_Init(VERTICAL);
    LCD_1IN47_Clear(WHITE);
    DEV_SET_PWM(0);
    UDOUBLE Imagesize = LCD_1IN47_HEIGHT * LCD_1IN47_WIDTH * 2;
    UWORD *BlackImage;
    if ((BlackImage = (UWORD *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        exit(0);
    }
    // /*1.Create a new image cache named IMAGE_RGB and fill it with white*/
    Paint_NewImage((UBYTE *)BlackImage, LCD_1IN47.WIDTH, LCD_1IN47.HEIGHT, 0, WHITE);
    Paint_SetScale(65);
    Paint_Clear(WHITE);
    Paint_SetRotate(ROTATE_0);
    // /* GUI */
    printf("drawing... (1)\r\n");
    // /*2.Drawing on the image*/
    DEV_SET_PWM(100);

    printf("drawing... (2)\r\n");
    Paint_DrawVariableWidthString(1, 85, "Hello, World!", &Roboto_Medium48, BLACK, WHITE);

    printf("drawing... (3)\r\n");
    // /*3.Refresh the picture in RAM to LCD*/
    LCD_1IN47_Display(BlackImage);
    DEV_Delay_ms(2000);

    printf("drawing... (4)\r\n");
    /* Module Exit */
    free(BlackImage);
    BlackImage = NULL;

    DEV_Module_Exit();
    return 0;
}

int main() {
    stdio_init_all();

    sleep_ms(2000);

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    } else {
        printf("Initialised\n");
    }

    cyw43_arch_enable_sta_mode();

    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Timeout\n");
    }
    printf("Connected to %s\n", WIFI_SSID);

    LCD_1in47_test();

    run_ntp_test();
    cyw43_arch_deinit();
    return 0;
}
