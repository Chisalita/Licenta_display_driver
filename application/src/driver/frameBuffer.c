#include "frameBuffer.h"

#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

#ifndef NO_FRAME_BUFFER
#include <bcm_host.h>

static DISPMANX_DISPLAY_HANDLE_T display = NULL;
static DISPMANX_RESOURCE_HANDLE_T screen_resource = NULL;
static VC_RECT_T rect1;
#else
#include <stdlib.h>

static uint16_t *fbp = NULL;
#endif

static uint16_t frameBufferWidth, frameBufferHeight;

int frameBuffer_initFrameBuffer(uint16_t width, uint16_t height)
{
    frameBufferWidth = width;
    frameBufferHeight = height;

#ifndef NO_FRAME_BUFFER
    DISPMANX_MODEINFO_T display_info;
    uint32_t image_prt;
    int ret;

    bcm_host_init();

    display = vc_dispmanx_display_open(0);
    if (!display)
    {
        printf("Unable to open primary display\n");
        return -1;
    }
    ret = vc_dispmanx_display_get_info(display, &display_info);
    if (ret)
    {
        printf("Unable to get primary display information\n");
        frameBuffer_deinitFrameBuffer();
        return -1;
    }

    screen_resource = vc_dispmanx_resource_create(VC_IMAGE_RGB565, frameBufferWidth, frameBufferHeight, &image_prt);
    if (!screen_resource)
    {
        printf("Unable to create screen buffer\n");
        frameBuffer_deinitFrameBuffer();
        return -1;
    }

    ret = vc_dispmanx_rect_set(&rect1, 0, 0, frameBufferWidth, frameBufferHeight);
    if (ret)
    {
        frameBuffer_deinitFrameBuffer();
        return ret;
    }

#else
    fbp = malloc(sizeof(uint16_t) * frameBufferWidth * frameBufferHeight);
    if (!fbp)
    {
        return -1;
    }
    memset(fbp, 0, sizeof(uint16_t) * frameBufferWidth * frameBufferHeight);
#endif

    return 0;
}

int frameBuffer_deinitFrameBuffer(void)
{

#ifdef NO_FRAME_BUFFER
    if (fbp)
    {
        free(fbp);
        fbp = NULL;
    }
#else
    if (screen_resource)
    {
        vc_dispmanx_resource_delete(screen_resource);
    }

    if (display)
    {
        vc_dispmanx_display_close(display);
    }
#endif

    return 0;
}

int frameBuffer_getActualFbDim(int *width, int *height)
{
    int fbfd = 0;

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd < 0)
    {
        printf("Unable to open display\n");
        return -1;
    }
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo))
    {
        printf("Unable to get display information\n");
        close(fbfd);
        return -1;
    }
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo))
    {
        printf("Unable to get display information\n");
        close(fbfd);
        return -1;
    }

    if ((vinfo.bits_per_pixel != 16) &&
        (vinfo.bits_per_pixel != 24) &&
        (vinfo.bits_per_pixel != 32))
    {
        printf("only 16, 24 and 32 bits per pixels supported\n");
        close(fbfd);
        return -1;
    }

    printf("resolution = %dx%d\n", vinfo.xres, vinfo.yres);

    if (width != NULL)
    {
        *width = vinfo.width;
    }
    if (height != NULL)
    {
        *height = vinfo.height;
    }
    close(fbfd);
    return 0;
}

void frameBuffer_drawPixel(uint16_t x, uint16_t y, uint16_t color)
{
#ifdef NO_FRAME_BUFFER

    if ((y > frameBufferHeight) || (y < 0) || (x > frameBufferWidth) || (x < 0))
    {
        return;
    }

    fbp[(y * frameBufferWidth) + x] = color;
#endif
}

int frameBuffer_getFrame(void *outFrameBuff)
{

#ifdef NO_FRAME_BUFFER
    memcpy(outFrameBuff, fbp, sizeof(uint16_t) * frameBufferWidth * frameBufferHeight);
    return 0;
#else

    int ret = vc_dispmanx_snapshot(display, screen_resource, 0);
    if (ret)
    {
        return ret;
    }

    ret = vc_dispmanx_resource_read_data(screen_resource, &rect1, outFrameBuff, frameBufferWidth * sizeof(uint16_t));
    if (ret)
    {
        return ret;
    }

    return 0;
#endif
}
