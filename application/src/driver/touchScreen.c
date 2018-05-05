#include "touchScreen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pigpio.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

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
#define TOUCH_PRESS_TRESHOLD (350)

#define DEFUALUT_DISPLAY_NAME ":0.0"
#define DEFAULT_XAUTHORITY_VALUE "~/.Xauthority"

typedef enum {
    STATE_CLICK_RELEASED,
    STATE_CLICK_PRESSED
} touchState;

static int analogRead(int c);
static long map(long x, long in_min, long in_max, long out_min, long out_max);
static int readChannel(uint8_t channel);
static void click(Display *display, int button);
static void releaseClick(Display *display, int button);
static void moveMouse(Display *display, int x, int y);
static void processTouchState(int x, int y, int z);
static void updateFrameBufferSizes(Display *display);

static int handle;       //make it local!!!
static int _rxplate = 0; //300; //WHAT IS THIS? (resistenta cred)
static int fb_width = 0;
static int fb_height = 0;
static int fbToActualSizeRatioX = 0;
static int fbToActualSizeRatioY = 0;
static char *displayName = DEFUALUT_DISPLAY_NAME;

Display *_display = NULL;
touchState currentTouchState = STATE_CLICK_RELEASED;

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

int touchScreen_initTouch(int frameBufferWidth, int frameBufferHeight)
{
    handle = spiOpen(0, 96000, 0);
    if (handle < 0)
    {
        printf("Error opening SPI!\n");
        return handle;
    }

    currentTouchState = STATE_CLICK_RELEASED;
    fb_width = frameBufferWidth;
    fb_height = frameBufferHeight;

    fbToActualSizeRatioX = fb_width / display_getDisplayWidth();
    fbToActualSizeRatioY = fb_height / display_getDisplayHeight();

    setenv("XAUTHORITY", DEFAULT_XAUTHORITY_VALUE, 0);

    char *dName = getenv("DISPLAY");
    if (dName)
    {
        printf("yes\n");
        displayName = dName;
    }
    else
    {
        setenv("DISPLAY", DEFUALUT_DISPLAY_NAME, 0);
    }
    printf("DISPLAY=%s\n", displayName);

    pid_t i = fork();
    if (i == 0)
    {
        int sc = execlp("xhost", "xhost", "+SI:localuser:root", NULL);
        printf("sc  = %d\n", sc);
        exit(1);
    }
    // display = XOpenDisplay(0);
    // Window root_window;
    // root_window = XRootWindow(display, 0);

    return 0;
}

int touchScreen_deinitTouch()
{
    if (_display)
    {
        XCloseDisplay(_display);
    }
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

    if (display_getDisplayRotation() == ROTATION_0_DEGREES || display_getDisplayRotation() == ROTATION_180_DEGREES)
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

    long px = map(x, tsMinx, tsMaxx, 0, display_getDisplayWidth());
    long py = map(y, tsMiny, tsMaxy, 0, display_getDisplayHeight());

    // printf("Maped Inverted: x = %ld, y = %ld, z = %d\n", px, py, z);

    int16_t pointWidth = 5;
    int16_t pointHeight = 5;

    //TODO: see what is needed
    gpioSetMode(_yp, PI_OUTPUT);
    gpioSetMode(_xm, PI_OUTPUT);
    gpioSetMode(_ym, PI_OUTPUT);
    gpioSetMode(_xp, PI_OUTPUT);
    processTouchState(px, py, z);
    // display_fillRect(px, py, pointWidth, pointHeight, GREEN);
    // click(px, py, Button1);
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
    int readBytes = spiXfer(handle, txBuf, rxBuf, commandLen);
    // printf("read %d bytes:\n", readBytes);

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

static void moveMouse(Display *display, int x, int y)
{
    if (display == NULL)
    {
        return;
    }

    int screenX = fbToActualSizeRatioX * x;
    int screenY = fbToActualSizeRatioY * y;

    Window root_window;
    root_window = XRootWindow(display, 0);

    XSelectInput(display, root_window, KeyReleaseMask);
    XWarpPointer(display, None, root_window, 0, 0, 0, 0, screenX, screenY);
    XFlush(display); // Flushes the output buffer, therefore updates the cursor's position
}

// Simulate mouse click
static void click(Display *display, int button)
{

    if (display == NULL)
    {
        return;
    }
    //     struct timespec ts;
    // //  int milliseconds = 1;
    //     // ts.tv_sec = milliseconds / 1000;
    //     // ts.tv_nsec = (milliseconds % 1000) * 1000000;
    //     ts.tv_sec = 0;
    //     ts.tv_nsec = 1000;
    //     nanosleep(&ts, NULL);

    // Create and setting up the event
    XEvent event;
    memset(&event, 0, sizeof(event));
    event.xbutton.button = button;
    event.xbutton.same_screen = True;
    event.xbutton.subwindow = DefaultRootWindow(display);
    while (event.xbutton.subwindow)
    {
        event.xbutton.window = event.xbutton.subwindow;
        XQueryPointer(display, event.xbutton.window,
                      &event.xbutton.root, &event.xbutton.subwindow,
                      &event.xbutton.x_root, &event.xbutton.y_root,
                      &event.xbutton.x, &event.xbutton.y,
                      &event.xbutton.state);
    }
    // Press
    event.type = ButtonPress;
    if (XSendEvent(display, PointerWindow, True, ButtonPressMask, &event) == 0)
        fprintf(stderr, "Error to send the event!\n");
    XFlush(display);
    /*   printf("YA\n");
    // sleep(1);
    printf("wow\n");
    //  nanosleep(&ts, NULL);

   // Release
    event.type = ButtonRelease;
    if (XSendEvent(display, PointerWindow, True, ButtonReleaseMask, &event) == 0)
        fprintf(stderr, "Error to send the event!\n");
    XFlush(display);
    // sleep(1);
    // nanosleep(&ts, NULL);*/
}

static void releaseClick(Display *display, int button)
{

    if (display == NULL)
    {
        return;
    }
    // Create and setting up the event
    XEvent event;
    memset(&event, 0, sizeof(event));
    event.xbutton.button = button;
    event.xbutton.same_screen = True;
    event.xbutton.subwindow = DefaultRootWindow(display);
    while (event.xbutton.subwindow)
    {
        event.xbutton.window = event.xbutton.subwindow;
        XQueryPointer(display, event.xbutton.window,
                      &event.xbutton.root, &event.xbutton.subwindow,
                      &event.xbutton.x_root, &event.xbutton.y_root,
                      &event.xbutton.x, &event.xbutton.y,
                      &event.xbutton.state);
    }

    // Release
    event.type = ButtonRelease;
    if (XSendEvent(display, PointerWindow, True, ButtonReleaseMask, &event) == 0)
        fprintf(stderr, "Error to send the event!\n");
    XFlush(display);
}

static void updateFrameBufferSizes(Display *display)
{

    if (display == NULL)
    {
        return;
    }

    Screen *screen = DefaultScreenOfDisplay(display);

    int x = WidthOfScreen(screen);
    int y = HeightOfScreen(screen);

    fb_width = x;
    fb_height = y;

    fbToActualSizeRatioX = fb_width / display_getDisplayWidth();
    fbToActualSizeRatioY = fb_height / display_getDisplayHeight();
}

static void processTouchState(int x, int y, int z)
{

    if (_display == NULL)
    {
        // printf("try\n");
        _display = XOpenDisplay(displayName);
        updateFrameBufferSizes(_display);
    }

    printf("_display = %p\n", _display);

    // printf("z = %d\n", z);
    touchState newTouchState = (z >= TOUCH_PRESS_TRESHOLD) ? STATE_CLICK_PRESSED : STATE_CLICK_RELEASED;

    switch (currentTouchState)
    {
    case STATE_CLICK_PRESSED:
        if (newTouchState == STATE_CLICK_PRESSED)
        {
            moveMouse(_display, x, y);
        }
        else
        {
            releaseClick(_display, Button1);
            printf("pressed -> released\n");
        }
        break;
    case STATE_CLICK_RELEASED:
        if (newTouchState == STATE_CLICK_PRESSED)
        {
            moveMouse(_display, x, y);
            click(_display, Button1);
            printf("released -> pressed\n");
        }
        break;
    }

    currentTouchState = newTouchState;
}