// clang-format off

/*****************************************************************************
* | File      	:   DEV_Config.c
* | Author      :   
* | Function    :   Hardware underlying interface
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2021-03-16
* | Info        :   
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of theex Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
******************************************************************************/
#include "DEV_Config.h"
#include "Debug.h"

#define SPI_PORT spi1

uint slice_num;
/**
 * GPIO read and write
**/
void DEV_Digital_Write(UWORD Pin, UBYTE Value)
{
    gpio_put(Pin, Value);
}

UBYTE DEV_Digital_Read(UWORD Pin)
{
    return gpio_get(Pin);
}

/**
 * SPI
**/
void DEV_SPI_WriteByte(uint8_t Value)
{
    spi_write_blocking(SPI_PORT, &Value, 1);
}

void DEV_SPI_Write_nByte(uint8_t pData[], uint32_t Len)
{
    spi_write_blocking(SPI_PORT, pData, Len);
}


/**
 * GPIO Mode
**/
void DEV_GPIO_Mode(UWORD Pin, UWORD Mode)
{
    gpio_init(Pin);
    if(Mode == 0 || Mode == GPIO_IN) {
        gpio_set_dir(Pin, GPIO_IN);
    } else {
        gpio_set_dir(Pin, GPIO_OUT);
    }
}


/**
 * delay x ms
**/
void DEV_Delay_ms(UDOUBLE xms)
{
    sleep_ms(xms);
}

void DEV_Delay_us(UDOUBLE xus)
{
    sleep_us(xus);
}



void DEV_GPIO_Init(LCD_GPIO_Config pins)
{
    DEV_GPIO_Mode(pins.RST, 1);
    DEV_GPIO_Mode(pins.DC, 1);
    DEV_GPIO_Mode(pins.CS, 1);
    DEV_GPIO_Mode(pins.BL, 1);
    DEV_GPIO_Mode(25, 1);


    DEV_GPIO_Mode(pins.CS, 1);
    DEV_GPIO_Mode(pins.BL, 1);

    DEV_Digital_Write(pins.CS, 1);
    DEV_Digital_Write(pins.DC, 0);
    DEV_Digital_Write(pins.BL, 1);
    DEV_Digital_Write(25, 0);
}
/******************************************************************************
function:	Module Initialize, the library and initialize the pins, SPI protocol
parameter:
Info:
******************************************************************************/
UBYTE DEV_Module_Init(LCD_GPIO_Config pins)
{
    stdio_init_all();   
    // SPI Config
    spi_init(SPI_PORT, 10000 * 1000);
    gpio_set_function(pins.CLK, GPIO_FUNC_SPI);
    gpio_set_function(pins.MOSI, GPIO_FUNC_SPI);

    // GPIO Config
    DEV_GPIO_Init(pins);
    
    // PWM Config
    gpio_set_function(pins.BL, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(pins.BL);
    pwm_set_wrap(slice_num, 100);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 1);
    pwm_set_clkdiv(slice_num,50);
    pwm_set_enabled(slice_num, true);

    Debug("DEV_Module_Init OK\r\n");
    return 0;
}

void DEV_SET_PWM(uint8_t Value){
    if(Value<0 || Value >100){
        printf("DEV_SET_PWM Error \r\n");
    }else {
        pwm_set_chan_level(slice_num, PWM_CHAN_B, Value);
    }
        
    
    
}

/******************************************************************************
function:	Module exits, closes SPI and BCM2835 library
parameter:
Info:
******************************************************************************/
void DEV_Module_Exit(void)
{

}
