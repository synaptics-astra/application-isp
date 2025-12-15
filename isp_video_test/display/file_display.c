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
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <assert.h>
#include <memory.h>
#include <sys/time.h>
#include <time.h>
#include <linux/videodev2.h>

#include "file_display.h"
#include "display.h"

FILE* pFile = NULL;

void dump_file(void* data, int width_align, int width, int height)
{
    int offset = 0;
    for(int i = 0; i < height; i++)
    {
        fwrite(data + offset,
                width,
                1,
                pFile);
        offset += width_align;
    }
}


int display_save_file_mp(void *handle, void* data1, void* data2,
        int width, int height, int fourcc, int iommu)
{
    char file_name[128];
    char full_path[512];
    int width_align = width;
    uint32_t size = 0;
    uint8_t isCopyaligned = 1;


    switch(fourcc) {
        case V4L2_PIX_FMT_NV12M:
            size = width_align * height * 3 / 2;
            sprintf(file_name, "Video_NV12M_%d_%d.yuv", width, height);
            if(iommu)
                width_align = ALIGN(width, WIDTH_ALIGN_SIZE);
            isCopyaligned = 0;
            break;
        case V4L2_PIX_FMT_NV12:
            size = width_align * height * 3 / 2;
            sprintf(file_name, "Video_NV12_%d_%d.yuv", width, height);
            break;
        case V4L2_PIX_FMT_YUV24:
            size = width * height * 3;
            sprintf(file_name, "Video_YUV24_%d_%d.yuv", width, height);
            width *= 3;
            if(iommu)
                width_align = ALIGN(width, WIDTH_ALIGN_SIZE);
            isCopyaligned = 0;
            break;
        case V4L2_PIX_FMT_BGR24:
            size = width_align * height * 3 /*bytes_per_pixel*/;
            sprintf(file_name, "Video_BGR24_%d_%d.bgr", width, height);
            break;
        case V4L2_PIX_FMT_RGB24:
            size = width_align * height * 3 /*bytes_per_pixel*/;
            sprintf(file_name, "Video_RGB24_%d_%d.rgb", width, height);
            break;
        case V4L2_PIX_FMT_SBGGR10:
        case V4L2_PIX_FMT_SGBRG10:
        case V4L2_PIX_FMT_SGRBG10:
        case V4L2_PIX_FMT_SRGGB10:
            size = width * height * 2;
            sprintf(file_name, "Video_RAW10_%d_%d.raw", width, height);
            break;
        case V4L2_PIX_FMT_SBGGR12:
        case V4L2_PIX_FMT_SGBRG12:
        case V4L2_PIX_FMT_SGRBG12:
        case V4L2_PIX_FMT_SRGGB12:
            size = width_align * height * 2;
            sprintf(file_name, "Video_RAW12_%d_%d.raw", width, height);
            break;
        default:
            printf("%s: Unsupported format: %d\n", __func__, fourcc);
            return -1;
    }

    if (pFile == NULL) {
        // Construct full path based on global output path
        if (g_output_path[0] != '\0') {
            snprintf(full_path, sizeof(full_path), "%s/%s", g_output_path, file_name);
            pFile = fopen(full_path, "w");
        } else {
            strncpy(full_path, file_name, sizeof(full_path));
            full_path[sizeof(full_path) - 1] = '\0';
            pFile = fopen(full_path, "w");
        }
    }


    if (pFile) {
        if (!isCopyaligned) {
            /* NV12M format - write Y and UV separately */
            /* Y */
            dump_file(data1, width_align, width, height);

            /* UV */
            if (data2)
                dump_file(data2, width_align, width, height / 2);
        } else {
            /* RAW, NV12 and RGB formats - write as a single block */
            fwrite(data1, size, 1, pFile);
        }
    } else {
        printf("%s: Failed to open file: %s\n", __func__, full_path);
        return -1;
    }

    return 0;
}

int display_save_file(void *handle, void* data, int width, int height, int fourcc)
{
    char *file = (char*)handle;
    uint32_t size = 0;
    char file_name[128];

    struct timeval time;

    gettimeofday(&time, NULL);

    switch(fourcc) {
        case V4L2_PIX_FMT_NV16:
            size = width * height * 2;
            sprintf(file_name, "Image_NV16_%d_%d_%ld_%06ld.yuv", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        case V4L2_PIX_FMT_YUYV:
            size = width * height * 2;
            sprintf(file_name, "Image_YUYV_%d_%d_%ld_%06ld.yuv", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        case V4L2_PIX_FMT_NV12:
            size = width * height * 3 / 2;
            sprintf(file_name, "Image_NV12_%d_%d_%ld_%06ld.yuv", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        case V4L2_PIX_FMT_P010:
            size = width * height * 2;
            sprintf(file_name, "Image_P010_%d_%d_%ld_%06ld.yuv", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        case V4L2_PIX_FMT_SBGGR8:
            size = width * height;
            sprintf(file_name, "Image_BGGR8_%d_%d_%ld_%06ld.raw", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        case V4L2_PIX_FMT_SGBRG8:
            size = width * height;
            sprintf(file_name, "Image_GBRG8_%d_%d_%ld_%06ld.raw", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        case V4L2_PIX_FMT_SGRBG8:
            size = width * height;
            sprintf(file_name, "Image_GRBG8_%d_%d_%ld_%06ld.raw", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        case V4L2_PIX_FMT_SRGGB8:
            size = width * height;
            sprintf(file_name, "Image_RGGB8_%d_%d_%ld_%06ld.raw", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        case V4L2_PIX_FMT_SBGGR10:
            size = width * height * 2;
            sprintf(file_name, "Image_BGGR10_%d_%d_%ld_%06ld.raw", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        case V4L2_PIX_FMT_SGBRG10:
            size = width * height * 2;
            sprintf(file_name, "Image_GBRG10_%d_%d_%ld_%06ld.raw", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        case V4L2_PIX_FMT_SGRBG10:
            size = width * height * 2;
            sprintf(file_name, "Image_GRBG10_%d_%d_%ld_%06ld.raw", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        case V4L2_PIX_FMT_SRGGB10:
            size = width * height * 2;
            sprintf(file_name, "Image_RGGB10_%d_%d_%ld_%06ld.raw", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        case V4L2_PIX_FMT_SBGGR12:
            size = width * height * 2;
            sprintf(file_name, "Image_BGGR12_%d_%d_%ld_%06ld.raw", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        case V4L2_PIX_FMT_SGBRG12:
            size = width * height * 2;
            sprintf(file_name, "Image_GBRG12_%d_%d_%ld_%06ld.raw", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        case V4L2_PIX_FMT_SGRBG12:
            size = width * height * 2;
            sprintf(file_name, "Image_GRBG12_%d_%d_%ld_%06ld.raw", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        case V4L2_PIX_FMT_SRGGB12:
            size = width * height * 2;
            sprintf(file_name, "Image_RGGB12_%d_%d_%ld_%06ld.raw", width, height, time.tv_sec, time.tv_usec);
            if (!file)
                file = file_name;
            break;
        default:
            printf("%s: Unsupported format: %d\n", __func__, fourcc);
            return -1;
    }

    FILE* pFile = fopen(file, "wb");
    if (pFile) {
        fwrite(data, 1, size, pFile);
        fclose(pFile);
    }

    return 0;
}

struct display_ops file_display_ops = {
    .show = display_save_file,
    .show_mp = display_save_file_mp,
};
