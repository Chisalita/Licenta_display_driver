#pragma once

#include <stdint.h>

int frameBuffer_initFrameBuffer();
int frameBuffer_deinitFrameBuffer();
int frameBuffer_getFrame(void* outFrameBuff);
void frameBuffer_drawPixel(uint16_t x, uint16_t y, uint16_t color);
int frameBuffer_getActualFbDim(int* width, int* height);