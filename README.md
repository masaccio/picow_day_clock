# Raspberry Pi Pico Day of Week Clock

This project is the software, hardware design and clock enclosure design for an LCD clock driven by the Raspberry Pi Pico 2 W. The clock design is inspired by the idea of a [Nixie](https://en.wikipedia.org/wiki/Nixie_tube) clock but uses LCDs so that the display is brighter and clearly visible in daylight.

The project is *not* compatible with earlier versions of the Pico and requires the CYW43439 Wi-Fi chip present in the Pico 2 W to function correctly.

## Software Operation

The pin-out and Wi-Fi settings are hard-coded into the project at buuld time. The credentials for Wi-Fi must be created in `secrets.cmake` and the project will fail to build without these. After the Pico firmware boots, the software will:

* Initialise all 7 LCD modules writing to the UART and exiting if this fails
* Create a Wi-Fi connection to the hard-coded Wi-Fi networt
* Get the current time from public NTP servers
* Display the abbreviated day of week on the first three LCDs and the current time in hours in minutes on the remaining 4 displays

The software takes into consideration daylight savings time, but currently calculates this based on the last Sunday of the month which is the approach taken in Europe including the United Kingdom. The default time is also set to GMT and no support for timezones is included. It would be trivial to assign some configuration parameters for the default timezone and whether to apply DST shifts but this is future work.

NTP syncs start at once per day and respect back-off requests from the server if this is too frequent. The timer itself runs once per second and any NTP drift that occurs daily is compensated for in these timer callbacks.

## Build and Testing

The software is expected to build on Mac, Linux and Windows hosts and should be able to be imported into the Raspberry Pi Pico extension to Visual Studio Code.

Tests for the core clock functionality and error handling is largely complete through some very basic mocking of the Pico SDK. The [code](src/tests/mock.c) for the mocking is by no means complete and is just the very minimum required to make the clock function and generate errors that might occur such as DNS timeouts.

The tests run on the host and assume Clang is available. Automated CI is provided by [GitHub runners](.github/workflows/build.yml).

## Pico pinout

The pinout for the LCD is defined in `clock.h` using a series of C macros `LCD1_GPIO_xxx`. The backlight is shared across all of the LCDs as there is no need to control the dimming of the LCDs individually.

| Pin to device mapping   | Name      | Pin | Pico  | Pin | Name      | Pin to device mapping   |
|-------------------------|-----------|-----| ----- |-----|-----------|-------------------------|
| Debug Probe RX          | GP0       | 1   | XXXXX | 40  | VBUS 5V   |                         |
| Debug Probe TX          | GP1       | 2   | XXXXX | 39  | VSYS 5V   |                         |
| Debug Probe GND         | GND       | 3   | XXXXX | 38  | GND       | LCD GND                 |
| LCD 3 data/command (DC) | GP2       | 4   | XXXXX | 37  | 3V3 En    |                         |
| LCD 3 chip select (CS)  | GP3       | 5   | XXXXX | 36  | 3V3 Out   | DollaTek VIN            |
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

![Clock schematic diagram](https://raw.githubusercontent.com/masaccio/picow_day_clock/images/schematic.jpg)

## TODO List

1. Look at whether the scan across the display can be eliminated
2. Move the LCD code away from a frame-buffer to direct SPI calls

## Credits

GPIO, SPI and PCM driver code for the LCD and the framebuffer management is derived from the [Waveshare Sample Demo](https://www.waveshare.com/wiki/1.47inch_LCD_Module) code for the Waveshare 1.47inch LCD. The driver [is licensed](licenses/waveshare.txt) by the Waveshare team under an MIT license.

Variable width fonts replace the fixed width fonts in the Waveshare examples, but the C data structures are ultimately derived from work by [STMicroelectronics](licenses/stm.txt) licensed under the 3-clause BSD license.

The digit font is [Bebas Neue Regular](https://fonts.google.com/specimen/Bebas+Neue) which is licensed under the [Open Font License](licenses/BebasNeue-OFL.txt).

The text font used in display diagnostics during boot is [Roboto Medium](https://fonts.google.com/specimen/Roboto) which is licensed under the [Open Font License](licenses/Roboto-OFL.txt).