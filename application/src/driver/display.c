#include "display.h"
#include <pigpio.h>
#include "ili9341Shield.h"

static void setLR(void);
static void flood(uint16_t color, uint32_t len);

void drawPixel(int16_t x, int16_t y, uint16_t color)
{

    // Clip
    if ((x < 0) || (y < 0) || (x >= ili9341Shield_getDisplayWidth()) || (y >= ili9341Shield_getDisplayHeight()))
        return;

    CS_ACTIVE;
    setAddrWindow(x, y, ili9341Shield_getDisplayWidth() - 1, ili9341Shield_getDisplayHeight() - 1);
    CS_ACTIVE;
    CD_COMMAND;
    write8(0x2C);
    CD_DATA;
    write8(color >> 8);
    write8(color);

    CS_IDLE;
}

void fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h,
              uint16_t fillcolor)
{
    int16_t x2, y2;

    // Initial off-screen clipping
    if ((w <= 0) || (h <= 0) ||
        (x1 >= ili9341Shield_getDisplayWidth()) || (y1 >= ili9341Shield_getDisplayHeight()) ||
        ((x2 = x1 + w - 1) < 0) || ((y2 = y1 + h - 1) < 0))
        return;
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
    if (x2 >= ili9341Shield_getDisplayWidth())
    { // Clip right
        x2 = ili9341Shield_getDisplayWidth() - 1;
        w = x2 - x1 + 1;
    }
    if (y2 >= ili9341Shield_getDisplayHeight())
    { // Clip bottom
        y2 = ili9341Shield_getDisplayHeight() - 1;
        h = y2 - y1 + 1;
    }

    setAddrWindow(x1, y1, x2, y2);
    flood(fillcolor, (uint32_t)w * (uint32_t)h);
    setLR();
}

static void setLR(void)
{
    CS_ACTIVE;
    /*writeRegisterPair(HX8347G_COLADDREND_HI, HX8347G_COLADDREND_LO, _width  - 1);
  writeRegisterPair(HX8347G_ROWADDREND_HI, HX8347G_ROWADDREND_LO, _height - 1);
  */
    CS_IDLE;
}

static void flood(uint16_t color, uint32_t len)
{
    uint16_t blocks;
    uint8_t i, hi = color >> 8,
               lo = color;

    CS_ACTIVE;
    CD_COMMAND;

    write8(0x2C);

    // Write first pixel normally, decrement counter by 1
    CD_DATA;
    write8(hi);
    write8(lo);
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
                write8(hi);
                write8(lo);
                write8(hi);
                write8(lo);
                write8(hi);
                write8(lo);
                write8(hi);
                write8(lo);
            } while (--i);
        }
        for (i = (uint8_t)len & 63; i--;)
        {
            write8(hi);
            write8(lo);
        }
    }
    CS_IDLE;
}

void fillScreen(uint16_t color)
{
    fillRect(0, 0, ili9341Shield_getDisplayWidth(), ili9341Shield_getDisplayHeight(), color);
}

void drawBorder()
{

    // Draw a border

    uint16_t width = ili9341Shield_getDisplayWidth() - 1;
    uint16_t height = ili9341Shield_getDisplayHeight() - 1;
    uint8_t border = 30;

    fillScreen(RED);
    fillRect(border, border, (width - border * 2), (height - border * 2), BLUE);
}

// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Issues 'raw' an array of 16-bit color values to the LCD; used
// externally by BMP examples.  Assumes that setWindowAddr() has
// previously been set to define the bounds.  Max 255 pixels at
// a time (BMP examples read in small chunks due to limited RAM).
void pushColors(uint16_t *data, /*uint8_t*/ int len, bool first)
{
    uint16_t color;
    uint8_t hi, lo;
    CS_ACTIVE;
    if (first == true)
    { // Issue GRAM write command only on first call
        CD_COMMAND;
        write8(0x2C);
    }
    CD_DATA;
    while (len--)
    {
        color = *data++;
        hi = color >> 8; // Don't simplify or merge these
        lo = color;      // lines, there's macro shenanigans
        write8(hi);      // going on.
        write8(lo);
    }
    CS_IDLE;
}