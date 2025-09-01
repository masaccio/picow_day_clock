# Waveshare 1.47inch LCD Module

The files here are copied from the [Waveshare Wiki](https://www.waveshare.com/wiki/1.47inch_LCD_Module) for the 1.47 inch LCD Module which is based off the ST7789V3 chipset.

These files are unmodified from the originals and are provided for convenience.

## Pico pinout

| Pin to device mapping   | Name      | Pin | Pico  | Pin | Name      | Pin to device mapping  |
|-------------------------|-----------|-----| ----- |-----|-----------|------------------------|
| Debug Probe RX          | GP0       | 1   | XXXXX | 40  | VBUS 5V   |                        |
| Debug Probe TX          | GP1       | 2   | XXXXX | 39  | VSYS 5V   |                        |
| Debug Probe GND         | GND       | 3   | XXXXX | 38  | GND       | LCD GND                |
|                         | GP2       | 4   | XXXXX | 37  | 3V3 En    |                        |
|                         | GP3       | 5   | XXXXX | 36  | 3V3 Out   | LCD VCC                |
|                         | GP4       | 6   | XXXXX | 35  | ADC VRef  |                        |
|                         | GP5       | 7   | XXXXX | 34  | GP28 A2   |                        |
|                         | GND       | 8   | XXXXX | 33  | ADC Gnd   |                        |
| LCD 1 data/command (DC) | GP6       | 9   | XXXXX | 32  | GP27 A1   |                        |
| LCD 1 chip select (CS)  | GP7       | 10  | XXXXX | 31  | GP26 A0   |                        |
| LCD 2 data/command (DC) | GP8       | 11  | XXXXX | 30  | RUN       |                        |
| LCD 2 chip select (CS)  | GP9       | 12  | XXXXX | 29  | GP22      |                        |
|                         | GND       | 13  | XXXXX | 28  | GND       |                        |
| Shared clock (CLK)      | GP10      | 14  | XXXXX | 27  | GP21      |                        |
| Shared MOSI (DIN)       | GP11      | 15  | XXXXX | 26  | GP20      |                        |
| Shared reset (RST)      | GP12      | 16  | XXXXX | 25  | GP19      |                        |
| Shared backlight (BL)   | GP13      | 17  | XXXXX | 24  | GP18      |                        |
|                         | GND       | 18  | XXXXX | 23  | GND       |                        |
|                         | GP14      | 19  | XXXXX | 22  | GP17      |                        |
|                         | GP15      | 20  | XXXXX | 21  | GP16      |                        |

## LCD connections

The default GPIO configuration is assumed, as defined in `lib/Config/DEV_Config.h`:

| Pico    | LCD 1 | LCD 2 |
|---------|-------|-------|
| 3V3 Out | VCC   | VCC   |
| GND     | GND   | GND   |
| GP11    | DIN   | DIN   |
| GP10    | CLK   | CLK   |
| GP7     | CS    |       |
| GP6     | DC    |       |
| GP9     |       | CS    |
| GP8     |       | DC    |
| GP12    | RST   | RST   |
| GP13    | BL    | BL    |
