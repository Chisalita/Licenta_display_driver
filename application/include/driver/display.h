#pragma once
#include <stdint.h>

typedef enum {
  //degrees are defined as clockwise, starting as having the button on the right side of the screen
  ROTATION_0_DEGREES,
  ROTATION_90_DEGREES,
  ROTATION_180_DEGREES,
  ROTATION_270_DEGREES
} rotation_t;

int display_drawFrameBuffer(void);
void display_drawPixel(uint16_t x, uint16_t y, uint16_t color);
void display_fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h, uint16_t fillcolor);
void display_displayOff(void);
void displayOn(void);
void setRotation(rotation_t rotation);

int display_getDisplayWidth(void);
int display_getDisplayHeight(void);
rotation_t display_getDisplayRotation(void);