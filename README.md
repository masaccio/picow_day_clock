# Raspberry Pi Pico Day of Week Clock

**THIS PROJECT IS A WORK IN PROGRESS**

![Current prototype](https://raw.githubusercontent.com/masaccio/picow_day_clock/main/images/prototype.jpeg)

## Planned work

* Test the TXB0108 is able to drive all 7 LCDs. It seems to have quite weak drivers so likely needs replacing
* Add a USB port and connect through to the Pico's USB with the correct isolation
* Improve the handling of errors:
  * Once the watchdog reboots, the watchdog icon permanently displays
  * Provide feedback from within the Wi-Fi and NTP modules so that timeouts are clearer
* Build the prototype into a more final layout that is more compact
* Build an enclosure. I plan to create two shallow wooden boxes, put some brass between them and encase the LCD screens in class domes
* Get the Windows CI build working for the test and coverage build
* Make the configuration cleaner with better CMake overrides of `config.h`
* And probably more...

## Introduction

This project is the software, hardware design and clock enclosure design for an LCD clock driven by the Raspberry Pi Pico 2 W. The clock design is inspired by the idea of a [Nixie](https://en.wikipedia.org/wiki/Nixie_tube) clock but uses LCDs so that the display is brighter and clearly visible in daylight. The clock is intended to be wall-mounted so the read is left a little messy to simplify the electronics build.

The project is *not* compatible with earlier versions of the Pico and requires the CYW43439 Wi-Fi chip present in the Pico 2 W to function correctly.

## Software Operation

The pin-out and Wi-Fi settings are hard-coded into the project at buuld time. After the Pico firmware boots, the software will:

* Initialise all 7 LCD modules writing to the UART and exiting if this fails
* Create a Wi-Fi connection to the hard-coded Wi-Fi networt
* Get the current time from public NTP servers
* Display the abbreviated day of week on the first three LCDs and the current time in hours in minutes on the remaining 4 displays

NTP syncs start at once per day and respect back-off requests from the server if this is too frequent. The timer itself runs once per second and any NTP drift that occurs daily is compensated for in these timer callbacks.

### Diagnostics

A watchdog timer runs on the clock and reboots the Pico if there is no activity for `WATCHDOG_TIMEOUT_MS` which is 3 seconds by default. On reboot, the clock reconnects to Wi-Fi and resyncs with NTP.

The clock displays icons along the top edge of LCD1 when errors occur. The icons indocate watchdog status, NTP status and Wi-Fi status. They are coloured red for errors and green for normal status and indicate the nature of the error that occured such as a Wi-Fi authentication provblem or network timeout.

![Example display icons](https://raw.githubusercontent.com/masaccio/picow_day_clock/main/images/icons-on-display.jpeg)

If the LCD cannot be initialises, the clock prints diagnostics to `stdout` as a last resort.

### Timezones

The software takes into consideration daylight savings time, but currently calculates this based on the last Sunday of the month which is the approach taken in Europe including the United Kingdom. 

The default time is also set to UTC with daylight savings time, which is the configuration for the United Kingdom.

Should the clock be useful to others, and pull requests are always appreciated, the handling of timezones could be improved as well as the settings for daylight savings time.

## Configuration

Before CMake configuration, the Wi-Fi credentials must be set in `config.cmake` which is not part of the git repository. The project will fail to build without these. The configuration file is also a good place to set the domain name of the NTP server which otherwise defaults to `pool.ntp.org`. An example configuraion is:

``` cmake
set(WIFI_SSID "My SSID)
set(WIFI_PASSWORD "v3ry-s3cr3et")
set(NTP_SERVER "uk.pool.ntp.org")
```

## Build and Testing

The software is expected to build on Mac, Linux and Windows hosts and should be able to be imported into the Raspberry Pi Pico extension to Visual Studio Code. The Pico target build uses the standard Pico SDK tooling, but the native tests and coverage require LLVM for Clang and the coverage tools.

Tests for the core clock functionality and error handling is largely complete through some very basic mocking of the Pico SDK. The [code](tests/mock.c) for the mocking is by no means complete and is just the very minimum required to make the clock function and generate errors that might occur such as DNS timeouts.

The tests run on the host and assume Clang is available. Automated CI is provided by [GitHub runners](.github/workflows/build.yml).

## Debug

On boot, all LCDs are initialised first and diagnostic messages are displayed on the LCD1 as Wi-Fi and then NTP is initialised. On success, the clock starts. If any error is unrecoverable, signaled by a status message in red, the clock will not start and the diagnostics will remain. If the diagnostics do not appear at all, there is a problem with the LCD connections or the LCD driver. The following CMake configuration is available for debug to the UART:

* `CLOCK_DEBUG_ENABLED`: debug messages from the day of week clock itself
* `PICO_CYW43_ARCH_DEBUG_ENABLED`: SDK built-in debug of Wi-FI connections
* `LWIP_MALLOC_DEBUG`: SDK built-in memory allocation debug
* `LWIP_PROTOCOL_DEBUG`: SDK built-in UDP debug

## Pico pinout

The pinout for the LCD is defined in `config.h` using a series of C macros `LCD1_GPIO_xxx`. The backlight is shared across all of the LCDs as there is no need to control the dimming of the LCDs individually.

| Pin to device mapping   | Name      | Pin | Pico  | Pin | Name      | Pin to device mapping   |
|-------------------------|-----------|-----| ----- |-----|-----------|-------------------------|
| Debug Probe RX          | GP0       | 1   | XXXXX | 40  | VBUS 5V   |                         |
| Debug Probe TX          | GP1       | 2   | XXXXX | 39  | VSYS 5V   | DollaTek VIN            |
| Debug Probe GND         | GND       | 3   | XXXXX | 38  | GND       | LCD GND                 |
| LCD 3 data/command (DC) | GP2       | 4   | XXXXX | 37  | 3V3 En    |                         |
| LCD 3 chip select (CS)  | GP3       | 5   | XXXXX | 36  | 3V3 Out   |                         |
| LCD 4 data/command (DC) | GP4       | 6   | XXXXX | 35  | ADC VRef  |                         |
| LCD 4 chip select (CS)  | GP5       | 7   | XXXXX | 34  | GP28 A2   |                         |
|                         | GND       | 8   | XXXXX | 33  | ADC Gnd   |                         |
| LCD 1 data/command (DC) | GP6       | 9   | XXXXX | 32  | GP27 A1   |                         |
| LCD 1 chip select (CS)  | GP7       | 10  | XXXXX | 31  | GP26 A0   |                         |
| LCD 2 data/command (DC) | GP8       | 11  | XXXXX | 30  | RUN       |                         |
| LCD 2 chip select (CS)  | GP9       | 12  | XXXXX | 29  | GP22      |                         |
|                         | GND       | 13  | XXXXX | 28  | GND       |                         |
| Shared clock (CLK)      | GP10      | 14  | XXXXX | 27  | GP21      |                         |
| Shared MOSI (DIN)       | GP11      | 15  | XXXXX | 26  | GP20      |                         |
| Shared reset (RST)      | GP12      | 16  | XXXXX | 25  | GP19      | LCD 7 chip select (CS)  |
| Shared backlight (BL)   | GP13      | 17  | XXXXX | 24  | GP18      | LCD 7 data/command (DC) |
|                         | GND       | 18  | XXXXX | 23  | GND       |                         |
| LCD 5 data/command (DC) | GP14      | 19  | XXXXX | 22  | GP17      | LCD 6 chip select (CS)  |
| LCD 5 chip select (CS)  | GP15      | 20  | XXXXX | 21  | GP16      | LCD 6 data/command (DC) |

## Electrical Design

The LCD module expects a 3.3V supply and the [datasheet for the Waveshare 1.47inch LCD](https://files.waveshare.com/upload/9/99/1.47inch_LCD_Datasheet.pdf) indicates that the backlight draws 60-90mA. All the LCDs can therefore be driven by a single voltage regulator such as the DollaTek 1117:

* VIN: 5V VSYS from the USB power supply shared with the Pico
* VOUT: LCD VCC, shared across all LCD modules
* GND: any GND pin on the Pico

The BL backlight pin of the LCD module is not the actual LCD backlight; it is a control pin to the LCD module's TXB0108PWR level shifter so can also be shared across all LCDs and, like the other control signals, driven directly from the Pico.

![Clock schematic diagram](https://raw.githubusercontent.com/masaccio/picow_day_clock/main/images/schematic.jpeg)

A TXB0108 logic level shifter is used as a set of buffers to increase the available current ro drive CLK, DIN, RST and BL control signals as the Pico's GPIO is insufficient to drive 7 LCDs without significant signal degredation.

## Mechanical design

I chose the Waveshare LCD because it comes with a socket that has a wired plug with 2.54mm pin connectors for easy protoyping. For the final clock, this connector gets in the way of the wires hiding neatly behind the LCD. There are 172x320 LCD displays that are based on the ST7789 that come with an FPC connector which is neatly conencted on the narrow edge of the display. The bare LCD with the FPC connector has the disadvantage for the hobbyist that you need to add a ZIF socket and then break that out into something with a wider pitch to solder. The connector on the Waveshare board can be desoldered using solder braid and the connector carefully snipped off using flush snippers. The Waveshare module also has 2mm brass mounting holes which makes mounting the LCDs securely simpler. The main drawback of the approach is that the read of the LCD is quite messy, but the intended use is for a wall-mounted clock. The soldered connections look like this (please forgive the terrible soldering skills; I am out of practice):

![Soldered connections to LCD module](https://raw.githubusercontent.com/masaccio/picow_day_clock/main/images/module-connections.jpeg)

## Credits

GPIO, SPI and PCM driver code for the LCD and the framebuffer management is derived from the [Waveshare Sample Demo](https://www.waveshare.com/wiki/1.47inch_LCD_Module) code for the Waveshare 1.47inch LCD. The driver is [licensed](licenses/waveshare.txt) by the Waveshare team under an MIT license.

NTP driver code is derived from [pico_ntp_client](https://github.com/raspberrypi/pico-examples/tree/master/pico_w/wifi/ntp_client) in the [Raspberry Pi Pico SDK Examples](https://github.com/raspberrypi/pico-examples) which is [licensed](raspberry-pi.txt) under a BSD license.

Variable width fonts replace the fixed-width fonts in the Waveshare examples, but the C data structures are ultimately derived from work by STMicroelectronics [licensed](licenses/stm.txt) under the 3-clause BSD license.

The digit font is [Bebas Neue Regular](https://fonts.google.com/specimen/Bebas+Neue) which is [licensed](licenses/BebasNeue-OFL.txt) under the Open Font License.

The text font used in display diagnostics during boot is [Roboto Medium](https://fonts.google.com/specimen/Roboto) which is [licensed](licenses/Roboto-OFL.txt) under the Open Font License.

Icons in this project are derived from [Google Material Design Icons](https://github.com/google/material-design-icons), which are licensed under the [Apache License 2.0](https://github.com/google/material-design-icons/blob/master/LICENSE). The icons have been resized and converted to C bitmaps for use in this project, and new icons have been derived to addition status badges.
