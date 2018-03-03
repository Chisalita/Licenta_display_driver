
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    char *program = argv[0];
    char *fbdevice = "/dev/fb0";

    /*  int opt = 0;

    //--------------------------------------------------------------------

    while ((opt = getopt(argc, argv, "d:")) != -1)
    {
        switch (opt)
        {
        case 'd':

            fbdevice = optarg;
            break;

        default:

            fprintf(stderr,
                    "Usage: %s [-d device] [-p pngname]\n",
                    program);
            exit(EXIT_FAILURE);
            break;
        }
    }
*/
    //--------------------------------------------------------------------

    int fbfd = open(fbdevice, O_RDWR);

    if (fbfd == -1)
    {
        fprintf(stderr,
                "%s: cannot open framebuffer - %s",
                program,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct fb_fix_screeninfo finfo;

    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        fprintf(stderr,
                "%s: reading framebuffer fixed information - %s",
                program,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct fb_var_screeninfo vinfo;

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        fprintf(stderr,
                "%s: reading framebuffer variable information - %s",
                program,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((vinfo.bits_per_pixel != 16) &&
        (vinfo.bits_per_pixel != 24) &&
        (vinfo.bits_per_pixel != 32))
    {
        fprintf(stderr, "%s: only 16, 24 and 32 ", program);
        fprintf(stderr, "bits per pixels supported\n");
        exit(EXIT_FAILURE);
    }

    printf("resolution = %dx%d\n", vinfo.xres, vinfo.yres);
    /*vinfo.xres = 1920;
    vinfo.yres = 1080;

    printf("yoffset = %d\n", vinfo.yoffset);
    vinfo.yoffset = 00;
    printf("result = %d\n",ioctl (fbfd, FBIOPUT_VSCREENINFO, &vinfo));

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        fprintf(stderr,
                "%s: reading framebuffer variable information - %s",
                program,
                strerror(errno));
        exit(EXIT_FAILURE);
    }


    printf("resolution = %dx%d\n", vinfo.xres, vinfo.yres);
    */

    printf("bpp: %d\n", vinfo.bits_per_pixel);

    printf("R: len = %d, offset = %d, msb_right = %d\n", vinfo.red.length, vinfo.red.offset, vinfo.red.msb_right);
    printf("G: len = %d, offset = %d, msb_right = %d\n", vinfo.green.length, vinfo.green.offset, vinfo.green.msb_right);
    printf("B: len = %d, offset = %d, msb_right = %d\n", vinfo.blue.length, vinfo.blue.offset, vinfo.blue.msb_right);
    printf("T: len = %d, offset = %d, msb_right = %d\n", vinfo.transp.length, vinfo.transp.offset, vinfo.transp.msb_right);

    void *memp = mmap(NULL, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

    close(fbfd);
    fbfd = -1;

    if (memp == MAP_FAILED)
    {
        fprintf(stderr,
                "%s: failed to map framebuffer device to memory - %s",
                program,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    uint8_t *fbp = memp;
    memp = NULL;

    int line_size = vinfo.xres * vinfo.bits_per_pixel / 8;
    int buffer_size = line_size * vinfo.yres;
    /*
    for(int i=0; i<buffer_size; i++){
        *(fbp+i) = i%236;
    }
*/

    char *outfile = "out";

    int outfd;

    int wr = 1;
    if (wr)
    {

        outfd = creat(outfile, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

        if (outfd == -1)
        {
            fprintf(stderr,
                    "%s: cannot open outputfile - %s",
                    program,
                    strerror(errno));
            exit(EXIT_FAILURE);
        }

        write(outfd, fbp /*+ (vinfo.yoffset * line_size)*/, buffer_size);
    }
    else
    {
        outfile = "tmp";
        outfd = open(outfile, O_RDWR);

        if (outfd == -1)
        {
            fprintf(stderr,
                    "%s: cannot open outfd - %s",
                    program,
                    strerror(errno));
            exit(EXIT_FAILURE);
        }

        int r = read(outfd, fbp /* + (vinfo.yoffset * line_size)*/, buffer_size);
        printf("read %d bytes\n", r);
    }

    close(outfd);

    munmap(fbp, finfo.smem_len);

    //--------------------------------------------------------------------

    return 0;
}

/*
// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t color565(uint32_t value, struct fb_var_screeninfo *info)
{

    uint8_t r = value & (0xFF << (32 - info->red.offset - info->red.length)); //0xff is not generic...
    uint8_t g = value & (0xFF << (32 - info->green.offset - info->green.length));
    uint8_t b = value & (0xFF << (32 - info->blue.offset - info->blue.length));

    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}*/