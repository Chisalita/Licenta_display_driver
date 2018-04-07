#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  //degrees are defined as clockwise, starting as having the button on the right side of the screen
  ROTATION_0_DEGREES,
  ROTATION_90_DEGREES,
  ROTATION_180_DEGREES,
  ROTATION_270_DEGREES
} rotation_t;

int display_drawFrameBuffer();
void display_drawPixel(int16_t x, int16_t y, uint16_t color);
void display_fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h, uint16_t fillcolor);
void display_drawBorder();

///////TODO:make them static////////
uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
void pushColors(uint16_t *data, /*uint8_t*/ int len, bool first);
///////////

void display_displayOff();
void displayOn();
void setRotation(rotation_t rotation);

int display_getDisplayWidth();
int display_getDisplayHeight();
rotation_t display_getDisplayRotation();