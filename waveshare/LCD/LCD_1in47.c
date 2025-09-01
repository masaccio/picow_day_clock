// clang-format off

/*****************************************************************************
 * | File      	:   LCD_1in47.c
 * | Author      :   Waveshare team
 * | Function    :   Hardware underlying interface
 * | Info        :
 *                Used to shield the underlying layers of each master
 *                and enhance portability
 *----------------
 * |	This version:   V1.0
 * | Date        :   2022-03-08
 * | Info        :   Basic version
 *
 ******************************************************************************/
#include "LCD_1in47.h"
#include "DEV_Config.h"

#include <stdlib.h> //itoa()
#include <string.h> // memcpy() and memset()
#include <stdio.h>

LCD_1IN47_ATTRIBUTES LCD_1IN47;

/******************************************************************************
function :	Hardware reset
parameter:
******************************************************************************/
static void LCD_1IN47_Reset(LCD_GPIO_Config pins)
{
    DEV_Digital_Write(pins.RST, 1);
    DEV_Delay_ms(100);
    DEV_Digital_Write(pins.RST, 0);
    DEV_Delay_ms(100);
    DEV_Digital_Write(pins.RST, 1);
    DEV_Delay_ms(100);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
static void LCD_1IN47_SendCommand(LCD_GPIO_Config pins, UBYTE Reg)
{
    DEV_Digital_Write(pins.DC, 0);
    DEV_Digital_Write(pins.CS, 0);
    DEV_SPI_WriteByte(Reg);
    DEV_Digital_Write(pins.CS, 1);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void LCD_1IN47_SendData_8Bit(LCD_GPIO_Config pins, UBYTE Data)
{
    DEV_Digital_Write(pins.DC, 1);
    DEV_Digital_Write(pins.CS, 0);
    DEV_SPI_WriteByte(Data);
    DEV_Digital_Write(pins.CS, 1);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void LCD_1IN47_SendData_16Bit(LCD_GPIO_Config pins, UWORD Data)
{
    DEV_Digital_Write(pins.DC, 1);
    DEV_Digital_Write(pins.CS, 0);
    DEV_SPI_WriteByte((Data >> 8) & 0xFF);
    DEV_SPI_WriteByte(Data & 0xFF);
    DEV_Digital_Write(pins.CS, 1);
}

/******************************************************************************
function :	Initialize the lcd register
parameter:
******************************************************************************/
static void LCD_1IN47_InitReg(LCD_GPIO_Config pins)
{
    LCD_1IN47_SendCommand(pins, 0x11);
    DEV_Delay_ms(120);
    LCD_1IN47_SendCommand(pins, 0x36);
    if (HORIZONTAL )
        LCD_1IN47_SendData_8Bit(pins, 0x00);
    else
        LCD_1IN47_SendData_8Bit(pins, 0x70);

    LCD_1IN47_SendCommand(pins, 0x3A);
    LCD_1IN47_SendData_8Bit(pins, 0x05);

    LCD_1IN47_SendCommand(pins, 0xB2);
    LCD_1IN47_SendData_8Bit(pins, 0x0C);
    LCD_1IN47_SendData_8Bit(pins, 0x0C);
    LCD_1IN47_SendData_8Bit(pins, 0x00);
    LCD_1IN47_SendData_8Bit(pins, 0x33);
    LCD_1IN47_SendData_8Bit(pins, 0x33);

    LCD_1IN47_SendCommand(pins, 0xB7);
    LCD_1IN47_SendData_8Bit(pins, 0x35);

    LCD_1IN47_SendCommand(pins, 0xBB);
    LCD_1IN47_SendData_8Bit(pins, 0x35);

    LCD_1IN47_SendCommand(pins, 0xC0);
    LCD_1IN47_SendData_8Bit(pins, 0x2C);

    LCD_1IN47_SendCommand(pins, 0xC2);
    LCD_1IN47_SendData_8Bit(pins, 0x01);

    LCD_1IN47_SendCommand(pins, 0xC3);
    LCD_1IN47_SendData_8Bit(pins, 0x13);

    LCD_1IN47_SendCommand(pins, 0xC4);
    LCD_1IN47_SendData_8Bit(pins, 0x20);

    LCD_1IN47_SendCommand(pins, 0xC6);
    LCD_1IN47_SendData_8Bit(pins, 0x0F);

    LCD_1IN47_SendCommand(pins, 0xD0);
    LCD_1IN47_SendData_8Bit(pins, 0xA4);
    LCD_1IN47_SendData_8Bit(pins, 0xA1);

    LCD_1IN47_SendCommand(pins, 0xD6);
    LCD_1IN47_SendData_8Bit(pins, 0xA1);

    LCD_1IN47_SendCommand(pins, 0xE0);
    LCD_1IN47_SendData_8Bit(pins, 0xF0);
    LCD_1IN47_SendData_8Bit(pins, 0x00);
    LCD_1IN47_SendData_8Bit(pins, 0x04);
    LCD_1IN47_SendData_8Bit(pins, 0x04);
    LCD_1IN47_SendData_8Bit(pins, 0x04);
    LCD_1IN47_SendData_8Bit(pins, 0x05);
    LCD_1IN47_SendData_8Bit(pins, 0x29);
    LCD_1IN47_SendData_8Bit(pins, 0x33);
    LCD_1IN47_SendData_8Bit(pins, 0x3E);
    LCD_1IN47_SendData_8Bit(pins, 0x38);
    LCD_1IN47_SendData_8Bit(pins, 0x12);
    LCD_1IN47_SendData_8Bit(pins, 0x12);
    LCD_1IN47_SendData_8Bit(pins, 0x28);
    LCD_1IN47_SendData_8Bit(pins, 0x30);

    LCD_1IN47_SendCommand(pins, 0xE1);
    LCD_1IN47_SendData_8Bit(pins, 0xF0);
    LCD_1IN47_SendData_8Bit(pins, 0x07);
    LCD_1IN47_SendData_8Bit(pins, 0x0A);
    LCD_1IN47_SendData_8Bit(pins, 0x0D);
    LCD_1IN47_SendData_8Bit(pins, 0x0B);
    LCD_1IN47_SendData_8Bit(pins, 0x07);
    LCD_1IN47_SendData_8Bit(pins, 0x28);
    LCD_1IN47_SendData_8Bit(pins, 0x33);
    LCD_1IN47_SendData_8Bit(pins, 0x3E);
    LCD_1IN47_SendData_8Bit(pins, 0x36);
    LCD_1IN47_SendData_8Bit(pins, 0x14);
    LCD_1IN47_SendData_8Bit(pins, 0x14);
    LCD_1IN47_SendData_8Bit(pins, 0x29);
    LCD_1IN47_SendData_8Bit(pins, 0x32);

    LCD_1IN47_SendCommand(pins, 0x21);

    LCD_1IN47_SendCommand(pins, 0x11);
    DEV_Delay_ms(120);
    LCD_1IN47_SendCommand(pins, 0x29);
}

/********************************************************************************
function:	Set the resolution and scanning method of the screen
parameter:
        Scan_dir:   Scan direction
********************************************************************************/
static void LCD_1IN47_SetAttributes(LCD_GPIO_Config pins, UBYTE Scan_dir)
{
    // Get the screen scan direction
    LCD_1IN47.SCAN_DIR = Scan_dir;
    UBYTE MemoryAccessReg = 0x00;

    // Get GRAM and LCD width and height
    if (Scan_dir == HORIZONTAL)
    {
        LCD_1IN47.HEIGHT = LCD_1IN47_WIDTH;
        LCD_1IN47.WIDTH = LCD_1IN47_HEIGHT;
        MemoryAccessReg = 0X78;
    }
    else
    {
        LCD_1IN47.HEIGHT = LCD_1IN47_HEIGHT;
        LCD_1IN47.WIDTH = LCD_1IN47_WIDTH;
        MemoryAccessReg = 0X00;
    }

    // Set the read / write scan direction of the frame memory
    LCD_1IN47_SendCommand(pins, 0x36);              // MX, MY, RGB mode
    LCD_1IN47_SendData_8Bit(pins, MemoryAccessReg); // 0x08 set RGB
}

/********************************************************************************
function :	Initialize the lcd
parameter:
********************************************************************************/
void LCD_1IN47_Init(LCD_GPIO_Config pins, UBYTE Scan_dir)
{
    DEV_SET_PWM(90);
    // Hardware reset
    LCD_1IN47_Reset(pins);

    // Set the resolution and scanning method of the screen
    LCD_1IN47_SetAttributes(pins, Scan_dir);

    // Set the initialization register
    LCD_1IN47_InitReg(pins);
}

/********************************************************************************
function:	Sets the start position and size of the display area
parameter:
        Xstart 	:   X direction Start coordinates
        Ystart  :   Y direction Start coordinates
        Xend    :   X direction end coordinates
        Yend    :   Y direction end coordinates
********************************************************************************/
void LCD_1IN47_SetWindows(LCD_GPIO_Config pins, UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend)
{
    if (HORIZONTAL)
    { // set the X coordinates
        LCD_1IN47_SendCommand(pins, 0x2A);
        LCD_1IN47_SendData_8Bit(pins, 0x00);
        LCD_1IN47_SendData_8Bit(pins, 0x22);
        LCD_1IN47_SendData_8Bit(pins, 0x00);
        LCD_1IN47_SendData_8Bit(pins, 0xcd);

        // set the Y coordinates
        LCD_1IN47_SendCommand(pins, 0x2B);
        LCD_1IN47_SendData_8Bit(pins, 0x00);
        LCD_1IN47_SendData_8Bit(pins, 0x00);
        LCD_1IN47_SendData_8Bit(pins, 0x01);
        LCD_1IN47_SendData_8Bit(pins, 0x3f);
    }
    else
    {
        // set the X coordinates
        LCD_1IN47_SendCommand(pins, 0x2A);
        LCD_1IN47_SendData_8Bit(pins, 0x00);
        LCD_1IN47_SendData_8Bit(pins, 0x00);
        LCD_1IN47_SendData_8Bit(pins, 0x01);
        LCD_1IN47_SendData_8Bit(pins, 0x3f);

        // set the Y coordinates
        LCD_1IN47_SendCommand(pins, 0x2B);
        LCD_1IN47_SendData_8Bit(pins, 0x00);
        LCD_1IN47_SendData_8Bit(pins, 0x22);
        LCD_1IN47_SendData_8Bit(pins, 0x00);
        LCD_1IN47_SendData_8Bit(pins, 0xcd);
        
    }

    LCD_1IN47_SendCommand(pins, 0x2C);
    // printf("%d %d\r\n",x,y);
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void LCD_1IN47_Clear(LCD_GPIO_Config pins, UWORD Color)
{
    UWORD j, i;
    UWORD Image[LCD_1IN47.WIDTH * LCD_1IN47.HEIGHT];

    Color = ((Color << 8) & 0xff00) | (Color >> 8);

    for (j = 0; j < LCD_1IN47.HEIGHT * LCD_1IN47.WIDTH; j++)
    {
        Image[j] = Color;
    }

    LCD_1IN47_SetWindows(pins, 0, 0, LCD_1IN47.WIDTH, LCD_1IN47.HEIGHT);
    DEV_Digital_Write(pins.DC, 1);
    DEV_Digital_Write(pins.CS, 0);
    // printf("HEIGHT %d, WIDTH %d\r\n",LCD_1IN47.HEIGHT,LCD_1IN47.WIDTH);
    for (j = 0; j < LCD_1IN47.HEIGHT; j++)
    {
        DEV_SPI_Write_nByte((uint8_t *)&Image[j * LCD_1IN47.WIDTH], LCD_1IN47.WIDTH * 2);
    }
    DEV_Digital_Write(pins.CS, 1);
}

/******************************************************************************
function :	Sends the image buffer in RAM to displays
parameter:
******************************************************************************/
void LCD_1IN47_Display(LCD_GPIO_Config pins, UWORD *Image)
{
    UWORD j;
    LCD_1IN47_SetWindows(pins, 0, 0, LCD_1IN47.WIDTH, LCD_1IN47.HEIGHT);
    DEV_Digital_Write(pins.DC, 1);
    DEV_Digital_Write(pins.CS, 0);
    for (j = 0; j < LCD_1IN47.HEIGHT; j++)
    {
        DEV_SPI_Write_nByte((uint8_t *)&Image[j * LCD_1IN47.WIDTH], LCD_1IN47.WIDTH * 2);
    }
    DEV_Digital_Write(pins.CS, 1);
    LCD_1IN47_SendCommand(pins, 0x29);
}

/******************************************************************************
function : Sends the image buffer in RAM to display, expanding each pixel according to Paint.Scale
parameter:
    Paint.Scale: 2=1bpp, 4=2bpp, 16=4bpp, 65=16bpp
    Image: pointer to source buffer
******************************************************************************/
void LCD_1IN47_Display_Scaled(LCD_GPIO_Config pins, void *Image, int scale)
{
    UWORD j, x;
    LCD_1IN47_SetWindows(pins, 0, 0, LCD_1IN47.WIDTH, LCD_1IN47.HEIGHT);
    DEV_Digital_Write(pins.DC, 1);
    DEV_Digital_Write(pins.CS, 0);
    
    // Temporary buffer for one line, always 16bpp output
    uint8_t linebuf[LCD_1IN47.WIDTH * 2];
    
    for (j = 0; j < LCD_1IN47.HEIGHT; j++) {
        if (scale == 65) {
            // 16bpp, just copy
            memcpy(linebuf, (uint8_t *)Image + (j * LCD_1IN47.WIDTH * 2), LCD_1IN47.WIDTH * 2);
        } else if (scale == 2) {
            // 1bpp, each bit is a pixel
            uint8_t *src = (uint8_t *)Image + (j * ((LCD_1IN47.WIDTH + 7) / 8));
            for (x = 0; x < LCD_1IN47.WIDTH; x++) {
                int byte = x / 8;
                int bit = 7 - (x % 8);
                uint16_t color = (src[byte] & (1 << bit)) ? 0xFFFF : 0x0000;
                linebuf[x * 2] = (color >> 8) & 0xFF;
                linebuf[x * 2 + 1] = color & 0xFF;
            }
        } else if (scale == 4) {
            // 2bpp, each 2 bits is a pixel
            uint8_t *src = (uint8_t *)Image + (j * ((LCD_1IN47.WIDTH + 3) / 4));
            for (x = 0; x < LCD_1IN47.WIDTH; x++) {
                int byte = x / 4;
                int shift = 6 - 2 * (x % 4);
                uint8_t val = (src[byte] >> shift) & 0x3;
                uint16_t color;
                switch (val) {
                    case 0: color = 0x0000; break;
                    case 1: color = 0x5555; break;
                    case 2: color = 0xAAAA; break;
                    case 3: color = 0xFFFF; break;
                }
                linebuf[x * 2] = (color >> 8) & 0xFF;
                linebuf[x * 2 + 1] = color & 0xFF;
            }
        } else if (scale == 16) {
            // 4bpp, each 4 bits is a pixel
            uint8_t *src = (uint8_t *)Image + (j * ((LCD_1IN47.WIDTH + 1) / 2));
            for (x = 0; x < LCD_1IN47.WIDTH; x++) {
                int byte = x / 2;
                int shift = 4 * (1 - (x % 2));
                uint8_t val = (src[byte] >> shift) & 0xF;
                // Map 16 levels to 16 colors (simple grayscale)
                uint16_t color = (val << 12) | (val << 8) | (val << 4) | val;
                linebuf[x * 2] = (color >> 8) & 0xFF;
                linebuf[x * 2 + 1] = color & 0xFF;
            }
        } else {
            // Unknown scale, fill black
            memset(linebuf, 0, LCD_1IN47.WIDTH * 2);
        }
        DEV_SPI_Write_nByte(linebuf, LCD_1IN47.WIDTH * 2);
    }
    DEV_Digital_Write(pins.CS, 1);
    LCD_1IN47_SendCommand(pins, 0x29);
}

void LCD_1IN47_DisplayWindows(LCD_GPIO_Config pins, UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD *Image)
{
    // display
    UDOUBLE Addr = 0;

    UWORD j;
    LCD_1IN47_SetWindows(pins, Xstart, Ystart, Xend, Yend);
    DEV_Digital_Write(pins.DC, 1);
    DEV_Digital_Write(pins.CS, 0);
    for (j = Ystart; j < Yend - 1; j++)
    {
        Addr = Xstart + j * LCD_1IN47.WIDTH;
        DEV_SPI_Write_nByte((uint8_t *)&Image[Addr], (Xend - Xstart) * 2);
    }
    DEV_Digital_Write(pins.CS, 1);
}

void LCD_1IN47_DisplayPoint(LCD_GPIO_Config pins, UWORD X, UWORD Y, UWORD Color)
{
    LCD_1IN47_SetWindows(pins, X, Y, X, Y);
    LCD_1IN47_SendData_16Bit(pins, Color);
}

void Handler_1IN47_LCD(int signo)
{
    // System Exit
    printf("\r\nHandler:Program stop\r\n");
    DEV_Module_Exit();
    exit(0);
}
