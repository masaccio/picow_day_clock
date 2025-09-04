/* Pico SDK */
#include "pico/cyw43_arch.h"

/* Local includes */
#include "common.h"
#include "hal.h"
#include "wifi.h"

bool connect_to_wifi(hal_t *hal, const char ssid[], const char password[])
{
    if (hal->cyw43_arch_init()) {
        CLOCK_DEBUG("Wi-Fi: failed to initialise CYW43\r\n");
        return 1;
    }

    hal->cyw43_arch_enable_sta_mode();

    absolute_time_t start_time = get_absolute_time();
    int bad_auth_count = 0;
    while (true) {
        int ret =
            hal->cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECT_TIMEOUT_MS);

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
                hal->sleep_ms(WIFI_BAD_AUTH_RETRY_DELAY_MS);
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

void disconnect_from_wifi(hal_t *hal)
{
    hal->cyw43_arch_deinit();
}