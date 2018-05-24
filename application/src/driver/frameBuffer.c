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
#endif

static uint16_t fbp[320 * 240];

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
        // exit(EXIT_FAILURE);
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
    fbp[(y * 320) + x] = color;
}

//TODO: optimize by makeing it continuous (not just one frame)
int frameBuffer_getFrame(void *outFrameBuff)
{

#ifdef NO_FRAME_BUFFER
    memcpy(outFrameBuff, fbp, sizeof(fbp));
    return 0;
#else

    DISPMANX_DISPLAY_HANDLE_T display;
    DISPMANX_MODEINFO_T display_info;
    DISPMANX_RESOURCE_HANDLE_T screen_resource;
    uint32_t image_prt;
    VC_RECT_T rect1;
    int ret;

    bcm_host_init();

    display = vc_dispmanx_display_open(0);
    if (!display)
    {
        printf("Unable to open primary display");
        return -1;
    }
    ret = vc_dispmanx_display_get_info(display, &display_info);
    if (ret)
    {
        printf("Unable to get primary display information");
        return -1;
    }
    // printf("Primary display is %d x %d", display_info.width, display_info.height);

    screen_resource = vc_dispmanx_resource_create(VC_IMAGE_RGB565, 320, 240, &image_prt); //TODO: get screen size
    if (!screen_resource)
    {
        printf("Unable to create screen buffer");
        vc_dispmanx_display_close(display);
        return -1;
    }

    memset(fbp, 0, sizeof(fbp)); //TODO:remove?

    vc_dispmanx_rect_set(&rect1, 0, 0, 320, 240); //TODO: get screen size
    while (1)
    {
        ret = vc_dispmanx_snapshot(display, screen_resource, 0);
        vc_dispmanx_resource_read_data(screen_resource, &rect1, /*fbp*/ outFrameBuff, 320 * 16 / 8); //vinfo.xres * vinfo.bits_per_pixel / 8);
        usleep(25 * 1000);                                                                           //TODO: process touch here?
        break;
    }

    ret = vc_dispmanx_resource_delete(screen_resource); //TODO: do this on errors too
    vc_dispmanx_display_close(display);
    return 0;
#endif
}
