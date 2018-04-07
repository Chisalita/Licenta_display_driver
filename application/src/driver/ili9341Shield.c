#include "ili9341Shield.h"

#include <stdio.h>
#include <unistd.h>
#include <pigpio.h>

#include "registers.h"


static int _reset = RESET_PIN;




void ili9341Shield_writeRegister32(uint8_t r, uint32_t d)
{
    CS_ACTIVE;
    CD_COMMAND;
    ili9341Shield_write8(r);
    CD_DATA;
    usleep(10);
    ili9341Shield_write8(d >> 24);
    usleep(10);
    ili9341Shield_write8(d >> 16);
    usleep(10);
    ili9341Shield_write8(d >> 8);
    usleep(10);
    ili9341Shield_write8(d);
    CS_IDLE;
}

void ili9341Shield_setAddrWindow(int x1, int y1, int x2, int y2)
{
    CS_ACTIVE;
    uint32_t t;

    t = x1;
    t <<= 16;
    t |= x2;
    ili9341Shield_writeRegister32(ILI9341_COLADDRSET, t); // HX8357D uses same registers!
    t = y1;
    t <<= 16;
    t |= y2;
    ili9341Shield_writeRegister32(ILI9341_PAGEADDRSET, t); // HX8357D uses same registers!

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

    if (_reset)
    {
        gpioWrite(_reset, 0);
        usleep(2 * 1000);
        gpioWrite(_reset, 1);
    }

    // Data transfer sync
    CS_ACTIVE;
    CD_COMMAND;
    ili9341Shield_write8(0x00);
    uint8_t i = 0;
    for (; i < 3; i++)
        WR_STROBE; // Three extra 0x00s
    CS_IDLE;
}

void ili9341Shield_write8(uint8_t value)
{

    //TODO: make generic the mask
    uint32_t mask = 0xFF00000; //0x3FC00; //for bits DATA_PINS_OFFSET to DATA_PINS_OFFSET+7
    uint32_t data = ((uint32_t)value) << DATA_PINS_OFFSET;
    //  printf("set = 0x%X\n", data);
    //  printf("clear = 0x%X\n", (data ^ mask));
    gpioWrite_Bits_0_31_Set(data);
    gpioWrite_Bits_0_31_Clear(data ^ mask);
    //TODO: see if strobe should be on the extremity (before and after)
    WR_STROBE;
}

void ili9341Shield_init()
{
    gpioSetMode(RESET_PIN, PI_OUTPUT);
    gpioSetMode(RD_PIN, PI_OUTPUT);
    gpioSetMode(WR_PIN, PI_OUTPUT);
    gpioSetMode(CD_PIN, PI_OUTPUT);
    gpioSetMode(CS_PIN, PI_OUTPUT);
    //set data to output
    int i = DATA_PINS_OFFSET;
    for (; i < DATA_PINS_OFFSET + 8; ++i)
    {
        gpioSetMode(i, PI_OUTPUT);
    }

    ili9341Shield_write8(0x0); //just set to 0

    ili9341Shield_reset();
    printf("reset\n");
    usleep(200 * 1000);

    // uint16_t a, d;
    CS_ACTIVE;
    ili9341Shield_writeRegister8(ILI9341_SOFTRESET, 0);
    // ili9341Shield_writeNoParamCommand(ILI9341_SOFTRESET);
    usleep(50 * 1000);
    //ili9341Shield_writeRegister8(ILI9341_DISPLAYOFF, 0);
    // ili9341Shield_writeNoParamCommand(ILI9341_DISPLAYOFF);

    ili9341Shield_writeRegister8(ILI9341_POWERCONTROL1, 0x23);
    ili9341Shield_writeRegister8(ILI9341_POWERCONTROL2, 0x10);
    ili9341Shield_writeRegister16(ILI9341_VCOMCONTROL1, 0x2B2B);
    ili9341Shield_writeRegister8(ILI9341_VCOMCONTROL2, 0xC0);
    ili9341Shield_writeRegister8(ILI9341_MADCTL, /*ILI9341_MADCTL_MY |*/ ILI9341_MADCTL_BGR); //here also do rotation
    ili9341Shield_writeRegister8(ILI9341_PIXELFORMAT, 0x55);
    ili9341Shield_writeRegister16(ILI9341_FRAMECONTROL, 0x001B);

    ili9341Shield_writeRegister8(ILI9341_ENTRYMODE, 0x07);
    // ili9341Shield_writeRegister8(ILI9341_SLEEPOUT, 0);
    ili9341Shield_writeNoParamCommand(ILI9341_SLEEPOUT);
    usleep(150 * 1000);
    // ili9341Shield_writeRegister8(ILI9341_DISPLAYON, 0);
    ili9341Shield_writeNoParamCommand(ILI9341_DISPLAYON);
    //init();
    usleep(500 * 1000);



    // setRotation(ROTATION_0_DEGREES); //TODO: this is important, but now it generates compile errors!!!

    // ili9341Shield_setAddrWindow(0, 0, _width - 1, _height - 1);
    CS_IDLE;
}
