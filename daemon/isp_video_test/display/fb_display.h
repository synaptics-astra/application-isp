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

#ifndef __FB_DISPLAY_H__
#define __FB_DISPLAY_H__


typedef struct fb_dev_s {
    int fd;
    int xres;
    int yres;
    int fb_size;
    unsigned char* framebuffer;
    unsigned char* fbcache;
} fb_dev_t;

extern struct display_ops fb_display_ops;

#endif
