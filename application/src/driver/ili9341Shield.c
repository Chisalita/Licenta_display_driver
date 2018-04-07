#include "ili9341Shield.h"

#include <stdio.h>
#include <unistd.h>
#include <pigpio.h>

#include "registers.h"



#define TFT_WIDTH 240
#define TFT_HEIGHT 320




rotation_t _rotation = ROTATION_0_DEGREES;
int _width = TFT_HEIGHT;
int _height = TFT_WIDTH;
int _reset = RESET_PIN;



rotation_t ili9341Shield_getDisplayRotation()
{
    return _rotation;
}

int ili9341Shield_getDisplayWidth()
{
    return _width;
}
int ili9341Shield_getDisplayHeight()
{
    return _height;
}


void writeRegister32(uint8_t r, uint32_t d)
{
    CS_ACTIVE;
    CD_COMMAND;
    write8(r);
    CD_DATA;
    usleep(10);
    write8(d >> 24);
    usleep(10);
    write8(d >> 16);
    usleep(10);
    write8(d >> 8);
    usleep(10);
    write8(d);
    CS_IDLE;
}

void setAddrWindow(int x1, int y1, int x2, int y2)
{
    CS_ACTIVE;
    uint32_t t;

    t = x1;
    t <<= 16;
    t |= x2;
    writeRegister32(ILI9341_COLADDRSET, t); // HX8357D uses same registers!
    t = y1;
    t <<= 16;
    t |= y2;
    writeRegister32(ILI9341_PAGEADDRSET, t); // HX8357D uses same registers!

    CS_IDLE;
}

void setRotation(rotation_t rotation)
{

    CS_ACTIVE;
    // MEME, HX8357D uses same registers as 9341 but different values
    uint16_t t;

    _rotation = rotation;
    switch (rotation)
    {
    case ROTATION_270_DEGREES: //270
        t = ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR;
        _width = TFT_WIDTH;
        _height = TFT_HEIGHT;
        break;
    case ROTATION_0_DEGREES: //0
        t = ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR;
        _width = TFT_HEIGHT;
        _height = TFT_WIDTH;
        break;
    case ROTATION_90_DEGREES: //90
        t = ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR;
        _width = TFT_WIDTH;
        _height = TFT_HEIGHT;
        break;
    case ROTATION_180_DEGREES: //180
        t = ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR;
        _width = TFT_HEIGHT;
        _height = TFT_WIDTH;
        break;
    }
    writeRegister8(ILI9341_MADCTL, t);
    // For 9341, init default full-screen address window:
    setAddrWindow(0, 0, _width - 1, _height - 1); // CS_IDLE happens here
}

void writeNoParamCommand(uint8_t value)
{
    CS_ACTIVE;
    CD_COMMAND;
    write8(value);
    CS_IDLE;
}

void displayOff()
{
    CS_ACTIVE;
    CD_COMMAND;
    write8(ILI9341_DISPLAYOFF);
    CS_IDLE;
}

void displayOn()
{
    CS_ACTIVE;
    CD_COMMAND;
    write8(ILI9341_DISPLAYON);
    CS_IDLE;
}

void reset(void)
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
    write8(0x00);
    uint8_t i = 0;
    for (; i < 3; i++)
        WR_STROBE; // Three extra 0x00s
    CS_IDLE;
}

void write8(uint8_t value)
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

    write8(0x0); //just set to 0

    reset();
    printf("reset\n");
    usleep(200 * 1000);

    // uint16_t a, d;
    CS_ACTIVE;
    writeRegister8(ILI9341_SOFTRESET, 0);
    // writeNoParamCommand(ILI9341_SOFTRESET);
    usleep(50 * 1000);
    //writeRegister8(ILI9341_DISPLAYOFF, 0);
    // writeNoParamCommand(ILI9341_DISPLAYOFF);

    writeRegister8(ILI9341_POWERCONTROL1, 0x23);
    writeRegister8(ILI9341_POWERCONTROL2, 0x10);
    writeRegister16(ILI9341_VCOMCONTROL1, 0x2B2B);
    writeRegister8(ILI9341_VCOMCONTROL2, 0xC0);
    writeRegister8(ILI9341_MADCTL, /*ILI9341_MADCTL_MY |*/ ILI9341_MADCTL_BGR); //here also do rotation
    writeRegister8(ILI9341_PIXELFORMAT, 0x55);
    writeRegister16(ILI9341_FRAMECONTROL, 0x001B);

    writeRegister8(ILI9341_ENTRYMODE, 0x07);
    // writeRegister8(ILI9341_SLEEPOUT, 0);
    writeNoParamCommand(ILI9341_SLEEPOUT);
    usleep(150 * 1000);
    // writeRegister8(ILI9341_DISPLAYON, 0);
    writeNoParamCommand(ILI9341_DISPLAYON);
    //init();
    usleep(500 * 1000);
    setRotation(ROTATION_0_DEGREES);

    // setAddrWindow(0, 0, _width - 1, _height - 1);
    CS_IDLE;
}
