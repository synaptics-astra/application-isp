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

#ifndef __DRM_DISPLAY_H__
#define __DRM_DISPLAY_H__

#include <stdint.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm/drm_fourcc.h>

typedef struct drm_buffer_s {
    void *fb_base;

    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t size;

    uint32_t handle;
    uint32_t fb;
    uint32_t  pixel_format;
    int dma_buf_fd;  // DMA-BUF file descriptor for V4L2 import

    unsigned long paddr;
} drm_buffer_t;

struct plane_props {
    uint32_t fb_id;
    uint32_t crtc_id;
    uint32_t src_x;
    uint32_t src_y;
    uint32_t src_w;
    uint32_t src_h;
    uint32_t crtc_x;
    uint32_t crtc_y;
    uint32_t crtc_w;
    uint32_t crtc_h;
};


typedef struct drm_dev_s {
    int fd;
    uint32_t conn;
    drmModeModeInfo mode;
    uint32_t count_modes;
    drmModeModeInfo *mode_list;
    int crtc_id;

    drm_buffer_t *bufs;
    uint32_t plane_id;
    struct plane_props props;
    int crtc_set;
    int crtc_index;
    int height;
    int width;
    drmModeAtomicReq *atomic_req;
    uint32_t atomic_flags;
    int num_display_buf;
    pthread_t vblank_thread;
    int pipefd[2];
    int is_thread_running;
} drm_dev_t;

extern struct display_ops drm_display_ops;

#endif
