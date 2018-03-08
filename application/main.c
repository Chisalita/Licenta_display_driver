#include <stdio.h>
#include <pigpio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/time.h>

#include "registers.h"

#define delay(t) usleep((t)*1000)
#define delayMicroseconds(t) usleep(t)

// Assign human-readable names to some common 16-bit color values:
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

#define TFTWIDTH 240
#define TFTHEIGHT 320

#define RESET_PIN (13)//(26)
#define RD_PIN (19)
#define WR_PIN (2)
#define CD_PIN (06)
#define CS_PIN (05)

#define DATA_PINS_OFFSET (20)

// These are single-instruction operations and always inline
#define RD_ACTIVE gpioWrite(RD_PIN, 0)
#define RD_IDLE gpioWrite(RD_PIN, 1)
#define WR_ACTIVE gpioWrite(WR_PIN, 0)
#define WR_IDLE gpioWrite(WR_PIN, 1)
#define CD_COMMAND gpioWrite(CD_PIN, 0)
#define CD_DATA gpioWrite(CD_PIN, 1)
#define CS_ACTIVE gpioWrite(CS_PIN, 0)
#define CS_IDLE gpioWrite(CS_PIN, 1)

// Data write strobe, ~2 instructions and always inline
#define WR_STROBE \
  {               \
    WR_ACTIVE;    \
    WR_IDLE;      \
  }

/*
 #define write8inline(d) { \
   (d & 0xff)<<DATA_PINS_OFFSET
   WR_STROBE; }
*/

void write8(uint8_t value)
{

  //TODO: make generic the mask
  uint32_t mask = 0xFF00000;//0x3FC00; //for bits DATA_PINS_OFFSET to DATA_PINS_OFFSET+7
  uint32_t data = ((uint32_t)value) << DATA_PINS_OFFSET;
  //  printf("set = 0x%X\n", data);
  //  printf("clear = 0x%X\n", (data ^ mask));
  gpioWrite_Bits_0_31_Set(data);
  gpioWrite_Bits_0_31_Clear(data ^ mask);
  //TODO: see if strobe should be on the extremity (before and after)
  WR_STROBE;
}

// Set value of TFT register: 8-bit address, 8-bit value
#define writeRegister8(a, d) \
  {                          \
    CD_COMMAND;              \
    write8(a);               \
    CD_DATA;                 \
    write8(d);               \
  }

// Set value of TFT register: 16-bit address, 16-bit value
// See notes at top about macro expansion, hence hi & lo temp vars
#define writeRegister16(a, d) \
  {                           \
    uint8_t hi, lo;           \
    hi = (a) >> 8;            \
    lo = (a);                 \
    CD_COMMAND;               \
    write8(hi);               \
    write8(lo);               \
    hi = (d) >> 8;            \
    lo = (d & 0xff);          \
    CD_DATA;                  \
    write8(hi);               \
    write8(lo);               \
  }

int _reset = RESET_PIN;

void drawPixel(int16_t x, int16_t y, uint16_t color);
void writeRegister32(uint8_t r, uint32_t d);
void setAddrWindow(int x1, int y1, int x2, int y2);
void reset(void);
void flood(uint16_t color, uint32_t len);
void fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h, uint16_t fillcolor);
void setLR(void);
void drawBorder();
uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
void pushColors(uint16_t *data, /*uint8_t*/ int len, bool first);

void writeRegister32(uint8_t r, uint32_t d)
{
  CS_ACTIVE;
  CD_COMMAND;
  write8(r);
  CD_DATA;
  delayMicroseconds(10);
  write8(d >> 24);
  delayMicroseconds(10);
  write8(d >> 16);
  delayMicroseconds(10);
  write8(d >> 8);
  delayMicroseconds(10);
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

int main()
{

  int res = gpioInitialise();
  if (res < 0)
  {
    return 1;
  }

  gpioSetMode(RESET_PIN, PI_OUTPUT);
  gpioSetMode(RD_PIN, PI_OUTPUT);
  gpioSetMode(WR_PIN, PI_OUTPUT);
  gpioSetMode(CD_PIN, PI_OUTPUT);
  gpioSetMode(CS_PIN, PI_OUTPUT);
  //set data to output
  int i = DATA_PINS_OFFSET;
  for (; i < DATA_PINS_OFFSET+8; ++i)
  {
    gpioSetMode(i, PI_OUTPUT);
  }

  write8(0x0); //just set to 0

  reset();
  printf("reset\n");
  delay(200);

  // uint16_t a, d;
  CS_ACTIVE;
  writeRegister8(ILI9341_SOFTRESET, 0);
  // writeNoParamCommand(ILI9341_SOFTRESET);
  delay(50);
  //writeRegister8(ILI9341_DISPLAYOFF, 0);
  // writeNoParamCommand(ILI9341_DISPLAYOFF);

  writeRegister8(ILI9341_POWERCONTROL1, 0x23);
  writeRegister8(ILI9341_POWERCONTROL2, 0x10);
  writeRegister16(ILI9341_VCOMCONTROL1, 0x2B2B);
  writeRegister8(ILI9341_VCOMCONTROL2, 0xC0);
  writeRegister8(ILI9341_MEMCONTROL, /*ILI9341_MADCTL_MY |*/ ILI9341_MADCTL_BGR);
  writeRegister8(ILI9341_PIXELFORMAT, 0x55);
  writeRegister16(ILI9341_FRAMECONTROL, 0x001B);

  writeRegister8(ILI9341_ENTRYMODE, 0x07);
  // writeRegister8(ILI9341_SLEEPOUT, 0);
  writeNoParamCommand(ILI9341_SLEEPOUT);
  delay(150);
  // writeRegister8(ILI9341_DISPLAYON, 0);
  writeNoParamCommand(ILI9341_DISPLAYON);
  //init();
  delay(500);
  setAddrWindow(0, 0, TFTWIDTH - 1, TFTHEIGHT - 1);
  CS_IDLE;

  printf("setAddr\n");
  // int i=0;

  drawBorder();
  for (i = 0; i < 150; i++)
  {
    drawPixel(i, i, BLACK);
  }

  printf("pixels\n");

  fillRect(50, 50, 50, 50, YELLOW);

  //See how much time it takes to fill the screen
  uint16_t data[320 * 240];
  uint8_t color = GREEN;
  //uint16_t data[200];
  for (i = 0; i < 500; i++)
  {
    color ^= BLUE;
    memset(data, color, sizeof(data));
    setAddrWindow(0, 0, TFTWIDTH - 1, TFTHEIGHT - 1);

    struct timeval start, end;

    long mtime, seconds, useconds;

    gettimeofday(&start, NULL);

    //pushColors(data, sizeof(data) / 2, true);
    // displayOff();
    fillRect(0, 0, TFTWIDTH, TFTHEIGHT, color);
    //writeRegister8(ILI9341_DISPLAYON, 0);

    // displayOn();
    //usleep(1E6);
    gettimeofday(&end, NULL);
    seconds = end.tv_sec - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;

    mtime = ((seconds)*1000 + useconds / 1000.0) + 0.5;

    printf("Elapsed time: %ld milliseconds\n", mtime);
  }

  while (1)
  {
  }

  return 0;
}

void drawPixel(int16_t x, int16_t y, uint16_t color)
{

  // Clip
  if ((x < 0) || (y < 0) || (x >= TFTWIDTH) || (y >= TFTHEIGHT))
    return;

  CS_ACTIVE;
  setAddrWindow(x, y, TFTWIDTH - 1, TFTHEIGHT - 1);
  CS_ACTIVE;
  CD_COMMAND;
  write8(0x2C);
  CD_DATA;
  write8(color >> 8);
  write8(color);

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

    /*
    gpioWrite(_reset,0);
    delay(120);
    gpioWrite(_reset,1);
    delay(120);
  */
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

void fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h,
              uint16_t fillcolor)
{
  int16_t x2, y2;

  // Initial off-screen clipping
  if ((w <= 0) || (h <= 0) ||
      (x1 >= TFTWIDTH) || (y1 >= TFTHEIGHT) ||
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
  if (x2 >= TFTWIDTH)
  { // Clip right
    x2 = TFTWIDTH - 1;
    w = x2 - x1 + 1;
  }
  if (y2 >= TFTHEIGHT)
  { // Clip bottom
    y2 = TFTHEIGHT - 1;
    h = y2 - y1 + 1;
  }

  setAddrWindow(x1, y1, x2, y2);
  flood(fillcolor, (uint32_t)w * (uint32_t)h);
  setLR();
}

void setLR(void)
{
  CS_ACTIVE;
  /*writeRegisterPair(HX8347G_COLADDREND_HI, HX8347G_COLADDREND_LO, _width  - 1);
  writeRegisterPair(HX8347G_ROWADDREND_HI, HX8347G_ROWADDREND_LO, _height - 1);
  */
  CS_IDLE;
}

void flood(uint16_t color, uint32_t len)
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
  fillRect(0, 0, TFTWIDTH, TFTHEIGHT, color);
}

void drawBorder()
{

  // Draw a border

  uint16_t width = TFTWIDTH - 1;
  uint16_t height = TFTHEIGHT - 1;
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