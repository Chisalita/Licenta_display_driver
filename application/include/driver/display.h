#pragma once
#include <stdint.h>
#include <stdbool.h>


void drawPixel(int16_t x, int16_t y, uint16_t color);
void fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h, uint16_t fillcolor);
void drawBorder();
uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
void pushColors(uint16_t *data, /*uint8_t*/ int len, bool first);