#include "display.h"
#ifdef USING_PIGPIO_LIB
#include <pigpio.h>
#else
#include <bcm2835.h>
#endif
#include "registers.h"
#include "ili9341Shield.h"
#include "frameBuffer.h"

#include <stdio.h>

#define TFT_WIDTH (240)
#define TFT_HEIGHT (320)
#define NO_OF_FRAME_BUFFERS (2)

static void flood(uint16_t color, uint32_t len);
static void pushColors(uint16_t *data, uint32_t len);
static uint16_t getStartLineToDraw(void);
static uint16_t getEndLineToDraw(uint16_t lastLine);
#if 0
static uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
#endif

static rotation_t _rotation = ROTATION_0_DEGREES;
static int _width = TFT_HEIGHT; 
static int _height = TFT_WIDTH;
static uint16_t __attribute__((aligned(32))) frameBuffer[NO_OF_FRAME_BUFFERS][TFT_WIDTH * TFT_HEIGHT] = {{0}, {0}};

int display_drawFrameBuffer(void)
{
    frameBuffer_getFrame(&frameBuffer[0][0]);
    ili9341Shield_setAddrWindow(0, 0, _width, _height);
    pushColors(&frameBuffer[0][0], (TFT_WIDTH * TFT_HEIGHT));
    return 0;
}

int display_drawFrameBufferOptimised(void)
{

    static int frameBufferIndex = 0;

    int status = frameBuffer_getFrame(&frameBuffer[frameBufferIndex][0]);

    if (status)
    {
        //some error occured while getting the frame
        return status;
    }

    uint16_t startLine = getStartLineToDraw();
    uint16_t endLine = getEndLineToDraw(startLine);

    if (startLine >= endLine)
    {
        //nothing to update
        return 0;
    }

    ili9341Shield_setAddrWindow(0, startLine, _width, endLine);
    pushColors(&frameBuffer[frameBufferIndex][startLine * _width], (endLine - startLine) * _width);
    frameBufferIndex++;
    frameBufferIndex %= NO_OF_FRAME_BUFFERS;
    return 0;
}

static uint16_t getEndLineToDraw(uint16_t lastLine)
{
    unsigned int *startPos = (unsigned int *)&(frameBuffer[0][_width * _height]);
    unsigned int *data1 = startPos;
    unsigned int *data2 = (unsigned int *)&(frameBuffer[1][_width * _height]);
    unsigned int const *lastToCheck = (unsigned int *)&(frameBuffer[0][_width * lastLine]);

    data1--;
    data2--;
    lastToCheck--;

    for (; data1 > lastToCheck; --data1, --data2)
    {
        if (*data1 != *data2)
        {
            uint32_t line = (startPos - data1) * sizeof(unsigned int); //number of bytes
            line /= sizeof(uint16_t);                                  //number of pixels
            line /= _width;                                            //number of lines
            line = _height - line;
            return (uint16_t)line;
        }
    }

    return lastLine; // everything was the same
}

static uint16_t getStartLineToDraw(void)
{
    unsigned int *data1 = (unsigned int *)&(frameBuffer[0][0]);
    unsigned int *data2 = (unsigned int *)&(frameBuffer[1][0]);

    unsigned int const *lastToCheck = (unsigned int *)&(frameBuffer[1][0]);

    for (; data1 < lastToCheck; ++data1, ++data2)
    {

        if (*data1 != *data2)
        {
            uint32_t line = ((data1 - ((unsigned int *)&(frameBuffer[0][0]))) * sizeof(unsigned int)); //number of bytes
            line /= sizeof(uint16_t);                                                                  //number of pixels
            line /= _width;                                                                            //number of lines
            return (uint16_t)line;
        }
    }

    return _height;
}

rotation_t display_getDisplayRotation(void)
{
    return _rotation;
}

int display_getDisplayWidth(void)
{
    return _width;
}
int display_getDisplayHeight(void)
{
    return _height;
}

void display_drawPixel(uint16_t x, uint16_t y, uint16_t color)
{

    // Clip
    if ((x >= display_getDisplayWidth()) || (y >= display_getDisplayHeight()))
        return;

    ili9341Shield_setAddrWindow(x, y, display_getDisplayWidth() - 1, display_getDisplayHeight() - 1);
    CS_ACTIVE;
    CD_COMMAND;
    ili9341Shield_write8(ILI9341_MEMORYWRITE);
    CD_DATA;
    ili9341Shield_write8(color >> 8);
    ili9341Shield_write8(color);
    CS_IDLE;
}

void display_fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h,
                      uint16_t fillcolor)
{
    int16_t x2, y2;

    // Initial off-screen clipping
    if ((w <= 0) || (h <= 0) ||
        (x1 >= display_getDisplayWidth()) || (y1 >= display_getDisplayHeight()) ||
        ((x2 = x1 + w - 1) < 0) || ((y2 = y1 + h - 1) < 0))
    {
        return;
    }

    if (x1 < 0)
    { // Clip left
        w += x1;
        x1 = 0;
    }
    if (y1 < 0)
    { // Clip top
        h += y1;
        y1 = 0;
    }
    if (x2 >= display_getDisplayWidth())
    { // Clip right
        x2 = display_getDisplayWidth() - 1;
        w = x2 - x1 + 1;
    }
    if (y2 >= display_getDisplayHeight())
    { // Clip bottom
        y2 = display_getDisplayHeight() - 1;
        h = y2 - y1 + 1;
    }

    ili9341Shield_setAddrWindow(x1, y1, x2, y2);
    flood(fillcolor, (uint32_t)w * (uint32_t)h);
}

static void flood(uint16_t color, uint32_t len)
{
    uint16_t blocks;
    uint8_t i, hi = color >> 8,
               lo = color;

    CS_ACTIVE;
    CD_COMMAND;

    ili9341Shield_write8(ILI9341_MEMORYWRITE);

    // Write first pixel normally, decrement counter by 1
    CD_DATA;
    ili9341Shield_write8(hi);
    ili9341Shield_write8(lo);
    len--;

    blocks = (uint16_t)(len / 64); // 64 pixels/block
    if (hi == lo)
    {
        // High and low bytes are identical.  Leave prior data
        // on the port(s) and just toggle the write strobe.
        while (blocks--)
        {
            i = 16; // 64 pixels/block / 4 pixels/pass
            do
            {
                WR_STROBE;
                WR_STROBE;
                WR_STROBE;
                WR_STROBE; // 2 bytes/pixel
                WR_STROBE;
                WR_STROBE;
                WR_STROBE;
                WR_STROBE; // x 4 pixels
            } while (--i);
        }
        // Fill any remaining pixels (1 to 64)
        for (i = (uint8_t)len & 63; i--;)
        {
            WR_STROBE;
            WR_STROBE;
        }
    }
    else
    {
        while (blocks--)
        {
            i = 16; // 64 pixels/block / 4 pixels/pass
            do
            {
                ili9341Shield_write8(hi);
                ili9341Shield_write8(lo);
                ili9341Shield_write8(hi);
                ili9341Shield_write8(lo);
                ili9341Shield_write8(hi);
                ili9341Shield_write8(lo);
                ili9341Shield_write8(hi);
                ili9341Shield_write8(lo);
            } while (--i);
        }
        for (i = (uint8_t)len & 63; i--;)
        {
            ili9341Shield_write8(hi);
            ili9341Shield_write8(lo);
        }
    }
    CS_IDLE;
}

void fillScreen(uint16_t color)
{
    display_fillRect(0, 0, display_getDisplayWidth(), display_getDisplayHeight(), color);
}

#if 0
// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
#endif

// Issues an array of 16-bit color values to the LCD;
// Assumes that setWindowAddr() has previously been set to define the bounds
static void pushColors(uint16_t *data, uint32_t len)
{
    uint16_t color;
    uint8_t hi, lo;
    CS_ACTIVE;
    CD_COMMAND;
    ili9341Shield_write8(ILI9341_MEMORYWRITE);
    CD_DATA;
    while (len--)
    {
        color = *data++;
        hi = color >> 8;
        lo = color;
        ili9341Shield_write8(hi);
        ili9341Shield_write8(lo);
    }
    CS_IDLE;
}

void setRotation(rotation_t rotation)
{
    uint16_t t = ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR;
    _rotation = rotation;

    switch (rotation)
    {
    case ROTATION_270_DEGREES:
        t = ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR;
        _width = TFT_WIDTH;
        _height = TFT_HEIGHT;
        break;
    case ROTATION_0_DEGREES:
        t = ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR;
        _width = TFT_HEIGHT;
        _height = TFT_WIDTH;
        break;
    case ROTATION_90_DEGREES:
        t = ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR;
        _width = TFT_WIDTH;
        _height = TFT_HEIGHT;
        break;
    case ROTATION_180_DEGREES:
        t = ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR;
        _width = TFT_HEIGHT;
        _height = TFT_WIDTH;
        break;
    }

    CS_ACTIVE;
    ili9341Shield_writeRegister8(ILI9341_MADCTL, t);
    // init default full-screen address window:
    ili9341Shield_setAddrWindow(0, 0, _width - 1, _height - 1);
}

void display_displayOff(void)
{
    CS_ACTIVE;
    CD_COMMAND;
    ili9341Shield_write8(ILI9341_DISPLAYOFF);
    CS_IDLE;
}

void displayOn(void)
{
    CS_ACTIVE;
    CD_COMMAND;
    ili9341Shield_write8(ILI9341_DISPLAYON);
    CS_IDLE;
}