#include "touchScreen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#ifdef USING_PIGPIO_LIB
#include <pigpio.h>
#else
#include <bcm2835.h>
#endif
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include "ili9341Shield.h"
#include "display.h"

#define REFERENCE_VOLTAGE (3.3f)
#define NUMSAMPLES (1)

#define _yp (2)
#define _xm (6)
#define _ym (27)
#define _xp (26)

/*// Calibrate values
#define TS_MINX 90  //75  //125
#define TS_MINY 250 //280 //85
#define TS_MAXX 600 //650 //965
#define TS_MAXY 915 //935 //905
*/
// Calibrate values
#define TS_MINX 110
#define TS_MINY 220
#define TS_MAXX 625
#define TS_MAXY 930

/* R&D
#define TS_MINX 51  //125
#define TS_MINY 200 //85
#define TS_MAXX 650 //965
#define TS_MAXY 880 //905
*/
#define TOUCH_PRESS_TRESHOLD (230) //(300)

#define DEFUALUT_DISPLAY_NAME ":0.0"

typedef enum
{
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
static int drop_root_privileges(void);

static int handle;
static int _rxplate = 0; //300;
static int fb_width = 0;
static int fb_height = 0;
static float fbToActualSizeRatioX = 0;
static float fbToActualSizeRatioY = 0;
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

static int drop_root_privileges(void)
{ // returns 0 on success and -1 on failure
    gid_t gid;
    uid_t uid;

    // no need to "drop" the privileges that you don't have in the first place!
    if (getuid() != 0)
    {
        return 0;
    }

    // when your program is invoked with sudo, getuid() will return 0 and you
    // won't be able to drop your privileges
    if ((uid = getuid()) == 0)
    {
        const char *sudo_uid = getenv("SUDO_UID");
        if (sudo_uid == NULL)
        {
            printf("environment variable `SUDO_UID` not found\n");
            return -1;
        }
        errno = 0;
        uid = (uid_t)strtoll(sudo_uid, NULL, 10);
        if (errno != 0)
        {
            perror("under-/over-flow in converting `SUDO_UID` to integer");
            return -1;
        }
    }

    // again, in case your program is invoked using sudo
    if ((gid = getgid()) == 0)
    {
        const char *sudo_gid = getenv("SUDO_GID");
        if (sudo_gid == NULL)
        {
            printf("environment variable `SUDO_GID` not found\n");
            return -1;
        }
        errno = 0;
        gid = (gid_t)strtoll(sudo_gid, NULL, 10);
        if (errno != 0)
        {
            perror("under-/over-flow in converting `SUDO_GID` to integer");
            return -1;
        }
    }

    if (setgid(gid) != 0)
    {
        perror("setgid");
        return -1;
    }
    if (setuid(uid) != 0)
    {
        perror("setgid");
        return -1;
    }

    // change your directory to somewhere else, just in case if you are in a
    // root-owned one (e.g. /root)
    if (chdir("/") != 0)
    {
        perror("chdir");
        return -1;
    }

    // check if we successfully dropped the root privileges
    if (setuid(0) == 0 || seteuid(0) == 0)
    {
        printf("could not drop root privileges!\n");
        return -1;
    }

    return 0;
}

static void trySettingAccessToXserver(void)
{

    char *dName = getenv("DISPLAY");
    if (dName)
    {
        displayName = dName;
    }
    else
    {
        setenv("DISPLAY", DEFUALUT_DISPLAY_NAME, 0);
    }

    pid_t i = fork();
    if (i == 0)
    {
        int dr = drop_root_privileges();
        if (dr < 0)
        {
            exit(dr);
        }

        while (1)
        {
            pid_t j = fork();
            if (j == 0)
            {
                int sc = execlp("xhost", "xhost", "+SI:localuser:root", NULL);
                printf("ERROR on execlp (%d)\n", sc);
                exit(1);
            }
            else if (j < 0)
            { //error
                exit(j);
            }
            else
            {
                int wstatus = 0;
                pid_t child = waitpid(j, &wstatus, 0);
                if (child < 0)
                {
                    perror("waitpid");
                    exit(1);
                }
                if (WIFEXITED(wstatus))
                {

                    int st = WEXITSTATUS(wstatus);

                    if (st == 0)
                    {
                        // the command executed successfully just exit now
                        exit(0);
                    }
                }

                sleep(1);
            }
        }
    }
    else if (i < 0)
    {
        perror("Error forking ");
    }
}

int touchScreen_initTouch(int frameBufferWidth, int frameBufferHeight)
{

#ifdef USING_PIGPIO_LIB
    handle = spiOpen(0, 96000, 0);
    if (handle < 0)
    {
        printf("Error opening SPI!\n");
        return handle;
    }
#else
    handle = bcm2835_spi_begin();
    if (!handle)
    {
        printf("Error opening SPI!\n");
        return handle;
    }

    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);   // The default
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                // The default
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64); // 6.250MHz
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                   // The default
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);   // the default
#endif

    currentTouchState = STATE_CLICK_RELEASED;
    fb_width = frameBufferWidth;
    fb_height = frameBufferHeight;

    fbToActualSizeRatioX = (float)fb_width / (float)display_getDisplayWidth();
    fbToActualSizeRatioY = (float)fb_height / (float)display_getDisplayHeight();

    trySettingAccessToXserver();
    return 0;
}

int touchScreen_deinitTouch(void)
{
    if (_display)
    {
        XCloseDisplay(_display);
    }

#ifdef USING_PIGPIO_LIB
    return spiClose(handle);
#else
    bcm2835_spi_end();
    return !bcm2835_close();
#endif
}

void touchScreen_getPoint(void)
{
    int x, y, z;
    int samples[NUMSAMPLES];
    uint8_t i, valid;

    valid = 1;

    GPIO_SET_MODE(_yp, GPIO_INPUT);
    GPIO_SET_MODE(_ym, GPIO_INPUT);
    GPIO_SET_PUD(_yp, GPIO_PUD_OFF);
    GPIO_SET_PUD(_ym, GPIO_PUD_OFF);

    GPIO_SET_MODE(_xp, GPIO_OUTPUT);
    GPIO_SET_MODE(_xm, GPIO_OUTPUT);
    GPIO_WRITE(_xp, GPIO_HIGH);
    GPIO_WRITE(_xm, GPIO_LOW);

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

    GPIO_SET_MODE(_xp, GPIO_INPUT);
    GPIO_SET_MODE(_xm, GPIO_INPUT);
    GPIO_SET_PUD(_xp, GPIO_PUD_OFF);
    GPIO_SET_PUD(_xm, GPIO_PUD_OFF);

    GPIO_SET_MODE(_yp, GPIO_OUTPUT);
    GPIO_SET_MODE(_ym, GPIO_OUTPUT);
    GPIO_WRITE(_yp, GPIO_HIGH);
    GPIO_WRITE(_ym, GPIO_LOW);

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

#ifdef INVERTED_Y
    y = samples[NUMSAMPLES / 2];
#else
    y = (1023 - samples[NUMSAMPLES / 2]);
#endif
    // Set X+ to ground
    GPIO_SET_MODE(_xp, GPIO_OUTPUT);
    GPIO_WRITE(_xp, GPIO_LOW);

    // Set Y- to VCC
    GPIO_WRITE(_ym, GPIO_HIGH);

    // Hi-Z Y+
    GPIO_SET_MODE(_yp, GPIO_INPUT);
    GPIO_SET_PUD(_yp, GPIO_PUD_OFF);

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

    int tsMaxx = TS_MAXX;
    int tsMaxy = TS_MAXY;
    int tsMinx = TS_MINX;
    int tsMiny = TS_MINY;

    if (display_getDisplayRotation() == ROTATION_0_DEGREES || display_getDisplayRotation() == ROTATION_180_DEGREES)
    {
        int aux = x;
        x = y;
        y = aux;

        tsMaxx = TS_MAXY;
        tsMaxy = TS_MAXX;
        tsMinx = TS_MINY;
        tsMiny = TS_MINX;
    }


    //  long px = map(x, (long) (0.99 * (float)(tsMinx)), (long) (1.01 * (float)(tsMaxx)), 0, fb_width);
    // long py = map(y, (long) (0.99 * (float)(tsMiny)), (long) (1.01 * (float)(tsMaxy)), 0, fb_height);

    long px = map(x, tsMinx, tsMaxx, 0, fb_width);
    long py = map(y, tsMiny, tsMaxy, 0, fb_height);

    //restore gpio modes for next draw
    GPIO_SET_MODE(_yp, GPIO_OUTPUT);
    GPIO_SET_MODE(_xm, GPIO_OUTPUT);
    processTouchState(px, py, z);
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

#ifdef USING_PIGPIO_LIB
    int readBytes = spiXfer(handle, txBuf, rxBuf, commandLen);
    if (readBytes < 0)
    {
        printf("Error reading from SPI!\n");
    }
#else
    bcm2835_spi_transfernb(txBuf, rxBuf, commandLen);
#endif

    uint16_t result = (rxBuf[0] & 0x01) << 9;
    result |= (rxBuf[1] & 0xFF) << 1;
    result |= (rxBuf[2] & 0x80) >> 7;
    result &= 0x3FF;
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

    if ((x < 0) || (x > fb_width) ||
        (y < 0) || (y > fb_height))
    {
        printf("nooooo!\n");
        printf("screenX =%d\n", x);
        printf("screenY =%d\n", y);
        return;
    }

    Window root_window;
    root_window = XRootWindow(display, 0);

    XSelectInput(display, root_window, KeyReleaseMask);
    XWarpPointer(display, None, root_window, 0, 0, 0, 0, x, y);
    XFlush(display); // Flushes the output buffer, therefore updates the cursor's position
}

// Simulate mouse click
static void click(Display *display, int button)
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
    // Press
    event.type = ButtonPress;
    if (XSendEvent(display, PointerWindow, True, ButtonPressMask, &event) == 0)
    {
        fprintf(stderr, "Error to send the event!\n");
    }
    XFlush(display);
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
    {
        fprintf(stderr, "Error to send the event!\n");
    }
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

    fbToActualSizeRatioX = (float)fb_width / (float)display_getDisplayWidth();
    fbToActualSizeRatioY = (float)fb_height / (float)display_getDisplayHeight();
}

static void processTouchState(int x, int y, int z)
{

    if (_display == NULL)
    {
        _display = XOpenDisplay(displayName);
        if (_display)
        {
            updateFrameBufferSizes(_display);
        }
    }

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