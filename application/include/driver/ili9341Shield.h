#pragma once

#include <stdint.h>

// Assign human-readable names to some common 16-bit color values:
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

#define RESET_PIN (13) //(26)
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
 #define ili9341Shield_write8inline(d) { \
   (d & 0xff)<<DATA_PINS_OFFSET
   WR_STROBE; }
*/


void ili9341Shield_init();
void ili9341Shield_write8(uint8_t value);
void ili9341Shield_reset(void);
void ili9341Shield_writeNoParamCommand(uint8_t value);
void ili9341Shield_writeRegister32(uint8_t r, uint32_t d);
void ili9341Shield_setAddrWindow(int x1, int y1, int x2, int y2);


// Set value of TFT register: 8-bit address, 8-bit value
#define ili9341Shield_writeRegister8(a, d) \
  {                          \
    CD_COMMAND;              \
    ili9341Shield_write8(a);               \
    CD_DATA;                 \
    ili9341Shield_write8(d);               \
  }

// Set value of TFT register: 16-bit address, 16-bit value
// See notes at top about macro expansion, hence hi & lo temp vars
#define ili9341Shield_writeRegister16(a, d) \
  {                           \
    uint8_t hi, lo;           \
    hi = (a) >> 8;            \
    lo = (a);                 \
    CD_COMMAND;               \
    ili9341Shield_write8(hi);               \
    ili9341Shield_write8(lo);               \
    hi = (d) >> 8;            \
    lo = (d & 0xff);          \
    CD_DATA;                  \
    ili9341Shield_write8(hi);               \
    ili9341Shield_write8(lo);               \
  }
