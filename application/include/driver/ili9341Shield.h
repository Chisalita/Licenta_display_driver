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

#define RESET_PIN (13)
#define RD_PIN (19)
#define WR_PIN (2)
#define CD_PIN (06)
#define CS_PIN (05)
#define DATA_PINS_OFFSET (20)
#define DATA_PINS_MASK (((uint32_t)0xFF) << DATA_PINS_OFFSET)

// These are single-instruction operations and always inline
#define RD_ACTIVE GPIO_WRITE(RD_PIN, 0)
#define RD_IDLE GPIO_WRITE(RD_PIN, 1)
#define WR_ACTIVE GPIO_WRITE(WR_PIN, 0)
#define WR_IDLE GPIO_WRITE(WR_PIN, 1)
#define CD_COMMAND GPIO_WRITE(CD_PIN, 0)
#define CD_DATA GPIO_WRITE(CD_PIN, 1)
#define CS_ACTIVE GPIO_WRITE(CS_PIN, 0)
#define CS_IDLE GPIO_WRITE(CS_PIN, 1)

// Data write strobe, ~2 instructions and always inline
#define WR_STROBE \
  {               \
    WR_ACTIVE;    \
    WR_IDLE;      \
  }

void ili9341Shield_init(void);
void ili9341Shield_write8(uint8_t value);
void ili9341Shield_reset(void);
void ili9341Shield_writeNoParamCommand(uint8_t value);
void ili9341Shield_writeRegister32(uint8_t r, uint32_t d);
void ili9341Shield_setAddrWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

// Set value of TFT register: 8-bit address, 8-bit value
#define ili9341Shield_writeRegister8(a, d) \
  {                                        \
    CD_COMMAND;                            \
    ili9341Shield_write8(a);               \
    CD_DATA;                               \
    ili9341Shield_write8(d);               \
  }

// Set value of TFT register: 16-bit address, 16-bit value
#define ili9341Shield_writeRegister16(a, d) \
  {                                         \
    uint8_t hi, lo;                         \
    hi = (a) >> 8;                          \
    lo = (a);                               \
    CD_COMMAND;                             \
    ili9341Shield_write8(hi);               \
    ili9341Shield_write8(lo);               \
    hi = (d) >> 8;                          \
    lo = (d & 0xff);                        \
    CD_DATA;                                \
    ili9341Shield_write8(hi);               \
    ili9341Shield_write8(lo);               \
  }
