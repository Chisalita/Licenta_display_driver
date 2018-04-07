#include "touchScreen.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pigpio.h>

#include "ili9341Shield.h"
#include "display.h" //TODO: remove when not needed

#define REFERENCE_VOLTAGE (3.3f)
#define NUMSAMPLES (1)

#define _yp (2)
#define _xm (6)
#define _ym (27)
#define _xp (26)

// Calibrate values
#define TS_MINX 75  //125
#define TS_MINY 280 //85
#define TS_MAXX 650 //965
#define TS_MAXY 935 //905

static int analogRead(int c);
static long map(long x, long in_min, long in_max, long out_min, long out_max);
static int readChannel(uint8_t channel);

int handle;       //make it local!!!
int _rxplate = 0; //300; //WHAT IS THIS? (resistenta cred)

#if (NUMSAMPLES > 2)
static void insert_sort(int array[], uint8_t size)
{
    uint8_t j;
    int save;

    for (int i = 1; i < size; i++)
    {
        save = array[i];
        for (j = i; j >= 1 && save < array[j - 1]; j--)
            array[j] = array[j - 1];
        array[j] = save;
    }
}
#endif

int touchScreen_initTouch()
{
    handle = spiOpen(0, 96000, 0);
    if (handle < 0)
    {
        //bad!
        printf("Error opening SPI!\n");
        return handle;
    }
    return 0;
}

int touchScreen_deinitTouch()
{
    return spiClose(handle);
}

void touchScreen_getPoint(void)
{
    int x, y, z;
    int samples[NUMSAMPLES];
    uint8_t i, valid;

    valid = 1;

    gpioSetMode(_yp, PI_INPUT);
    gpioSetMode(_ym, PI_INPUT);
    gpioSetPullUpDown(_yp, PI_PUD_OFF);
    gpioSetPullUpDown(_ym, PI_PUD_OFF);

    gpioSetMode(_xp, PI_OUTPUT);
    gpioSetMode(_xm, PI_OUTPUT);
    gpioWrite(_xp, PI_HIGH);
    gpioWrite(_xm, PI_LOW);

    for (i = 0; i < NUMSAMPLES; i++)
    {
        samples[i] = analogRead(_yp);
    }
#if NUMSAMPLES > 2
    insert_sort(samples, NUMSAMPLES);
#endif
#if NUMSAMPLES == 2
    if (samples[0] != samples[1])
    {
        valid = 0;
    }
#endif
    x = (1023 - samples[NUMSAMPLES / 2]);

    usleep(10000);

    gpioSetMode(_xp, PI_INPUT);
    gpioSetMode(_xm, PI_INPUT);
    gpioSetPullUpDown(_xp, PI_PUD_OFF);
    gpioSetPullUpDown(_xm, PI_PUD_OFF);

    gpioSetMode(_yp, PI_OUTPUT);
    gpioWrite(_yp, PI_HIGH);
    gpioSetMode(_ym, PI_OUTPUT);
    gpioWrite(_ym, PI_LOW); ////////TODO: remove if not working?

    for (i = 0; i < NUMSAMPLES; i++)
    {
        samples[i] = analogRead(_xm);
    }

#if NUMSAMPLES > 2
    insert_sort(samples, NUMSAMPLES);
#endif
#if NUMSAMPLES == 2
    if (samples[0] != samples[1])
    {
        valid = 0;
    }
#endif

    y = (1023 - samples[NUMSAMPLES / 2]);

    // Set X+ to ground
    gpioSetMode(_xp, PI_OUTPUT);
    gpioWrite(_xp, PI_LOW);

    // Set Y- to VCC
    gpioSetMode(_ym, PI_OUTPUT); //TODO: remove after tested
    gpioWrite(_ym, PI_HIGH);

    // Hi-Z X- and Y+
    gpioSetMode(_yp, PI_OUTPUT);
    gpioWrite(_yp, PI_LOW);

    gpioSetMode(_yp, PI_INPUT);

    gpioSetPullUpDown(_yp, PI_PUD_OFF); //HiZ (not sure if good)

    ///////////////////////////
    int z1 = analogRead(_xm);
    int z2 = analogRead(_yp);

    if (_rxplate != 0)
    {
        // now read the x
        float rtouch;
        rtouch = z2;
        rtouch /= z1;
        rtouch -= 1;
        rtouch *= x;
        rtouch *= _rxplate;
        rtouch /= 1024;

        z = rtouch;
    }
    else
    {
        z = (1023 - (z2 - z1));
    }

    if (!valid)
    {
        z = 0;
    }

    // printf("Normal: x = %d, y = %d, z = %d\n", x, y, z);
    // printf("Inverted: x = %d, y = %d, z = %d\n", x, 1023 - y, z);

    int tsMaxx = TS_MAXX;
    int tsMaxy = TS_MAXY;
    int tsMinx = TS_MINX;
    int tsMiny = TS_MINY;

    if (ili9341Shield_getDisplayRotation() == ROTATION_0_DEGREES || ili9341Shield_getDisplayRotation() == ROTATION_180_DEGREES)
    {
        //TODO: make common case faster
        int aux = x;
        x = y;
        y = aux;

        tsMaxx = TS_MAXY;
        tsMaxy = TS_MAXX;
        tsMinx = TS_MINY;
        tsMiny = TS_MINX;
    }

    long px = map(x, tsMinx, tsMaxx, 0, ili9341Shield_getDisplayWidth());
    long py = map(y, tsMiny, tsMaxy, 0, ili9341Shield_getDisplayHeight());

    printf("Maped Inverted: x = %ld, y = %ld, z = %d\n", px, py, z);

    int16_t pointWidth = 5;
    int16_t pointHeight = 5;

    if (z > 200)
    { //TODO: make define

        //TODO: see what is needed
        gpioSetMode(_yp, PI_OUTPUT);
        gpioSetMode(_xm, PI_OUTPUT);
        gpioSetMode(_ym, PI_OUTPUT);
        gpioSetMode(_xp, PI_OUTPUT);

        fillRect(px, py, pointWidth, pointHeight, GREEN);
    }
}

static long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static int readChannel(uint8_t channel)
{
    uint8_t command = 0b11 << 6;
    command |= (channel & 0x07) << 3;
    const int commandLen = 3;
    char txBuf[commandLen];
    char rxBuf[commandLen];

    memset(txBuf, 0, commandLen);
    memset(rxBuf, 0, commandLen);

    txBuf[0] = command;
    // printf("channel %d:\n", channel);
    printf("read %d bytes:\n", spiXfer(handle, txBuf, rxBuf, commandLen));

    for (int i = 0; i < commandLen; ++i)
    {
        // printf("0x%X ", rxBuf[i]);
    }
    // printf("\n");

    uint16_t result = (rxBuf[0] & 0x01) << 9;
    result |= (rxBuf[1] & 0xFF) << 1;
    result |= (rxBuf[2] & 0x80) >> 7;
    result &= 0x3FF;
    // printf("value = %d(%fV)\n", result, (REFERENCE_VOLTAGE * (float)result) / 1024.f);
    return result;
}

static int analogRead(int c)
{

    switch (c)
    {
    case _yp:
    {
        return readChannel(1);
    }
    break;
    case _xm:
    {
        return readChannel(0);
    }
    break;
    default:
        printf("analog Read error!!!\n");
        break;
    }

    return 0;
}