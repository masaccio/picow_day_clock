# Waveshare 1.47inch LCD Module

The files here are copied from the [Waveshare Wiki](https://www.waveshare.com/wiki/1.47inch_LCD_Module) for the 1.47 inch LCD Module which is based off the ST7789V3 chipset.

These files are unmodified from the originals and are provided for convenience.

## Pin connection

The default GPIO configuration is assumed, as defined in `lib/Config/DEV_Config.h`:

| LCD Pin | GPIO    |
|---------|---------|
| VCC     | VSYS    |
| GND     | GND     |
| DIN     | 11      |
| CLK     | 10      |
| CS      | 9       |
| DC      | 8       |
| RST     | 12      |
| BL      | 13      |

## Pico pinout

| UART     | I2C       | SPI      | Name | Pin | Pin | Name      | SPI        | I2C        | UART       |
| ---------|-----------|----------|------|-----|-----|-----------|------------|------------|------------|
| UART0 TX | I2C0 SDA  | SPI0 RX  | GP0  | 1   | 40  | VBUS 5V   |            |            |            |
| UART0 RX | I2C0 SCL  | SPI0 CSn | GP1  | 2   | 39  | VSYS 5V   |            |            |            |
|          |           |          | GND  | 3   | 38  | GND       |            |            |            |
|          | I2C1 SDA  | SPI0 SCK | GP2  | 4   | 37  | 3V3 En    |            |            |            |
|          | I2C1 SCL  | SPI0 TX  | GP3  | 5   | 36  | 3V3 Out   |            |            |            |
| UART1 TX | I2C0 SDA  | SPI0 RX  | GP4  | 6   | 35  | ADC VRef  |            |            |            |
| UART1 RX | I2C0 SCL  | SPI0 CSn | GP5  | 7   | 34  | GP28 A2   | SPI1 RX    |            |            |
|          |           |          | GND  | 8   | 33  | ADC Gnd   |            |            |            |
|          | I2C1 SDA  | SPI0 SCK | GP6  | 9   | 32  | GP27 A1   | SPI1 TX    | I2C1 SCL   |            |
|          | I2C1 SCL  | SPI0 TX  | GP7  | 10  | 31  | GP26 A0   | SPI1 SCK   | I2C1 SDA   |            |
| UART1 TX | I2C0 SDA  | SPI1 RX  | GP8  | 11  | 30  | RUN       |            |            |            |
| UART1 RX | I2C0 SCL  | SPI1 CSn | GP9  | 12  | 29  | GP22      |            |            |            |
|          |           |          | GND  | 13  | 28  | GND       |            |            |            |
|          | I2C1 SDA  | SPI1 SCK | GP10 | 14  | 27  | GP21      | SPI0 CSn   | I2C0 SCL   | UART1 RX   |
|          | I2C1 SCL  | SPI1 TX  | GP11 | 15  | 26  | GP20      | SPI0 RX    | I2C0 SDA   | UART1 TX   |
| UART0 TX | I2C0 SDA  | SPI1 RX  | GP12 | 16  | 25  | GP19      | SPI0 TX    | I2C1 SCL   |            |
| UART0 RX | I2C0 SCL  | SPI1 CSn | GP13 | 17  | 24  | GP18      | SPI0 SCK   | I2C1 SDA   |            |
|          |           |          | GND  | 18  | 23  | GND       |            |            |            |
|          | I2C1 SDA  | SPI1 SCK | GP14 | 19  | 22  | GP17      | SPI0 CSn   | I2C0 SCL   | UART0 RX   |
|          | I2C1 SCL  | SPI1 TX  | GP15 | 20  | 21  | GP16      | SPI0 RX    | I2C0 SDA   | UART0 TX   |
