#include <stdio.h>
#include <pigpio.h>
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
#include "display.h" //TODO: remove when not needed
#include "frameBuffer.h"

int main(int argc, char **argv)
{

  int res = gpioInitialise();
  if (res < 0)
  {
    return 1;
  }
  int i;
  ili9341Shield_init();
  printf("____________________________\n");

  printf("setAddr\n");
  // int i=0;

  display_drawBorder();
  for (i = 0; i < 150; i++)
  {
    display_drawPixel(i, i, BLACK);
  }

  printf("pixels\n");

  display_fillRect(50, 50, 50, 50, GREEN);

  //See how much time it takes to fill the screen
  uint16_t data[320 * 240];
  uint8_t color = GREEN;
  //uint16_t data[200];
  /* for (i = 0; i < 500; i++)
  {
    color ^= BLUE;
    memset(data, color, sizeof(data));
    ili9341Shield_setAddrWindow(0, 0, _width - 1, _height - 1);

    struct timeval start, end;

    long mtime, seconds, useconds;

    gettimeofday(&start, NULL);

    //pushColors(data, sizeof(data) / 2, true);
    // display_displayOff();
    display_fillRect(0, 0, _width, _height, color);
    //ili9341Shield_writeRegister8(ILI9341_DISPLAYON, 0);

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
*/


  int st = touchScreen_initTouch();
  setRotation(ROTATION_0_DEGREES);


  //display_fillRect(0, 0, _width, _height, RED);
  while(1){
  display_drawFrameBuffer();
  touchScreen_getPoint();
  }



  while (1)
  {
    usleep(1000 * 100);
    printf("do\n");
    /*readChannel(0);
        readChannel(1);*/
    touchScreen_getPoint();
  }

  touchScreen_deinitTouch();

  return 0;
}
