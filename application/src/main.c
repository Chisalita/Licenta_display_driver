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

#define DESIRED_FRAME_RATE (30)
#define MIN_DELAY_BETWEEN_FRAMES_MS (1000 / DESIRED_FRAME_RATE)

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
  frameBuffer_initFrameBuffer(display_getDisplayWidth(), display_getDisplayHeight());

  int fbWidth, fbHeight;
  frameBuffer_getActualFbDim(&fbWidth, &fbHeight);
  int st = touchScreen_initTouch(fbWidth, fbHeight);
  if (st)
  {
    printf("Error at touchScreen_initTouch (%d)\n", st);
  }

  //clear the screen
  display_fillRect(0, 0, display_getDisplayWidth(), display_getDisplayHeight(), BLACK);

  struct timeval start, end;
  long useconds;
  while (1)
  {

    gettimeofday(&start, NULL);
    
    display_drawFrameBufferOptimised();
    touchScreen_getPoint();

    gettimeofday(&end, NULL);
    useconds = end.tv_usec - start.tv_usec;
    if (useconds < 0)
    {
      useconds = 1E6 - useconds;
    }

    long toSleep = (MIN_DELAY_BETWEEN_FRAMES_MS * 1000) - useconds;
    if (toSleep > 0)
    {
      usleep(toSleep);
    }
  }

  frameBuffer_deinitFrameBuffer();
  touchScreen_deinitTouch();

  return 0;
}
