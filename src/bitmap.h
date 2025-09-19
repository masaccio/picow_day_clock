#ifndef _BITMAP_H
#define _BITMAP_H

#include <stdint.h>

typedef struct
{
    const uint8_t width;
    const uint8_t *table;
} font_glyph_t;

typedef struct
{
    const font_glyph_t *table;
    uint16_t byte_width;
    uint16_t height;
} font_t;

extern font_t text_font;
extern font_t clock_digit_font;

#define ICON_SIZE 36
#define BYTES_PER_ROW (ICON_SIZE + 7) / 8
#define ICON_BYTES (BYTES_PER_ROW * ICON_SIZE)

extern const uint8_t watchdog_icon[ICON_BYTES];
extern const uint8_t wifi_icon[ICON_BYTES];
extern const uint8_t ntp_icon[ICON_BYTES];
extern const uint8_t wifi_init_icon[ICON_BYTES];
extern const uint8_t wifi_timeout_icon[ICON_BYTES];
extern const uint8_t wifi_password_icon[ICON_BYTES];
extern const uint8_t wifi_connect_icon[ICON_BYTES];
extern const uint8_t wifi_error_icon[ICON_BYTES];
extern const uint8_t ntp_init_icon[ICON_BYTES];
extern const uint8_t ntp_dns_icon[ICON_BYTES];
extern const uint8_t ntp_timeout_icon[ICON_BYTES];
extern const uint8_t ntp_memory_icon[ICON_BYTES];
extern const uint8_t ntp_error_icon[ICON_BYTES];

#endif // _BITMAP_H
