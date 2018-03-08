#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <pigpio.h>
#include <unistd.h>

#define REFERENCE_VOLTAGE (3.3f)
#define NUMSAMPLES (1)
#define pinMode(p, m) gpioSetMode(p, m)
#define INPUT (PI_INPUT)
#define OUTPUT (PI_OUTPUT)
#define HIGH (PI_HIGH)
#define LOW (PI_LOW)
#define digitalWrite(p, l) gpioWrite(p, l)

#define _yp (2)
#define _xm (6)
#define _ym (27) //17
#define _xp (26) //16//TODO: change this when integrating


// Calibrate values
#define TS_MINX 125
#define TS_MINY 85
#define TS_MAXX 965
#define TS_MAXY 905


#define TFT_WIDTH (240)
#define TFT_HEIGHT (320)

//TODO make sure to do as the example with pull resistors!!!

int _rxplate = 0;//300; //WHAT IS THIS?

int handle; //make it local!!!

int readChannel(uint8_t channel);
void getPoint(void);
void getPoint_modif(void);
long map(long x, long in_min, long in_max, long out_min, long out_max);

int main(int argc, char *argv[])
{
    printf("Touch me!\n");

    int res = gpioInitialise();
    if (res < 0)
    {
        return 1;
    }

    handle = spiOpen(0, 96000, 0);
    if (handle < 0)
    {
        //bad!
        printf("Error opening SPI!\n");
    }

    while (1)
    {
        usleep(1000 * 100);
        printf("do\n");
        /*readChannel(0);
        readChannel(1);*/
        getPoint();
    }

    spiClose(handle);

    return 0;
}

int readChannel(uint8_t channel)
{
    uint8_t command = 0b11 << 6;
    command |= (channel & 0x07) << 3;
    const int commandLen = 3;
    char txBuf[commandLen];
    char rxBuf[commandLen];

    memset(txBuf, 0, commandLen);
    memset(rxBuf, 0, commandLen);

    txBuf[0] = command;
    printf("channel %d:\n", channel);
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
    printf("value = %d(%fV)\n", result, (REFERENCE_VOLTAGE * (float)result) / 1024.f);
    return result;
}

int analogRead(int c)
{


    switch (c)
    {
    case _yp:
    {
        // gpioSetPullUpDown(_yp, PI_PUD_DOWN);
        return readChannel(1);
    }
    break;
    case _xm:
    {
        // gpioSetPullUpDown(_xm, PI_PUD_DOWN);
        return readChannel(0);
    }
    break;
    default:
        printf("analog Read error!!!\n");
        break;
    }

    return 0;
}

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

void getPoint(void)
{
    int x, y, z;
    int samples[NUMSAMPLES];
    uint8_t i, valid;

    valid = 1;

    pinMode(_yp, INPUT);
    pinMode(_ym, INPUT);
    gpioSetPullUpDown(_yp, PI_PUD_OFF);
    gpioSetPullUpDown(_ym, PI_PUD_OFF);

    pinMode(_xp, OUTPUT);
    pinMode(_xm, OUTPUT);
    digitalWrite(_xp, HIGH);
    digitalWrite(_xm, LOW);

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

    pinMode(_xp, INPUT);
    pinMode(_xm, INPUT);
    gpioSetPullUpDown(_xp, PI_PUD_OFF);
    gpioSetPullUpDown(_xm, PI_PUD_OFF);


    pinMode(_yp, OUTPUT);
    digitalWrite(_yp, HIGH);
    pinMode(_ym, OUTPUT);
    digitalWrite(_ym, LOW);////////TODO: remove if not working?


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
    pinMode(_xp, OUTPUT);
    digitalWrite(_xp, LOW);

    // Set Y- to VCC
    pinMode(_ym, OUTPUT); //TODO: remove after tested
    digitalWrite(_ym, HIGH);

    // Hi-Z X- and Y+
    pinMode(_yp, OUTPUT);
    digitalWrite(_yp, LOW);

    pinMode(_yp, INPUT);

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

    //return TSPoint(x, y, z);

    // *** SPFD5408 change -- Begin
    // SPFD5408 change, because Y coordinate is inverted in this controller
    //return TSPoint(x, 1023 - y, z);
    // -- End
    printf("Normal: x = %d, y = %d, z = %d\n", x, y, z);
    printf("Inverted: x = %d, y = %d, z = %d\n", x, 1023 - y, z);

    long px = map(x, TS_MINX, TS_MAXX, 0, TFT_WIDTH);
    long py = map( 1023 - y, TS_MINY, TS_MAXY, 0, TFT_HEIGHT);


    printf("Maped Inverted: x = %ld, y = %ld, z = %d\n", px, py, z);
}


void getPoint_modif(void)
{
    int x, y, z;
    int samples[NUMSAMPLES];
    uint8_t i, valid;

    valid = 1;

    pinMode(_yp, INPUT);
    pinMode(_ym, INPUT);

    /*digitalWrite(_yp, LOW);
    digitalWrite(_ym, LOW);
    */
    //pull down
    //////////gpioSetPullUpDown(_yp, PI_PUD_DOWN);/////////
    //////////gpioSetPullUpDown(_ym, PI_PUD_DOWN);////////
    gpioSetPullUpDown(_yp, PI_PUD_OFF);/////////
    gpioSetPullUpDown(_ym, PI_PUD_OFF);////////

    pinMode(_xp, OUTPUT);
    pinMode(_xm, OUTPUT);
    digitalWrite(_xp, HIGH);
    digitalWrite(_xm, LOW);

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



    pinMode(_xp, INPUT);
    pinMode(_xm, INPUT);
    //digitalWrite(_xp, LOW);
    ////////gpioSetPullUpDown(_xp, PI_PUD_DOWN);////////
    gpioSetPullUpDown(_xp, PI_PUD_OFF);////////
    gpioSetPullUpDown(_xm, PI_PUD_OFF);////////

    pinMode(_yp, OUTPUT);
    digitalWrite(_yp, HIGH);
    pinMode(_ym, OUTPUT);
    digitalWrite(_ym, LOW);////////


    for (i = 0; i < NUMSAMPLES; i++)
    {
        samples[i] = analogRead(_xp);/////xm
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

    /*
    // Set X+ to ground
    pinMode(_xp, OUTPUT);
    digitalWrite(_xp, LOW);

    // Set Y- to VCC
    digitalWrite(_ym, HIGH);

    // Hi-Z X- and Y+
    digitalWrite(_yp, LOW);
    
    gpioSetPullUpDown(_yp, PI_PUD_OFF); //HiZ (not sure if good)
    pinMode(_yp, INPUT);
    */////////////

    // Set X+ to ground
    pinMode(_xp, OUTPUT);
    digitalWrite(_xp, LOW);

    // Set Y- to VCC
    pinMode(_ym, OUTPUT); //TODO: remove after tested
    digitalWrite(_ym, HIGH);

    // Hi-Z X- and Y+
    pinMode(_yp, INPUT);
    pinMode(_xm, INPUT);

    gpioSetPullUpDown(_xm, PI_PUD_OFF); //HiZ (not sure if good)
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

    //return TSPoint(x, y, z);

    // *** SPFD5408 change -- Begin
    // SPFD5408 change, because Y coordinate is inverted in this controller
    //return TSPoint(x, 1023 - y, z);
    // -- End
    printf("Normal: x = %d, y = %d, z = %d\n", x, y, z);
    printf("Inverted: x = %d, y = %d, z = %d\n", x, 1023 - y, z);

    long px = map(x, TS_MINX, TS_MAXX, 0, TFT_WIDTH);
    long py = map( 1023 - y, TS_MINY, TS_MAXY, 0, TFT_HEIGHT);


    printf("Maped Inverted: x = %ld, y = %ld, z = %d\n", px, py, z);
}

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}