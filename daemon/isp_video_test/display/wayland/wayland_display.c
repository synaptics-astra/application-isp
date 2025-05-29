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

#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <time.h>
#include <stdbool.h>

#include "wayland_display.h"
#include "display.h"

#define TIME_OUT 200
#define LOGTAG "WlDisplay"

static shm_format_support shm_format_table[] = {
    {SHM_FORMAT_YUYV, false},
    {SHM_FORMAT_NV12, false},
    {SHM_FORMAT_NV16, false},
    {SHM_FORMAT_P010, false},
};

static void rand_shm_name(char *buf)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i) {
        buf[i] = 'A'+(r&15)+(r&16)*2;
        r >>= 5;
    }
    return;
}

static int create_shm_file(void)
{
    int ret = -1;
    int retries = 100;

    do {
        char name[] = "/wl_shm-XXXXXX";
        rand_shm_name(name + sizeof(name) - 7);
        --retries;
        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            shm_unlink(name);
            return fd;
        }
    } while (retries > 0 && errno == EEXIST);

    return ret;
}

static int allocate_shm_file(size_t size)
{
    int ret = -1;
    int fd = create_shm_file();

    if (fd < 0)
    return ret;

    do {
        ret = ftruncate(fd, size);
    } while (ret < 0 && errno == EINTR);

    if (ret < 0) {
        close(fd);
        return ret;
    }

    return fd;
}

static void buffer_release(void *data, struct wl_buffer *buffer)
{
    struct buffer *pbuffer = (struct buffer *)data;
    pbuffer->busy = 0;
}

static const struct wl_buffer_listener buffer_listener = {
    buffer_release
};

static int create_shm_buf(struct display_wl *pdisplay, struct buffer *pbuffer) {
    int ret = 0;
    int fd  = -1;
    int pool_size = 0;
    void *data    = NULL;
    struct wl_shm_pool *pool = NULL;

    pool_size = pdisplay->format.image_size;
    fd = allocate_shm_file(pool_size);
    if (fd < 0) {
        ret = -1;
        return ret;
    }

    data = mmap(NULL, pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        printf("mmap failed: %s", strerror(errno));
        close(fd);
        ret = -1;
        return ret;
    }

    pool = wl_shm_create_pool(pdisplay->shm, fd, pool_size);
    pbuffer->buf = wl_shm_pool_create_buffer(pool, 0,
        pdisplay->format.width, pdisplay->format.height,
        pdisplay->format.stride, pdisplay->format.shm_format);
    wl_buffer_add_listener(pbuffer->buf, &buffer_listener, pbuffer);

    wl_shm_pool_destroy(pool);
    close(fd);

    pbuffer->shm_data = data;
    pbuffer->shm_pool_size = pool_size;
    memset(pbuffer->shm_data, 0x00, pool_size);

    return ret;
}

static void xdg_wm_base_ping( void *data, struct xdg_wm_base *shell, uint32_t serial)
{
    xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
    xdg_wm_base_ping,
};

static void shm_format(void *data, struct wl_shm *wl_shm, uint32_t format) {
    switch (format) {
    case WL_SHM_FORMAT_YUYV:
        shm_format_table[SHM_FORMAT_YUYV].support = true;
        break;
    case WL_SHM_FORMAT_NV12:
        shm_format_table[SHM_FORMAT_NV12].support = true;
        break;
    case WL_SHM_FORMAT_NV16:
        shm_format_table[SHM_FORMAT_NV16].support = true;
        break;
    case WL_SHM_FORMAT_P010:
        shm_format_table[SHM_FORMAT_P010].support = true;
        break;
    default:
        break;
    }
}

static const struct wl_shm_listener shm_listener = {
    shm_format,
};

static void registry_handle_global(void *data, struct wl_registry *registry,
                       uint32_t id, const char *interface, uint32_t version)
{
    struct display_wl *display_handle = (struct display_wl *)data;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        display_handle->compositor = (struct wl_compositor*)wl_registry_bind(registry,
                                 id, &wl_compositor_interface, 1);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        display_handle->wm_base = (struct xdg_wm_base*)wl_registry_bind(registry,
                            id, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(display_handle->wm_base, &wm_base_listener, display_handle);
    } else if (strcmp(interface, wp_viewporter_interface.name) == 0) {
        display_handle->viewporter = (struct wp_viewporter *)wl_registry_bind(registry, id,
                        &wp_viewporter_interface, 1);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        display_handle->shm = (struct wl_shm *)wl_registry_bind(registry, id, &wl_shm_interface, 1);
        wl_shm_add_listener(display_handle->shm, &shm_listener, NULL);
    }
};

static void registry_handle_global_remove(void *data, struct wl_registry *registry,
                              uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove
};

static struct display_wl *create_display_wl(const char *name)
{
    struct display_wl *pdisplay = NULL;

    pdisplay = (struct display_wl *) malloc(sizeof(struct display_wl));
    if (pdisplay == NULL) {
        printf("malloc display failed");
        return pdisplay;
    }
    memset(pdisplay, 0, sizeof(struct display_wl));

    pdisplay->display = wl_display_connect(name);
    if (pdisplay->display == NULL) {
        printf("Can't connect to display\n");
        return pdisplay;
    }

    pdisplay->registry = wl_display_get_registry(pdisplay->display);
    wl_registry_add_listener(pdisplay->registry,
                             &registry_listener, pdisplay);

    wl_display_roundtrip(pdisplay->display);

    if (pdisplay->compositor == NULL) {
        printf("Can't find compositor\n");
    }

    if (pdisplay->viewporter == NULL) {
        printf("Can't find viewporter\n");
    }

    if (pdisplay->wm_base == NULL) {
        printf("Can't find xdg_wm_base\n");
    }

    if (pdisplay->shm == NULL) {
        printf("Can't find wl_shm\n");
    }

    // do two roundtrip: ensure process shm, xdg event
    wl_display_roundtrip(pdisplay->display);

    return pdisplay;
}

static void xdg_surface_handle_config(void *data, struct xdg_surface *surface, uint32_t serial)
{
    xdg_surface_ack_configure(surface, serial);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    xdg_surface_handle_config,
};

static struct window *create_window(struct display_wl *pdisplay)
{
    struct window *pwindow;
    pwindow = (struct window *) malloc(sizeof(struct window));
    memset(pwindow, 0, sizeof(struct window));

    assert(pdisplay);
    pwindow->display = pdisplay;

    pwindow->surface = wl_compositor_create_surface(pdisplay->compositor);
    assert(pwindow->surface);

    if (pdisplay->wm_base) {
        pwindow->viewport = wp_viewporter_get_viewport(pdisplay->viewporter,
                                  pwindow->surface);
        assert(pwindow->viewport);

        pwindow->xdg_surface = xdg_wm_base_get_xdg_surface(pdisplay->wm_base,
                            pwindow->surface);
        assert(pwindow->xdg_surface);
        xdg_surface_add_listener(pwindow->xdg_surface,
                     &xdg_surface_listener, pwindow);

        pwindow->xdg_toplevel = xdg_surface_get_toplevel(pwindow->xdg_surface);
        assert(pwindow->xdg_toplevel);
        wl_surface_commit(pwindow->surface);
        wl_display_roundtrip(pdisplay->display);
    } else {
        printf("Get xdg_shm_base failed");
    }

    return pwindow;
}

static int NV16_to_YUYV(unsigned char *src, unsigned char *dst,
                        int width, int height) {
    int ret  = 0;
    int size = width * height * 2;
    const int offset_y1  = 0;
    const int offset_u   = 1;
    const int offset_y2  = 2;
    const int offset_v   = 3;
    unsigned char *src_y = src;
    unsigned char *src_u = src + width * height;
    unsigned char *src_v = src_u + 1;

    if (width % 4 != 0 || height % 4 != 0) {
        printf("%s: width %d and height %d are not integer multiple of 4",
            __func__, width, height);
        ret = -1;
        return ret;
    }

    for (int i = 0; i < size; i += 4) {
        dst[i + offset_y1] = *src_y;
        src_y++;
        dst[i + offset_u]  = *src_u;
        src_u = src_v + 1;
        dst[i + offset_y2] = *src_y;
        src_y++;
        dst[i + offset_v]  = *src_v;
        src_v = src_u + 1;
    }

    return ret;
}

static int P010_to_NV12(unsigned char *src, unsigned char *dst,
                        int width, int height, bool lsb) {
    int ret  = 0;
    int size = width * height * 3 / 2;
    uint16_t *src_u16 = (uint16_t *)src;

    if (width % 2 != 0 || height % 2 != 0) {
        printf("%s: width %d and height %d are not integer multiple of 2",
            __func__, width, height);
        ret = -1;
        return ret;
    }

    for (int i = 0; i < size; i++) {
        if (lsb) {
            // lsb: low 10 bit is valid, upper 6bit 0 + 10bit valid
            dst[i] = (unsigned char)((src_u16[i] >> 2) & 0xff);
        } else {
            // msb: high 10 bit is valid, 10bit valid + lower 6 bit 0
            dst[i] = (unsigned char)((src_u16[i] >> 8) & 0xff);
        }
    }

    return ret;
}

static int queue_initial_buffers(struct window *pwindow)
{
    int ret = 0;
    struct buffer *pbuffer;

    pwindow->buffers = (struct buffer *)calloc(pwindow->num_bufs, sizeof(struct buffer));
    for (int index = 0; index < pwindow->num_bufs; index++) {
        pbuffer = &pwindow->buffers[index];
        pbuffer->display = pwindow->display;
        pbuffer->index = index;
        pbuffer->width = pwindow->display->format.width;
        pbuffer->height = pwindow->display->format.height;
        ret = create_shm_buf(pbuffer->display, pbuffer);
        if (ret) {
            printf("%s: create shm buf %d failed", __func__, index);
            return ret;
        }
    }

    return ret;
}

static int display_format_set(struct window *pWindow, int aligned_width, int height,
                                        int size, int fourcc, int width)
{
    int ret = 0;
    int stride = 0;
    struct display_wl *pdisplay = pWindow->display;

    switch (fourcc) {
    case V4L2_PIX_FMT_YUYV:
        pdisplay->format.shm_format = WL_SHM_FORMAT_YUYV;
        stride = aligned_width * 2;
        break;
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV12M:
        pdisplay->format.shm_format = WL_SHM_FORMAT_NV12;
        stride = aligned_width;
        break;
    case V4L2_PIX_FMT_NV16:
        if (shm_format_table[SHM_FORMAT_NV16].support) {
            pdisplay->format.shm_format = WL_SHM_FORMAT_NV16;
        } else {
            // transform NV16 to YUYV
            pdisplay->format.shm_format = WL_SHM_FORMAT_YUYV;
            pdisplay->format.trfm_to_other = true;
        }
        stride = aligned_width * 2;
        break;
    case V4L2_PIX_FMT_P010:
        if (shm_format_table[SHM_FORMAT_P010].support) {
            pdisplay->format.shm_format = WL_SHM_FORMAT_P010;
            stride = aligned_width * 2;
        } else {
            // transform P010 to NV12
            pdisplay->format.shm_format = WL_SHM_FORMAT_NV12;
            pdisplay->format.trfm_to_other = true;
            stride = aligned_width;
            size /= 2;
        }
        break;
    default:
        printf("%s: Unsupported format: %d\n", __func__, fourcc);
        return -1;
    }

    pWindow->num_bufs = NUM_DISPLAY_BUFFERS;
    pdisplay->format.width  = width;
    pdisplay->format.height = height;
    pdisplay->format.stride = stride;
    pdisplay->format.image_size = size;

    ret = queue_initial_buffers(pWindow);
    if (ret) {
        printf("queue init buffers failed");
        return ret;
    }

    return ret;
}

void *WlDisplay_Create(int idx, int fourcc, int num_display_buf)
{
    char socket_name[20], socket_addr[64];
    char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    struct display_wl *pdisplay;
    struct window *pWindow = NULL;
    sprintf(socket_name, "wayland-%d", idx);
    sprintf(socket_addr, "%s/%s", runtime_dir, socket_name);
    if(access(socket_addr, F_OK) != -1) {
        pdisplay = create_display_wl(socket_name);
        pWindow = create_window(pdisplay);
        printf("%s:create window %s succcess!\n",__func__, socket_name);
    } else {
        printf("%s:create window %s failed!\n",__func__, socket_name);
    }

    return pWindow;
}

static void destroy_display(struct display_wl *pdisplay)
{
    if (!pdisplay) {
        return;
    }

    if (pdisplay->wm_base) {
        xdg_wm_base_destroy(pdisplay->wm_base);
    }

    if (pdisplay->shm) {
        wl_shm_destroy(pdisplay->shm);
    }

    if (pdisplay->viewporter) {
        wp_viewporter_destroy(pdisplay->viewporter);
    }

    if (pdisplay->compositor)
        wl_compositor_destroy(pdisplay->compositor);

    wl_registry_destroy(pdisplay->registry);
    wl_display_flush(pdisplay->display);
    wl_display_disconnect(pdisplay->display);
}


static void destroy_window(struct window *pwindow)
{
    if (pwindow->viewport) {
        wp_viewport_destroy(pwindow->viewport);
    }

    if (pwindow->xdg_toplevel) {
        xdg_toplevel_destroy(pwindow->xdg_toplevel);
    }

    if (pwindow->xdg_surface) {
        xdg_surface_destroy(pwindow->xdg_surface);
    }

    wl_surface_destroy(pwindow->surface);

    for (int i = 0; i < pwindow->num_bufs; i++) {
        if (!&pwindow->buffers[i])
            continue;
        if (pwindow->buffers[i].shm_data) {
            munmap(pwindow->buffers[i].shm_data,
                pwindow->buffers[i].shm_pool_size);
        }

        if (!pwindow->buffers[i].buf)
            continue;
        wl_buffer_destroy(pwindow->buffers[i].buf);
    }
}

void WlDisplay_Destory(void *handle)
{
    struct window *pWindow = (struct window *)handle;
    if (pWindow) {
        struct display_wl *pdisplay = pWindow->display;
        destroy_window(pWindow);
        destroy_display(pdisplay);
        free(pdisplay);
        free(pWindow->buffers);
        free(pWindow);
    }
}

static int frameid = 0;
int WlDisplay_showBuffer(void *handle, void *data, int width, int height, int fourcc) {
    struct window *pWindow = (struct window *)handle;
    int ret = 0;
    int size = 0;

    switch (fourcc) {
        case V4L2_PIX_FMT_YUYV:
            size = width * height * 2;
            break;
        case V4L2_PIX_FMT_NV12:
            size = width * height * 3 / 2;
            break;
        case V4L2_PIX_FMT_NV16:
            size = width * height * 2;
            break;
        case V4L2_PIX_FMT_P010:
            size = width * height * 3;
            break;
        default:
            printf("%s: Unsupported format: %d\n", __func__, fourcc);
            return -1;
    }

    if((width != pWindow->video_width) || (height != pWindow->video_height)) {
        ret = display_format_set(pWindow, width, height, size, fourcc, width);
        if (ret) {
            printf("%s: set display format failed", __func__);
            return -1;
        }
    }

    struct buffer *pbuffer = NULL;
    /*avoid use busy buffer*/
    while (pWindow->buffers[frameid].busy) {}

    pbuffer = &pWindow->buffers[frameid];

    if (fourcc == V4L2_PIX_FMT_NV16 && pWindow->display->format.trfm_to_other) {
        ret = NV16_to_YUYV(data, (unsigned char *)pbuffer->shm_data,
                width, height);
        if (ret) {
            printf("%s: transform nv16 to yuyv failed", __func__);
            return -1;
        }
    } else if (fourcc == V4L2_PIX_FMT_P010 &&
            pWindow->display->format.trfm_to_other) {
        ret = P010_to_NV12(data, (unsigned char *)pbuffer->shm_data,
                width, height, true);
        if (ret) {
            printf("%s: transform p010 to nv12 failed", __func__);
            return -1;
        }
    } else {
        memcpy(pbuffer->shm_data, data, size);
    }

    pWindow->video_width = width;
    pWindow->video_height = height;

    wl_surface_attach(pWindow->surface, pbuffer->buf, 0, 0);
    wl_surface_damage(pWindow->surface, 0, 0,
            pWindow->display->format.width,
            pWindow->display->format.height);

    if (width < pWindow->video_width || height < pWindow->video_height) {
        wp_viewport_set_destination(pWindow->viewport, width, height);
    } else {
        wp_viewport_set_destination(pWindow->viewport, pWindow->video_width, pWindow->video_height);
    }

    wl_surface_commit(pWindow->surface);
    wl_display_flush(pWindow->display->display);
    wl_display_dispatch(pWindow->display->display);
    pbuffer->busy = 1;

    frameid++;
    frameid %= NUM_DISPLAY_BUFFERS;

    return 0;
}

int WlDisplay_showBuffer_mp(void *handle, void *data1, void *data2,
        int width, int height, int fourcc, int iommu)
{
    struct window *pWindow = (struct window *)handle;
    int ret = 0;
    int size = 0;
    int aligned_width = width;
    int y_size, uv_size = 0;
    int curr_time = 0;


    if(iommu)
        aligned_width = ALIGN(width, WIDTH_ALIGN_SIZE);

    switch (fourcc) {
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV12M:
            size = aligned_width * height * 3 / 2;
            break;
        default:
            printf("%s: Unsupported format: %d\n", __func__, fourcc);
            return -1;
    }

    if((width != pWindow->video_width) || (height != pWindow->video_height)) {
        ret = display_format_set(pWindow, aligned_width, height, size, fourcc, width);
        if (ret) {
            printf("%s: set display format failed", __func__);
            return -1;
        }
    }

    struct buffer *pbuffer = NULL;
    /*avoid use busy buffer*/
    while (pWindow->buffers[frameid].busy) {
        if (++curr_time > TIME_OUT) {
            printf("%s: Buffer busy for too long, timing out\n", __func__);
            return -1;
        }
        usleep(1000);
    }

    pbuffer = &pWindow->buffers[frameid];

    if (fourcc == V4L2_PIX_FMT_NV16 && pWindow->display->format.trfm_to_other) {
        ret = NV16_to_YUYV(data1, (unsigned char *)pbuffer->shm_data,
                width, height);
        if (ret) {
            printf("%s: transform nv16 to yuyv failed", __func__);
            return -1;
        }
    } else if (fourcc == V4L2_PIX_FMT_P010 &&
            pWindow->display->format.trfm_to_other) {
        ret = P010_to_NV12(data1, (unsigned char *)pbuffer->shm_data,
                width, height, true);
        if (ret) {
            printf("%s: transform p010 to nv12 failed", __func__);
            return -1;
        }
    } else {
        if (!data2)
            memcpy(pbuffer->shm_data, data1, size);
        else {
            y_size = aligned_width * height;
            uv_size = aligned_width * height/2;
            memcpy(pbuffer->shm_data, data1, y_size);
            memcpy((char *)pbuffer->shm_data + y_size, data2, uv_size);
        }
    }

    pWindow->video_width = width;
    pWindow->video_height = height;

    wl_surface_attach(pWindow->surface, pbuffer->buf, 0, 0);
    wl_surface_damage(pWindow->surface, 0, 0,
            pWindow->display->format.width,
            pWindow->display->format.height);

    if (aligned_width < pWindow->video_width || height < pWindow->video_height) {
        wp_viewport_set_destination(pWindow->viewport, aligned_width, height);
    } else {
        wp_viewport_set_destination(pWindow->viewport, pWindow->video_width, pWindow->video_height);
    }

    wl_surface_commit(pWindow->surface);
    wl_display_flush(pWindow->display->display);
    wl_display_dispatch(pWindow->display->display);
    pbuffer->busy = 1;

    frameid++;
    frameid %= 4;

    return 0;
}

struct display_ops wl_display_ops = {
    .open = WlDisplay_Create,
    .close = WlDisplay_Destory,
    .show = WlDisplay_showBuffer,
    .show_mp = WlDisplay_showBuffer_mp,
};
