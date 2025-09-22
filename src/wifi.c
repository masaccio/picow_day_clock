// Pico SDK
#ifndef TEST_MODE
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#else
#include "mock.h"
#endif

// Local includes
#include "clock.h"
#include "wifi.h"

wifi_status_t connect_to_wifi(const char ssid[], const char password[])
{
    if (cyw43_arch_init() != 0) {
        return WIFI_STATUS_INIT_FAIL;
    }

    cyw43_arch_enable_sta_mode();

    absolute_time_t start_time_us = get_absolute_time();
    int bad_auth_count = 0;
    while (true) {
        int ret = cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECT_TIMEOUT_MS);

        if (ret == 0) {
            CLOCK_DEBUG("Wi-Fi: connected to %s\r\n", ssid);
            return WIFI_STATUS_SUCCESS;
        } else if (ret == PICO_ERROR_TIMEOUT) {
            if (absolute_time_diff_us(start_time_us, get_absolute_time()) >= (WIFI_ABANDON_TIMEOUT_MS * 1000)) {
                CLOCK_DEBUG("Wi-Fi: exceeded maximum timeout; giving up\r\n");
                return WIFI_STATUS_TIMEOUT;
            }
            CLOCK_DEBUG("Wi-Fi: timeout; trying again\r\n");
        } else if (ret == PICO_ERROR_BADAUTH) {
            bad_auth_count++;
            if (bad_auth_count >= WIFI_BAD_AUTH_RETRY_COUNT) {
                CLOCK_DEBUG("Wi-Fi: too many bad authentication failures; giving up\r\n");
                return WIFI_STATUS_BAD_AUTH;
            } else {
                CLOCK_DEBUG("Wi-Fi: invalid credentials; retrying...\r\n");
                sleep_ms(WIFI_BAD_AUTH_RETRY_DELAY_MS);
            }
        } else if (ret == PICO_ERROR_CONNECT_FAILED) {
            CLOCK_DEBUG("Wi-Fi: connection failed for unknown reason; giving up\r\n");
            return WIFI_STATUS_CONNECT_FAILED;
        } else {
            CLOCK_DEBUG("Wi-Fi: unknown error %d; giving up\r\n", ret);
            return WIFI_STATUS_UNKNOWN_ERROR;
        }
    }
}
