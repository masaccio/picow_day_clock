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

#include <stdarg.h>
#include <string.h>
#include <time.h>

/* Pico SDK includes */
#include "lwip/dns.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

/* Local includes */
#include "fonts_extra.h"
#include "gui_paint_extra.h"
#include "lcd.h"
#include "ntp.h"

// Determines if given UTC time is in British Summer Time (BST)
int is_bst(struct tm *utc) {
    if (!utc) return false;

    int year = utc->tm_year + 1900;

    // Find last Sunday in March
    struct tm start = {
        .tm_year = year - 1900, .tm_mon = 2, .tm_mday = 31, .tm_hour = 1, .tm_min = 0, .tm_sec = 0, .tm_isdst = 0};
    mktime(&start);
    while (start.tm_wday != 0) {
        start.tm_mday--;
        mktime(&start);
    }

    // Find last Sunday in October
    struct tm end = {
        .tm_year = year - 1900, .tm_mon = 9, .tm_mday = 31, .tm_hour = 1, .tm_min = 0, .tm_sec = 0, .tm_isdst = 1};
    mktime(&end);
    while (end.tm_wday != 0) {
        end.tm_mday--;
        mktime(&end);
    }

    time_t now = mktime(utc);
    time_t start_time = mktime(&start);
    time_t end_time = mktime(&end);

    return now >= start_time && now < end_time;
}

void ntp_timer_callback(lcd_state_t *state, time_t *ntp_time) {
    int bst = is_bst(gmtime(ntp_time));
    time_t local_time_t = *ntp_time + (bst ? 3600 : 0);
    struct tm *local = gmtime(&local_time_t);

    printf("NTP time is %02d/%02d/%04d %02d:%02d:%02d (%s)\r\n", local->tm_mday, local->tm_mon + 1,
           local->tm_year + 1900, local->tm_hour, local->tm_min, local->tm_sec, bst ? "BST" : "GMT");

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "NTP time is %02d/%02d/%04d %02d:%02d:%02d (%s)\r\n", local->tm_mday,
             local->tm_mon + 1, local->tm_year + 1900, local->tm_hour, local->tm_min, local->tm_sec,
             bst ? "BST" : "GMT");
    print_line(state, buffer);
}

// Runs ntp test forever
void run_ntp_test(lcd_state_t *lcd_state) {
    ntp_state_t *ntp_state = ntp_init(lcd_state, ntp_timer_callback);
    if (!ntp_state) return;
    while (true) {
        if (absolute_time_diff_us(get_absolute_time(), ntp_state->ntp_test_time) < 0 && !ntp_state->dns_request_sent) {
            // Set alarm in case udp requests are lost
            ntp_state->ntp_resend_alarm = add_alarm_in_ms(NTP_RESEND_TIME, ntp_failed_handler, ntp_state, true);

            cyw43_arch_lwip_begin();
            int err = dns_gethostbyname(NTP_SERVER, &ntp_state->ntp_server_address, ntp_dns_found, ntp_state);
            cyw43_arch_lwip_end();

            ntp_state->dns_request_sent = true;
            if (err == ERR_OK) {
                ntp_request(ntp_state);         // Cached result
            } else if (err != ERR_INPROGRESS) { // ERR_INPROGRESS means expect a callback
                print_line(lcd_state, "DNS request failed");
                ntp_result(ntp_state, -1, NULL);
            }
        }
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of
        // some (blocking) work you might be doing.
        sleep_ms(1000);
    }
    free(ntp_state);
}

// void core1_lcd() {
//     printf("Core1: starting\r\n");
//     init_lcd(lcd_state.frame_buffer);

//     Paint_SetRotate(ROTATE_0);
//     lcd_state print_lcd(0, 0, "LCD initialised");
//     LCD_1IN47_Display(lcd_state.frame_buffer);

//     while (true) {
//         if (clock_state.lcd_update_requested) {
//             printf("Core1: updating LCD\r\n");
//             clock_state.lcd_update_requested = false;
//             LCD_1IN47_Display(clock_state.frame_buffer);
//         }
//         sleep_ms(10);
//     }
// }

// int core0_lwip() {
//     stdio_init_all();

//     // Launch LCD code on core 1 first
//     multicore_launch_core1(core1_lcd);

//     // Initialize Wi-Fi on core 0
//     if (cyw43_arch_init()) {
//         printf("Core0: Failed to initialise cyw43\r\n");
//         return 1;
//     } else {
//         printf("Core0: cyw43 initialized\r\n");
//     }

//     printf("Core0: Attempting connection to: %s\r\n", WIFI_SSID);
//     cyw43_arch_enable_sta_mode();

//     while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK,
//                                               /* timeout ms */ 10000)) {
//         printf("Core0: Timeout: trying again\r\n");
//     }

//     // Wi-Fi connected: update LCD with status
//     print_lcd(0, 26, "Wi-Fi initialised:");
//     print_lcd(0, 52, WIFI_SSID);

//     // Signal core1 to update LCD
//     clock_state.lcd_update_requested = true;

//     printf("Core0: Connected to %s\r\n", WIFI_SSID);

//     // Run your NTP test or main app logic
//     run_ntp_test(&clock_state);

//     // Cleanup (if ever reached)
//     free(clock_state.frame_buffer);
//     DEV_Module_Exit();
//     cyw43_arch_deinit();
//     return 0;
// }

int single_core_main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("Failed to initialise\r\n");
        return 1;
    } else {
        printf("Initialised\r\n");
    }

    printf("Attempting connection to: %s\r\n", WIFI_SSID);
    cyw43_arch_enable_sta_mode();
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK,
                                              /* timeout ms */ 5000)) {
        printf("Timeout: trying again\r\n");
    }

    lcd_state_t *lcd_state = init_lcd();

    Paint_SetRotate(ROTATE_0);
    print_line(lcd_state, "LCD initialised");
    LCD_1IN47_Display(lcd_state->frame_buffer);

    print_line(lcd_state, "Wi-Fi initialised:");
    print_line(lcd_state, WIFI_SSID);
    printf("Connected to %s\r\n", WIFI_SSID);
    LCD_1IN47_Display(lcd_state->frame_buffer);

    run_ntp_test(lcd_state);

    free(lcd_state->frame_buffer);
    DEV_Module_Exit();
    cyw43_arch_deinit();
    return 0;
}

int main() {
    // return core0_lwip();
    return single_core_main();
}