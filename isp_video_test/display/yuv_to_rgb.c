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
#include <stdint.h>
#include "yuv_to_rgb.h"

static short y_table[256];
static short rv_table[256];
static short gu_table[256];
static short gv_table[256];
static short bu_table[256];

void yuv_to_rgb_bt601_limit_range_init(void)
{
    float rv_param = 1.596;
    float gu_param = 0.392;
    float gv_param = 0.812;
    float bu_param = 2.016;
    int y  = 0;
    int uv = 0;
    for (int i = 0; i < 256; i++) {
        y  = ((i <= 16) ? 16 : ((i >= 235) ? 235 : i));
        uv = ((i <= 16) ? 16 : ((i >= 240) ? 240 : i));
        y_table[i]  = 1.164 * (y - 16);
        rv_table[i] = rv_param * (uv - 128);
        gu_table[i] = gu_param * (uv - 128);
        gv_table[i] = gv_param * (uv - 128);
        bu_table[i] = bu_param * (uv - 128);
    }
}

void yuv_to_rgb_bt601_full_range_init(void)
{
    float rv_param = 1.4075;
    float gu_param = 0.3455;
    float gv_param = 0.7169;
    float bu_param = 1.779;
    for (int i = 0; i < 256; i++) {
        y_table[i]  = i;
        rv_table[i] = rv_param * (i - 128);
        gu_table[i] = gu_param * (i - 128);
        gv_table[i] = gv_param * (i - 128);
        bu_table[i] = bu_param * (i - 128);
    }
}

int yuyv_to_rgb32(uint32_t *rgb, char *yuv, int width, int height)
{
    int i , j;
    uint8_t y, u, v;
    int r, g ,b;

    int pos;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pos = width * i + j;
            y = yuv[pos << 1];
            if (j & 0x01) {
                u = yuv[(pos << 1) - 1];
                v = yuv[(pos << 1) + 1];
            } else {
                u = yuv[(pos << 1) + 1];
                v = yuv[(pos << 1) + 3];
            }
            r = y_table[y] + rv_table[v];
            g = y_table[y] - gu_table[u] - gv_table[v];
            b = y_table[y] + bu_table[u];
            r = ((r < 0) ? 0 : (r > 255 ? 255 : r));
            g = ((g < 0) ? 0 : (g > 255 ? 255 : g));
            b = ((b < 0) ? 0 : (b > 255 ? 255 : b));

            rgb[pos] = 0xff000000 | (r << 16) | (g << 8) | b;
        }
    }

    return 0;
}

int nv16_to_rgb32(uint32_t *rgb, char *yuv, int width, int height)
{
    int i , j;
    uint8_t y, u, v;
    int r, g ,b;
    char *uv = yuv + width * height;

    int pos;
    int uv_pos;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pos = width * i + j;
            uv_pos = width * i + (j & (~ 0x01));
            y = yuv[pos];
            u = uv[uv_pos];
            v = uv[uv_pos + 1];
            r = y_table[y] + rv_table[v];
            g = y_table[y] - gu_table[u] - gv_table[v];
            b = y_table[y] + bu_table[u];
            r = ((r < 0) ? 0 : (r > 255 ? 255 : r));
            g = ((g < 0) ? 0 : (g > 255 ? 255 : g));
            b = ((b < 0) ? 0 : (b > 255 ? 255 : b));

            rgb[pos] = 0xff000000 | (r << 16) | (g << 8) | b;
        }
    }

    return 0;
}

int nv12_to_rgb32(uint32_t *rgb, char *yuv, int width, int height)
{
    int i , j;
    uint8_t y, u, v;
    int r, g ,b;
    char *uv = yuv + width * height;

    int pos;
    int uv_pos;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pos = width * i + j;
            uv_pos = (width * (i >> 1)) + (j & (~ 0x01));
            y = yuv[pos];
            u = uv[uv_pos];
            v = uv[uv_pos + 1];
            r = y_table[y] + rv_table[v];
            g = y_table[y] - gu_table[u] - gv_table[v];
            b = y_table[y] + bu_table[u];
            r = ((r < 0) ? 0 : ((r > 255) ? 255 : r));
            g = ((g < 0) ? 0 : ((g > 255) ? 255 : g));
            b = ((b < 0) ? 0 : ((b > 255) ? 255 : b));

            rgb[pos] = 0xff000000 | (r << 16) | (g << 8) | b;
        }
    }

    return 0;
}

int p010_to_rgb32(uint32_t *rgb, char *yuv, int width, int height, bool lsb)
{
    int i , j;
    uint8_t y, u, v;
    int r, g ,b;
    uint16_t *y_data = (uint16_t *)yuv;
    uint16_t *uv_data = y_data + width * height;

    int pos;
    int uv_pos;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pos = width * i + j;
            uv_pos = (width * (i >> 1)) + (j & (~ 0x01));

            if (lsb) {
                // lsb: low 10 bit is valid, upper 6bit 0 + 10bit valid
                y = (uint8_t)((y_data[pos] >> 2) & 0xff);
                u = (uint8_t)((uv_data[uv_pos] >> 2) & 0xff);
                v = (uint8_t)((uv_data[uv_pos + 1] >> 2) & 0xff);
            } else {
                // msb: high 10 bit is valid, 10bit valid + lower 6 bit 0
                y = (uint8_t)((y_data[pos] >> 8) & 0xff);
                u = (uint8_t)((uv_data[uv_pos] >> 8) & 0xff);
                v = (uint8_t)((uv_data[uv_pos + 1] >> 8) & 0xff);
            }

            r = y_table[y] + rv_table[v];
            g = y_table[y] - gu_table[u] - gv_table[v];
            b = y_table[y] + bu_table[u];
            r = ((r < 0) ? 0 : ((r > 255) ? 255 : r));
            g = ((g < 0) ? 0 : ((g > 255) ? 255 : g));
            b = ((b < 0) ? 0 : ((b > 255) ? 255 : b));

            rgb[pos] = 0xff000000 | (r << 16) | (g << 8) | b;
        }
    }

    return 0;
}

int rgb24_to_rgb32(uint32_t *rgb, char *data, int width, int height)
{
    int i , j;
    char r, g ,b;
    int pos;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pos = width * i + j;
            r = data[pos * 3];
            g = data[pos * 3 + 1];
            b = data[pos * 3 + 2];
            rgb[pos] = 0xff000000 | (r << 16) | (g << 8) | b;
        }
    }

    return 0;
}

int bgr24_to_rgb32(uint32_t *rgb, char *data, int width, int height)
{
    int i , j;
    char r, g ,b;
    int pos;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pos = width * i + j;
            b = data[pos * 3];
            g = data[pos * 3 + 1];
            r = data[pos * 3 + 2];
            rgb[pos] = 0xff000000 | (r << 16) | (g << 8) | b;
        }
    }

    return 0;
}

int raw8_to_rgb32(uint32_t *rgb, char *data, int width, int height)
{
    int i , j;
    char r, g ,b;
    int pos;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pos = width * i + j;
            r = data[pos];
            g = r;
            b = r;
            rgb[pos] = 0xff000000 | (r << 16) | (g << 8) | b;
        }
    }

    return 0;
}

int raw16_to_rgb32(uint32_t *rgb, char *data, int width, int height, uint32_t bit)
{
    int i , j;
    char r, g ,b;
    int pos;
    uint16_t *raw_data = (uint16_t *)data;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pos = width * i + j;
            r = ((((raw_data[pos] >> 8) & 0xff) | ((raw_data[pos] & 0xff) << 8)) >> (bit - 8)) & 0xFF;
            g = r;
            b = r;
            rgb[pos] = 0xff000000 | (r << 16) | (g << 8) | b;
        }
    }

    return 0;
}
