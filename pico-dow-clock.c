/**
 * NTP code is derived from picow_ntp_client.c in the Rasperry Pi Pico
 * examples at https://github.com/raspberrypi/pico-examples/ which is
 * licebsed in the BSD-3-Clause license with this notice:
 *
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * All additions are also licensed under the BSD-3-Clause license as follows:
 *
 * Copyright (c) 2025 Jon Connell
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef WIFI_SSID
#error                                                                         \
    "WIFI_SSID is not defined. Please define it in secrets.cmake or via a compile flag."
#endif

#ifndef WIFI_PASSWORD
#error                                                                         \
    "WIFI_PASSWORD is not defined. Please define it in secrets.cmake or via a compile flag."
#endif

#include <string.h>
#include <time.h>

#include "hardware/spi.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

typedef struct NTP_T_ {
  ip_addr_t ntp_server_address;
  bool dns_request_sent;
  struct udp_pcb *ntp_pcb;
  absolute_time_t ntp_test_time;
  alarm_id_t ntp_resend_alarm;
} NTP_T;

#ifndef NTP_SERVER
#define NTP_SERVER "pool.ntp.org" // Default NTP server
#endif
#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_TEST_TIME (30 * 1000)
#define NTP_RESEND_TIME (10 * 1000)

#include "pico/stdlib.h"

// Determines if given UTC time is in British Summer Time (BST)
int is_bst(struct tm *utc) {
  if (!utc)
    return false;

  int year = utc->tm_year + 1900;

  // Find last Sunday in March
  struct tm start = {.tm_year = year - 1900,
                     .tm_mon = 2,
                     .tm_mday = 31,
                     .tm_hour = 1,
                     .tm_min = 0,
                     .tm_sec = 0,
                     .tm_isdst = 0};
  mktime(&start);
  while (start.tm_wday != 0) {
    start.tm_mday--;
    mktime(&start);
  }

  // Find last Sunday in October
  struct tm end = {.tm_year = year - 1900,
                   .tm_mon = 9,
                   .tm_mday = 31,
                   .tm_hour = 1,
                   .tm_min = 0,
                   .tm_sec = 0,
                   .tm_isdst = 1};
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

// Called with results of operation
static void ntp_result(NTP_T *state, int status, time_t *result) {
  if (status == 0 && result) {
    int bst = is_bst(gmtime(result));
    time_t local_time_t = *result + (bst ? 3600 : 0);
    struct tm *local = gmtime(&local_time_t);

    printf("NTP time is %02d/%02d/%04d %02d:%02d:%02d (%s)\n", local->tm_mday,
           local->tm_mon + 1, local->tm_year + 1900, local->tm_hour,
           local->tm_min, local->tm_sec, bst ? "BST" : "GMT");
  }

  if (state->ntp_resend_alarm > 0) {
    cancel_alarm(state->ntp_resend_alarm);
    state->ntp_resend_alarm = 0;
  }
  state->ntp_test_time = make_timeout_time_ms(NTP_TEST_TIME);
  state->dns_request_sent = false;
}

static int64_t ntp_failed_handler(alarm_id_t id, void *user_data);

// Make an NTP request
static void ntp_request(NTP_T *state) {
  // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure
  // correct locking. You can omit them if you are in a callback from lwIP. Note
  // that when using pico_cyw_arch_poll these calls are a no-op and can be
  // omitted, but it is a good practice to use them in case you switch the
  // cyw43_arch type later.
  cyw43_arch_lwip_begin();
  struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
  uint8_t *req = (uint8_t *)p->payload;
  memset(req, 0, NTP_MSG_LEN);
  req[0] = 0x1b;
  udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
  pbuf_free(p);
  cyw43_arch_lwip_end();
}

static int64_t ntp_failed_handler(alarm_id_t id, void *user_data) {
  NTP_T *state = (NTP_T *)user_data;
  printf("ntp request failed\n");
  ntp_result(state, -1, NULL);
  return 0;
}

// Call back with a DNS result
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr,
                          void *arg) {
  NTP_T *state = (NTP_T *)arg;
  if (ipaddr) {
    state->ntp_server_address = *ipaddr;
    printf("NTP address: %s\n", ipaddr_ntoa(ipaddr));
    ntp_request(state);
  } else {
    printf("NTP DNS request failed\n");
    ntp_result(state, -1, NULL);
  }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                     const ip_addr_t *addr, u16_t port) {
  NTP_T *state = (NTP_T *)arg;
  uint8_t mode = pbuf_get_at(p, 0) & 0x7;
  uint8_t stratum = pbuf_get_at(p, 1);

  // Check the result
  if (ip_addr_cmp(addr, &state->ntp_server_address) && port == NTP_PORT &&
      p->tot_len == NTP_MSG_LEN && mode == 0x4 && stratum != 0) {
    uint8_t seconds_buf[4] = {0};
    pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
    uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 |
                                  seconds_buf[2] << 8 | seconds_buf[3];
    uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
    time_t epoch = seconds_since_1970;
    ntp_result(state, 0, &epoch);
  } else {
    printf("invalid ntp response\n");
    ntp_result(state, -1, NULL);
  }
  pbuf_free(p);
}

// Perform initialisation
static NTP_T *ntp_init(void) {
  NTP_T *state = (NTP_T *)calloc(1, sizeof(NTP_T));
  if (!state) {
    printf("failed to allocate state\n");
    return NULL;
  }
  state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
  if (!state->ntp_pcb) {
    printf("failed to create pcb\n");
    free(state);
    return NULL;
  }
  udp_recv(state->ntp_pcb, ntp_recv, state);
  return state;
}

// Runs ntp test forever
void run_ntp_test(void) {
  NTP_T *state = ntp_init();
  if (!state)
    return;
  while (true) {
    if (absolute_time_diff_us(get_absolute_time(), state->ntp_test_time) < 0 &&
        !state->dns_request_sent) {
      // Set alarm in case udp requests are lost
      state->ntp_resend_alarm =
          add_alarm_in_ms(NTP_RESEND_TIME, ntp_failed_handler, state, true);

      // cyw43_arch_lwip_begin/end should be used around calls into lwIP to
      // ensure correct locking. You can omit them if you are in a callback from
      // lwIP. Note that when using pico_cyw_arch_poll these calls are a no-op
      // and can be omitted, but it is a good practice to use them in case you
      // switch the cyw43_arch type later.
      cyw43_arch_lwip_begin();
      int err = dns_gethostbyname(NTP_SERVER, &state->ntp_server_address,
                                  ntp_dns_found, state);
      cyw43_arch_lwip_end();

      state->dns_request_sent = true;
      if (err == ERR_OK) {
        ntp_request(state); // Cached result
      } else if (err !=
                 ERR_INPROGRESS) { // ERR_INPROGRESS means expect a callback
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

int Paint_DrawVariableWidthChar(UWORD Xpoint, UWORD Ypoint,
                                const char Acsii_Char, vFONT *Font,
                                UWORD Color_Foreground,
                                UWORD Color_Background) {
  UWORD Page, Column;

  const vFONTENTRY *entry = &Font->table[Acsii_Char - ' '];
  const uint8_t *ptr = entry->table;
  int font_height = Font->Height;
  int font_width = entry->Width;

  Debug("Paint_DrawVariableWidthChar: character '%c', font_height=%d\r\n",
        Acsii_Char, font_height);

  for (Page = 0; Page < Font->Height; Page++) {
    for (Column = 0; Column < font_width; Column++) {
      // To determine whether the font background color and screen background
      // color is consistent
      if (FONT_BACKGROUND ==
          Color_Background) { // this process is to speed up the scan
        if (*ptr & (0x80 >> (Column % 8))) {
          Debug("Paint_DrawVariableWidthChar: [%d,%d] 0x%04x\r\n",
                Xpoint + Column, Ypoint + Page, Color_Foreground);
          Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
        }
      } else {
        if (*ptr & (0x80 >> (Column % 8))) {
          Debug("Paint_DrawVariableWidthChar: [%d,%d] 0x%04x\r\n",
                Xpoint + Column, Ypoint + Page, Color_Foreground);
          Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
        } else {
          Debug("Paint_DrawVariableWidthChar: [%d,%d] 0x%04x\r\n",
                Xpoint + Column, Ypoint + Page, Color_Background);
          Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Background);
        }
      }
      // One pixel is 8 bits
      if (Column % 8 == 7)
        ptr++;
    } // Write a line
    if (font_width % 8 != 0)
      ptr++;
  } // Write all

  Debug("Paint_DrawVariableWidthChar: last width %d\n", font_width);
  return font_width;
}

void Paint_DrawVariableWidthString(UWORD Xstart, UWORD Ystart,
                                   const char *pString, vFONT *Font,
                                   UWORD Color_Foreground,
                                   UWORD Color_Background) {
  UWORD Xpoint = Xstart;
  UWORD Ypoint = Ystart;

  Debug("Paint_DrawVariableWidthString: starting\r\n");

  if (Xstart > Paint.Width || Ystart > Paint.Height) {
    Debug("Paint_DrawVariableWidthString Input exceeds the normal display "
          "range\r\n");
    return;
  }

  int width = 0;
  while (*pString != '\0') {
    // if X direction filled , reposition to(Xstart,Ypoint),Ypoint is Y
    // direction plus the Height of the character
    if ((Xpoint + width) > Paint.Width) {
      Xpoint = Xstart;
      Ypoint += Font->Height;
    }

    Debug("Paint_DrawVariableWidthString: character '%c'\r\n", *pString);
    // If the Y direction is full, reposition to(Xstart, Ystart)
    if ((Ypoint + Font->Height) > Paint.Height) {
      Xpoint = Xstart;
      Ypoint = Ystart;
    }
    width = Paint_DrawVariableWidthChar(Xpoint, Ypoint, *pString, Font,
                                        Color_Foreground, Color_Background);

    // The next character of the address
    pString++;

    Xpoint += width;
  }
}

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
  Paint_NewImage((UBYTE *)BlackImage, LCD_1IN47.WIDTH, LCD_1IN47.HEIGHT, 0,
                 WHITE);
  Paint_SetScale(65);
  Paint_Clear(WHITE);
  Paint_SetRotate(ROTATE_0);
  // /* GUI */
  printf("drawing... (1)\r\n");
  // /*2.Drawing on the image*/
  DEV_SET_PWM(100);

  printf("drawing... (2)\r\n");
  Paint_DrawVariableWidthString(1, 85, "Hello, World!", &Roboto_Medium48, BLACK,
                                WHITE);

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

  if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
                                         CYW43_AUTH_WPA2_AES_PSK, 10000)) {
    printf("Failed to connect\n");
    return 1;
  }
  printf("Connected to %s\n", WIFI_SSID);

  // spi_init(SPI_PORT, 40 * 1000 * 1000);
  // gpio_set_function(PIN_LCD_SCK, GPIO_FUNC_SPI);
  // gpio_set_function(PIN_LCD_MOSI, GPIO_FUNC_SPI);

  // // Initialize control pins
  // gpio_init(PIN_LCD_CS);
  // gpio_set_dir(PIN_LCD_CS, GPIO_OUT);
  // gpio_put(PIN_LCD_CS, 1);

  // gpio_init(PIN_LCD_DC);
  // gpio_set_dir(PIN_LCD_DC, GPIO_OUT);

  // gpio_init(PIN_LCD_RST);
  // gpio_set_dir(PIN_LCD_RST, GPIO_OUT);

  // gpio_init(PIN_LCD_BL);
  // gpio_set_dir(PIN_LCD_BL, GPIO_OUT);
  // gpio_put(PIN_LCD_BL, 1); // Backlight on

  // printf("SPI init done\n");

  // lcd_init();

  // // Clear screen to black
  // lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_BLACK);

  // printf("LCD init done\n");

  // // Calculate max scale to fill screen width for one digit
  // // 5 cols * scale <= LCD_WIDTH (240), 7 rows * scale <= LCD_HEIGHT (240)
  // // So scale = min(240/5, 240/7) = 34 (integer division)
  // uint16_t scale = 34;

  // // Draw digits 0-9 in a row
  // for (uint8_t d = 0; d <= 9; d++) {
  //   uint16_t x = d * 5 * scale; // digits side by side
  //   if (x + 5 * scale > LCD_WIDTH)
  //     break;
  //   lcd_draw_digit(d, x, 0, scale, COLOR_WHITE, COLOR_BLACK);
  // }

  LCD_1in47_test();

  run_ntp_test();
  cyw43_arch_deinit();
  return 0;
}
