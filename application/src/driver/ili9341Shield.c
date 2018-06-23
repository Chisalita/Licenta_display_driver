#include "ili9341Shield.h"

#include <stdio.h>
#include <unistd.h>
#ifdef USING_PIGPIO_LIB
#include <pigpio.h>
#else
#include <bcm2835.h>
#endif
#include "registers.h"

#define RESET_TIME_US (15)
#define SOFT_RESET_TIME_US (6E3)
#define SLEEP_OUT_TIME_US (6E3)


void ili9341Shield_writeRegister32(uint8_t r, uint32_t d)
{
    CS_ACTIVE;
    CD_COMMAND;
    ili9341Shield_write8(r);
    CD_DATA;
    ili9341Shield_write8(d >> 24);
    ili9341Shield_write8(d >> 16);
    ili9341Shield_write8(d >> 8);
    ili9341Shield_write8(d);
    CS_IDLE;
}

void ili9341Shield_setAddrWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{

    uint32_t t = ((uint32_t)x1) << 16;
    t |= x2;

    CS_ACTIVE;
    ili9341Shield_writeRegister32(ILI9341_COLADDRSET, t);
    t = ((uint32_t)y1) << 16;
    t |= y2;
    ili9341Shield_writeRegister32(ILI9341_PAGEADDRSET, t);
    CS_IDLE;
}

void ili9341Shield_writeNoParamCommand(uint8_t value)
{
    CS_ACTIVE;
    CD_COMMAND;
    ili9341Shield_write8(value);
    CS_IDLE;
}

void ili9341Shield_reset(void)
{

    CS_IDLE;
    //  CD_DATA;
    WR_IDLE;
    RD_IDLE;

    GPIO_WRITE(RESET_PIN, 0);
    usleep(RESET_TIME_US);
    GPIO_WRITE(RESET_PIN, 1);

    // Data transfer sync
    CS_ACTIVE;
    CD_COMMAND;
    ili9341Shield_write8(0x00);
    uint8_t i = 0;
    for (; i < 3; i++)
        WR_STROBE; // Three extra 0x00s
    CS_IDLE;
}

__attribute__((always_inline)) inline void ili9341Shield_write8(uint8_t value)
{
    uint32_t data = ((uint32_t)value) << DATA_PINS_OFFSET;
    GPIO_SET_MULTI(data);
    GPIO_CLEAR_MULTI(data ^ DATA_PINS_MASK);
    WR_STROBE;
}

void ili9341Shield_init(void)
{
    GPIO_SET_MODE(RESET_PIN, GPIO_OUTPUT);
    GPIO_SET_MODE(RD_PIN, GPIO_OUTPUT);
    GPIO_SET_MODE(WR_PIN, GPIO_OUTPUT);
    GPIO_SET_MODE(CD_PIN, GPIO_OUTPUT);
    GPIO_SET_MODE(CS_PIN, GPIO_OUTPUT);
    //set data to output
    int i = DATA_PINS_OFFSET;
    for (; i < DATA_PINS_OFFSET + 8; ++i)
    {
        GPIO_SET_MODE(i, GPIO_OUTPUT);
    }

    ili9341Shield_write8(0x0); //just set data pins to 0

    ili9341Shield_reset();

    CS_ACTIVE;
    ili9341Shield_writeRegister8(ILI9341_SOFTRESET, 0);
    usleep(SOFT_RESET_TIME_US);

    ili9341Shield_writeRegister8(ILI9341_POWERCONTROL1, 0x23);
    ili9341Shield_writeRegister8(ILI9341_POWERCONTROL2, 0x10);
    ili9341Shield_writeRegister16(ILI9341_VCOMCONTROL1, 0x2B2B);
    ili9341Shield_writeRegister8(ILI9341_VCOMCONTROL2, 0xC0);
    ili9341Shield_writeRegister8(ILI9341_MADCTL, ILI9341_MADCTL_BGR); 
    ili9341Shield_writeRegister8(ILI9341_PIXELFORMAT, 0x55);
    ili9341Shield_writeRegister16(ILI9341_FRAMECONTROL, 0x001B);

    ili9341Shield_writeRegister8(ILI9341_ENTRYMODE, 0x07);
    ili9341Shield_writeNoParamCommand(ILI9341_SLEEPOUT);
    usleep(SLEEP_OUT_TIME_US);
    ili9341Shield_writeNoParamCommand(ILI9341_DISPLAYON);

    CS_IDLE;
}
