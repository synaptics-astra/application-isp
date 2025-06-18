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

#ifndef __YUV_TO_RGB_H__
#define __YUV_TO_RGB_H__
#include <stdint.h>
#include <stdbool.h>

void yuv_to_rgb_bt601_limit_range_init(void);
void yuv_to_rgb_bt601_full_range_init(void);
int yuyv_to_rgb32(uint32_t *rgb, char *yuv, int width, int height);
int nv16_to_rgb32(uint32_t *rgb, char *yuv, int width, int height);
int nv12_to_rgb32(uint32_t *rgb, char *yuv, int width, int height);
int bgr24_to_rgb32(uint32_t *rgb, char *data, int width, int height);
int rgb24_to_rgb32(uint32_t *rgb, char *data, int width, int height);
int raw8_to_rgb32(uint32_t *rgb, char *data, int width, int height);
int raw16_to_rgb32(uint32_t *rgb, char *data, int width, int height, uint32_t bit);
int p010_to_rgb32(uint32_t *rgb, char *yuv, int width, int height, bool lsb);

#endif
