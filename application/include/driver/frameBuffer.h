#pragma once

#include <stdint.h>

int frameBuffer_initFrameBuffer(uint16_t width, uint16_t height);
int frameBuffer_deinitFrameBuffer(void);
int frameBuffer_getFrame(void* outFrameBuff);
void frameBuffer_drawPixel(uint16_t x, uint16_t y, uint16_t color);
int frameBuffer_getActualFbDim(int* width, int* height);