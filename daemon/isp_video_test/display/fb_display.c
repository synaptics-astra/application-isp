/******************************************************************************\
|* Copyright (c) <2024> by VeriSilicon Holdings Co., Ltd. ("VeriSilicon")     *|
|* All Rights Reserved.                                                       *|
|*                                                                            *|
|* The material in this file is confidential and contains trade secrets       *|
|* of VeriSilicon.  This is proprietary information owned or licensed by      *|
|* VeriSilicon.  No part of this work may be disclosed, reproduced, copied,   *|
|* transmitted, or used in any way for any purpose, without the express       *|
|* written permission of VeriSilicon.                                         *|
|*                                                                            *|
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <assert.h>
#include <memory.h>
#include <linux/videodev2.h>
#include <linux/fb.h>

#include "fb_display.h"
#include "display.h"
#include "yuv_to_rgb.h"

static void *display_fb_open(int idx, int fourcc)
{
    int fd;
    fb_dev_t* fb_dev = (fb_dev_t*)malloc(sizeof(fb_dev_t));
    char fb_dev_name[64];
    sprintf(fb_dev_name, "/dev/fb%d", idx);
    fd = open(fb_dev_name, O_RDWR);
    if (!fd) {
        printf("Error: cannot open framebuffer device\n");
        goto err_free_handle;
    }
    fb_dev->fd = fd;

    struct fb_fix_screeninfo finfo;
    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo))  {
        printf("Error:reading fixed information\n");
        goto err_free_handle;
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo))   {
        printf("Error: reading variable information\n");
        goto err_free_handle;
    }

    printf("[offset,length,msb_right]: R[%u,%u,%u],G[%u,%u,%u],B[%u,%u,%u]\n",
           vinfo.red.offset, vinfo.red.length, vinfo.red.msb_right,
           vinfo.green.offset, vinfo.green.length, vinfo.green.msb_right,
           vinfo.blue.offset, vinfo.blue.length, vinfo.blue.msb_right);

    printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
    printf("%dx%d, %d %d\n", vinfo.xres_virtual, vinfo.yres_virtual, vinfo.xoffset, vinfo.yoffset);

    fb_dev->xres = vinfo.xres;
    fb_dev->yres = vinfo.yres;

    fb_dev->fb_size = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    fb_dev->framebuffer = (unsigned char *)mmap(NULL,
        fb_dev->fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fb_dev->framebuffer == MAP_FAILED) {
        printf("Error: failed to map framebuffer device to memory\n");
        fb_dev->framebuffer = NULL;
        goto err_free_handle;
    }

    fb_dev->fbcache = (unsigned char *)malloc(fb_dev->fb_size);
    if (fb_dev->fbcache == NULL) {
        printf("Error: failed to alloc framebuffer cache memory\n");
        goto err_free_handle;
    }

    yuv_to_rgb_bt601_limit_range_init();

    return fb_dev;

err_free_handle:
    free(fb_dev);
    fb_dev = NULL;

    return NULL;
}

void display_fb_close(void *handle)
{
    fb_dev_t *fb_dev = (fb_dev_t*)handle;
    if (!fb_dev)
        return;

    if (fb_dev->framebuffer) {
        munmap(fb_dev->framebuffer, fb_dev->fb_size);
        fb_dev->framebuffer = NULL;
    }

    if (fb_dev->fbcache) {
        free(fb_dev->fbcache);
        fb_dev->fbcache = NULL;
    }

    if (fb_dev->fd > 0) {
        close(fb_dev->fd);
        fb_dev->fd = -1;
    }

    free(fb_dev);
}

int display_fb_show(void *handle, void *data, int width, int height, int fourcc)
{
    fb_dev_t *fb_dev = (fb_dev_t*)handle;
    if ((!fb_dev->framebuffer) || (!fb_dev->fbcache))
        return -1;

    switch(fourcc) {
        case V4L2_PIX_FMT_NV16:
            nv16_to_rgb32((uint32_t*)fb_dev->fbcache, data, width, height);
            break;
        case V4L2_PIX_FMT_YUYV:
            yuyv_to_rgb32((uint32_t*)fb_dev->fbcache, data, width, height);
            break;
        case V4L2_PIX_FMT_NV12:
            nv12_to_rgb32((uint32_t*)fb_dev->fbcache, data, width, height);
            break;
        case V4L2_PIX_FMT_BGR24:
            bgr24_to_rgb32((uint32_t*)fb_dev->fbcache, data, width, height);
            break;
        case V4L2_PIX_FMT_SBGGR8:
        case V4L2_PIX_FMT_SGBRG8:
        case V4L2_PIX_FMT_SGRBG8:
        case V4L2_PIX_FMT_SRGGB8:
            raw8_to_rgb32((uint32_t*)fb_dev->fbcache, data, width, height);
            break;
        case V4L2_PIX_FMT_SBGGR10:
        case V4L2_PIX_FMT_SGBRG10:
        case V4L2_PIX_FMT_SGRBG10:
        case V4L2_PIX_FMT_SRGGB10:
            raw16_to_rgb32((uint32_t*)fb_dev->fbcache, data, width, height, 10);
            break;
        case V4L2_PIX_FMT_SBGGR12:
        case V4L2_PIX_FMT_SGBRG12:
        case V4L2_PIX_FMT_SGRBG12:
        case V4L2_PIX_FMT_SRGGB12:
            raw16_to_rgb32((uint32_t*)fb_dev->fbcache, data, width, height, 12);
            break;
        default:
            printf("%s: Unsupported format: %d\n", __func__, fourcc);
            return -1;
    }

    memcpy(fb_dev->framebuffer, fb_dev->fbcache, fb_dev->fb_size);

    return 0;
}

struct display_ops fb_display_ops = {
    .open = display_fb_open,
    .close = display_fb_close,
    .show = display_fb_show,
};
