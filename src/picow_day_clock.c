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

/* Pico SDK */
#include "lwip/dns.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

/* Local includes */
#include "common.h"
#include "fonts_extra.h"
#include "gui_paint_extra.h"
#include "lcd.h"
#include "ntp.h"

#include "Debug.h" /* Waveshare */

#define WIFI_CONNECT_TIMEOUT_MS (10 * 1000)
#define WIFI_ABANDON_TIMEOUT_MS (60 * 1000)
#define WIFI_BAD_AUTH_RETRY_COUNT 3
#define WIFI_BAD_AUTH_RETRY_DELAY_MS 500

typedef struct clock_state_t {
    bool ntp_updated;
    time_t ntp_time;
    lcd_state_t *lcd_state;
    ntp_state_t *ntp_state;
} clock_state_t;

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

void ntp_timer_callback(void *state, time_t *ntp_time) {
    clock_state_t *clock_state = (clock_state_t *)state;
    clock_state->ntp_time = *ntp_time;
}

void debug_print_time(time_t ntp_time) {
    int bst = is_bst(gmtime(&ntp_time));
    time_t local_time_t = ntp_time + (bst ? 3600 : 0);
    struct tm *local = gmtime(&local_time_t);

    CLOCK_DEBUG("NTP: time is %02d/%02d/%04d %02d:%02d:%02d (%s)\r\n", local->tm_mday, local->tm_mon + 1,
                local->tm_year + 1900, local->tm_hour, local->tm_min, local->tm_sec, bst ? "BST" : "GMT");
}

bool connect_to_wifi(const char ssid[], const char password[]) {
    CLOCK_DEBUG("Wi-Fi: attempting connection to '%s'\r\n", ssid);

    if (cyw43_arch_init()) {
        CLOCK_DEBUG("Wi-Fi: failed to initialise CYW43\r\n");
        return 1;
    } else {
        CLOCK_DEBUG("Wi-Fi: CYW43 initialised\r\n");
    }

    cyw43_arch_enable_sta_mode();

    absolute_time_t start_time = get_absolute_time();
    int bad_auth_count = 0;
    while (true) {
        int ret = cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECT_TIMEOUT_MS);

        if (ret == 0) {
            CLOCK_DEBUG("Wi-Fi: connected to %s\r\n", ssid);
            return true;
        } else if (ret == PICO_ERROR_TIMEOUT) {
            if (absolute_time_diff_us(start_time, get_absolute_time()) > (WIFI_ABANDON_TIMEOUT_MS * 1000)) {
                CLOCK_DEBUG("Wi-Fi: exceeded maximum timeout; giving up\r\n");
                return false;
            }
            CLOCK_DEBUG("Wi-Fi: timeout; trying again\r\n");
        } else if (ret == PICO_ERROR_BADAUTH) {
            if (++bad_auth_count > WIFI_BAD_AUTH_RETRY_COUNT) {
                CLOCK_DEBUG("Wi-Fi: too many bad authentication failures; giving up\r\n");
                return false;
            } else {
                CLOCK_DEBUG("Wi-Fi: invalid credentials; retrying...\r\n");
                sleep_ms(WIFI_BAD_AUTH_RETRY_DELAY_MS);
            }
        } else if (ret == PICO_ERROR_CONNECT_FAILED) {
            CLOCK_DEBUG("Wi-Fi: connection failed for unknown reason; giving up\r\n");
            return false;
        } else {
            CLOCK_DEBUG("Wi-Fi: unknown error %d; giving up\r\n", ret);
            return false;
        }
    }
}

int LCD_1in47_test(void) {
    DEV_Delay_ms(100);
    Debug("LCD_1in47_test Demo\r\n");
    if (DEV_Module_Init() != 0) {
        return -1;
    }
    /* LCD Init */
    Debug("Pico_LCD_1.47 demo...\r\n");
    LCD_1IN47_Init(VERTICAL);
    LCD_1IN47_Clear(WHITE);
    DEV_SET_PWM(0);
    UDOUBLE Imagesize = LCD_1IN47_HEIGHT * LCD_1IN47_WIDTH * 2;
    UWORD *BlackImage;
    if ((BlackImage = (UWORD *)malloc(Imagesize)) == NULL) {
        Debug("Failed to apply for black memory...\r\n");
        exit(0);
    }
    // /*1.Create a new image cache named IMAGE_RGB and fill it with white*/
    Paint_NewImage((UBYTE *)BlackImage, LCD_1IN47.WIDTH, LCD_1IN47.HEIGHT, 0, WHITE);
    Paint_SetScale(65);
    Paint_Clear(WHITE);
    Paint_SetRotate(ROTATE_0);
    // /* GUI */
    Debug("drawing...\r\n");
    // /*2.Drawing on the image*/
    DEV_SET_PWM(100);

    Paint_DrawPoint(2, 18, BLACK, DOT_PIXEL_1X1, DOT_FILL_RIGHTUP); // 240 240
    Paint_DrawPoint(2, 20, BLACK, DOT_PIXEL_2X2, DOT_FILL_RIGHTUP);
    Paint_DrawPoint(2, 23, BLACK, DOT_PIXEL_3X3, DOT_FILL_RIGHTUP);
    Paint_DrawPoint(2, 28, BLACK, DOT_PIXEL_4X4, DOT_FILL_RIGHTUP);
    Paint_DrawPoint(2, 33, BLACK, DOT_PIXEL_5X5, DOT_FILL_RIGHTUP);

    Paint_DrawLine(20, 5, 80, 65, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(20, 65, 80, 5, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    Paint_DrawLine(148, 35, 208, 35, CYAN, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine(178, 5, 178, 65, CYAN, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

    Paint_DrawRectangle(20, 5, 80, 65, RED, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(85, 5, 145, 65, BLUE, DOT_PIXEL_2X2, DRAW_FILL_FULL);

    Paint_DrawCircle(178, 35, 30, GREEN, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(240, 35, 30, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    Paint_DrawString_EN(1, 70, "AaBbCc123", &Font16, RED, WHITE);
    Paint_DrawNum(130, 85, 9.87654321, &Font20, 7, WHITE, BLACK);
    Paint_DrawString_EN(1, 85, "AaBbCc123", &Font20, 0x000f, 0xfff0);
    Paint_DrawString_EN(1, 105, "AaBbCc123", &Font24, RED, WHITE);
    Paint_DrawString_CN(1, 130, "��ӭʹ��Abc", &Font24CN, WHITE, BLUE);

    /* Module Exit */
    free(BlackImage);
    BlackImage = NULL;

    DEV_Module_Exit();
    Debug("LCD demo done\r\n");
    return 0;
}

int main() {
    clock_state_t *clock_state = (clock_state_t *)calloc(1, sizeof(clock_state_t));

    stdio_init_all();

    if (!connect_to_wifi(WIFI_SSID, WIFI_PASSWORD)) {
        return 1;
    }

    clock_state->ntp_state = ntp_init((void *)clock_state, ntp_timer_callback);
    if (clock_state->ntp_state == NULL) {
        CLOCK_DEBUG("NTP: failed to initialise\r\n");
        return 1;
    }

    ntp_status_t ntp_status = ntp_get_time(clock_state->ntp_state);
    if (ntp_status != NTP_STATUS_SUCCESS) {
        CLOCK_DEBUG("NTP: get time failed\r\n");
        return 1;
    } else {
        debug_print_time(clock_state->ntp_time);
    }

    clock_state->lcd_state = lcd_init();
    if (clock_state->lcd_state == NULL) {
        CLOCK_DEBUG("LCD: failed to initialise\r\n");
        return 1;
    }

    LCD_1in47_test();

    sleep_ms(2000));

    clear_screen(clock_state->lcd_state);
    Paint_SetRotate(ROTATE_0);
    print_line(clock_state->lcd_state, "SUCCESS!");

    ntp_status = ntp_get_time(clock_state->ntp_state);
    if (ntp_status != NTP_STATUS_SUCCESS) {
        CLOCK_DEBUG("NTP: get time failed\r\n");
        return 1;
    } else {
        debug_print_time(clock_state->ntp_time);
    }

    cyw43_arch_deinit();
    return 0;
}
