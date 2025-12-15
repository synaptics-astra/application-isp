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

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <stdint.h>
#include <linux/videodev2.h>

#ifndef V4L2_PIX_FMT_P010
#define V4L2_PIX_FMT_P010 v4l2_fourcc('P', '0', '1', '0') /* 24 Y/CbCr 4:2:0 10-bit per component */
#endif

/* ISP IOMMU MTR needs 256 bytes alignment */
#define WIDTH_ALIGN_SIZE 256
#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

extern char g_output_path[256];

enum display_type {
    VIDEO_DISPLAY_TYPE_DRM   = 0,
    VIDEO_DISPLAY_TYPE_FB    = 1,
    VIDEO_DISPLAY_TYPE_FILE  = 2,
    VIDEO_DISPLAY_TYPE_WL    = 3,

    VIDEO_DISPLAY_TYPE_MAX
};

struct display
{
    void *handle;
    struct display_ops *ops;
};

struct display_ops
{
    void* (*open)(int idx, int fourcc, int num_display_buf);
    void (*close)(void *handle);
    int (*show)(void *handle, void *data, int width, int height, int fourcc);
    int (*show_mp)(void *handle, void *data1, void *data2, int width, int height, int fourcc,
            int iommu);
};

struct display *create_display(uint8_t type);

#endif
