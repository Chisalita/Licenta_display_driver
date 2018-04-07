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

int main()
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

  drawBorder();
  for (i = 0; i < 150; i++)
  {
    drawPixel(i, i, BLACK);
  }

  printf("pixels\n");

  fillRect(50, 50, 50, 50, GREEN);

  //See how much time it takes to fill the screen
  uint16_t data[320 * 240];
  uint8_t color = GREEN;
  //uint16_t data[200];
  /* for (i = 0; i < 500; i++)
  {
    color ^= BLUE;
    memset(data, color, sizeof(data));
    setAddrWindow(0, 0, _width - 1, _height - 1);

    struct timeval start, end;

    long mtime, seconds, useconds;

    gettimeofday(&start, NULL);

    //pushColors(data, sizeof(data) / 2, true);
    // displayOff();
    fillRect(0, 0, _width, _height, color);
    //writeRegister8(ILI9341_DISPLAYON, 0);

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

  /////////////FRAME BUFFER////////////////
  char *outfile = "out.file";
  int outfd = open(outfile, O_RDWR);
  uint16_t frameBuffer[320 * 240];

  if (outfd == -1)
  {
    fprintf(stderr, "cannot open outfd - %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  int r = read(outfd, frameBuffer, sizeof(frameBuffer));
  printf("read %d bytes\n", r);
  setRotation(ROTATION_0_DEGREES);
  //fillRect(0, 0, _width, _height, RED);
  pushColors(frameBuffer, r / 2, true);

  /////////////FRAME BUFFER////////////////
  int st = touchScreen_initTouch();

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