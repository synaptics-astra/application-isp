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

#ifndef _V4L_WLDISPLAY_H_
#define _V4L_WLDISPLAY_H_

#include <stdbool.h>
#include <wayland-client.h>
#include <drm/drm_fourcc.h>
#include <sys/types.h>

#include "xdg-shell-client-protocol.h"
#include "viewporter-client-protocol.h"

enum shm_format_id {
    SHM_FORMAT_YUYV = 0,
    SHM_FORMAT_NV12,
    SHM_FORMAT_NV16,
    SHM_FORMAT_P010,
};

typedef struct shm_format_support {
    uint32_t format;
    unsigned int support;
} shm_format_support;

struct buffer_format {
    unsigned int trfm_to_other;
    uint32_t width;
    uint32_t height;
    unsigned stride;
    uint32_t image_size;
    uint32_t pixel_format;
    uint32_t shm_format;
    uint32_t type;
};

struct display_wl {
    int v4l2_fd;
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wp_viewporter *viewporter;
    struct xdg_wm_base *wm_base;
    struct wl_shm *shm;
    struct buffer_format format;
};

struct buffer {
    uint32_t width;
    uint32_t height;
    int busy;
    int index;
    int shm_pool_size;
    void *shm_data;
    struct wl_buffer *buf;
    struct display_wl *display;
};

struct window {
    int num_bufs;
    int video_width;
    int video_height;
    struct display_wl *display;
    struct wl_surface *surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct wp_viewport *viewport;
    struct buffer *buffers;
};

void *WlDisplay_Create(int idx, int fourcc, int num_display_buf);
void WlDisplay_Destory(void *handle);
int WlDisplay_showBuffer(void *handle, void *data, int width, int height, int fourcc);

extern struct display_ops wl_display_ops;

#endif // _V4L_WLDISPLAY_H_
