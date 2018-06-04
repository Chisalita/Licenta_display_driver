#include <stdio.h>
#ifdef USING_PIGPIO_LIB
#include <pigpio.h>
#else
#include <bcm2835.h>
#endif
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <unistd.h>
#include <sys/time.h>

#include "registers.h"
#include "ili9341Shield.h"
#include "touchScreen.h"
#include "display.h"
#include "frameBuffer.h"

int main(int argc, char **argv)
{

  int res = GPIO_INIT();

#ifdef USING_PIGPIO_LIB
  if (res < 0)
#else
  if (!res)
#endif
  {
    return 1;
  }
#if 0
  int out = open("/home/pi/driver.out", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
  if (out < 0)
  {
    printf("error opening out file\n");
  }

  if(dup2(out, 1) < 0){
        printf("error dup2\n");
  }
#endif
  ili9341Shield_init();
  setRotation(ROTATION_0_DEGREES);
  printf("____________________________\n");
#ifdef USING_PIGPIO_LIB
  printf("USING_PIGPIO_LIB\n");
#else
  printf("USING_BCM_LIB\n");
#endif
  printf("____________________________\n");

  frameBuffer_initFrameBuffer(display_getDisplayWidth(), display_getDisplayHeight());
  
  int fbWidth, fbHeight;
  frameBuffer_getActualFbDim(&fbWidth, &fbHeight);
  int st = touchScreen_initTouch(fbWidth, fbHeight);
  if (st)
  {
    printf("Error at touchScreen_initTouch (%d)\n", st);
  }


  display_fillRect(0, 0, display_getDisplayWidth(), display_getDisplayHeight(), BLACK);
  //TODO: remove this
  for(int i=0; i<100; i++){
    frameBuffer_drawPixel(i, i, GREEN);
    frameBuffer_drawPixel(30, i, YELLOW);
    frameBuffer_drawPixel(i, 100, BLUE);
  }


  //clear the screen
  while (1)
  {
    display_drawFrameBufferOptimised();
    // usleep(25 * 1000);
    touchScreen_getPoint();
  }

  frameBuffer_deinitFrameBuffer();
  touchScreen_deinitTouch();

  return 0;
}
