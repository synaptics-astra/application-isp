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
#include <drm/drm_fourcc.h>
#include <pthread.h>

#include "drm_display.h"
#include "display.h"
#include "yuv_to_rgb.h"
#include "memcpy_optimized.h"

#define PIPE_READ_INDEX   0
#define PIPE_WRITE_INDEX  1

unsigned int buf_index = 0;
unsigned int first = 0;
long count_frames = 0;
long default_frame_count = 1000;


static int format_conversion(int fourcc)
{
    if (fourcc == V4L2_PIX_FMT_RGB24)
        return fourcc = DRM_FORMAT_RGB888;
    else if (fourcc == V4L2_PIX_FMT_BGR24)
        return fourcc = DRM_FORMAT_BGR888;
    else if (fourcc == V4L2_PIX_FMT_NV12M)
        return fourcc = DRM_FORMAT_NV12;

    return -1;
}

static int drm_open(drm_dev_t *handle, char *node)
{
    int fd;
    fd = open(node, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        printf("open drm %s failed\n", node);
        return -1;
    }

    uint64_t has_dumb;
    if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 || !has_dumb) {
        printf("drm device '%s' does not support dumb buffers\n", node);
        close(fd);
        return -2;
    }

    handle->fd = fd;

    return 0;
}

static int modeset_find_crtc(drm_dev_t *handle, drmModeRes *res, drmModeConnector *conn)
{
    drmModeEncoder *enc;
    int i, j;

    for (i = 0; i < conn->count_encoders; i++) {
        enc = drmModeGetEncoder(handle->fd, conn->encoders[i]);
        if (!enc)
            continue;

        for (j = 0; j < res->count_crtcs; j++) {
            if (!(enc->possible_crtcs & (1 << j)))
                continue;

            // Use the first available CRTC that matches the encoder
            handle->crtc_id = res->crtcs[j];
            drmModeFreeEncoder(enc);
            return 0;
        }
        drmModeFreeEncoder(enc);
    }
    return -ENOENT;
}

int create_dumb_buffer(int fd, uint32_t width, uint32_t height, uint32_t bpp, uint32_t pixel_format,
                       uint32_t *handle, uint32_t *pitch, uint64_t *size)
{
    struct drm_mode_create_dumb create_dumb = {0};

    create_dumb.width = width;
    create_dumb.bpp   = bpp;

    if (pixel_format == DRM_FORMAT_NV12) {
        /* Since NV12 has 2 plane while calcaulting height bothe planes needs
         * to be considered for actual size calculation
         */
        create_dumb.height = height; //For plane0
        create_dumb.height += height/2; //For plane1
    } else {
        create_dumb.height = height;
    }

    int ret = ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb);
    if (ret) {
        fprintf(stderr, "Failed to create dumb buffer: %s\n", strerror(errno));
        return -1;
    }

    *handle = create_dumb.handle;
    *pitch  = create_dumb.pitch;
    *size   = create_dumb.size;

    return 0;
}

int create_framebuffer(int fd, uint32_t width, uint32_t height, uint32_t handle,
                       uint32_t pitch, uint32_t pixel_format, uint32_t *buf_id)
{
    uint32_t bo_handles[4] = {handle};
    uint32_t pitches[4]    = {pitch};
    uint32_t offsets[4]    = {0};

    if (pixel_format == DRM_FORMAT_NV12) {
        bo_handles[1] = handle;
        pitches[1] = pitch;
        offsets[1] = pitch * height;
    }

    int ret = drmModeAddFB2(fd, width, height, pixel_format,
                            bo_handles, pitches, offsets, buf_id, 0);
    if (ret) {
        printf("Failed to create display frame buffer\n");
        return -1;
    }

    return 0;
}


static int drm_create_fb(drm_dev_t *handle, drm_buffer_t *buf)
{
    struct drm_mode_map_dumb mreq = {0};
    int ret = 0;
    uint32_t pitch, bpp;
    uint64_t size;

    bpp = (buf->pixel_format == DRM_FORMAT_NV12) ? 8 : 24;

    if (create_dumb_buffer(handle->fd, buf->width, buf->height, bpp, buf->pixel_format,
                           &buf->handle, &pitch, &size) < 0) {
        printf("%s: create_dumb_buffer failed\n", __func__);
        return -1;
    }

    buf->size   = size;
    buf->stride = pitch;

	printf("%s: the buffer size %d pitch %d\n", __func__, size, pitch);

    if (create_framebuffer(handle->fd, buf->width, buf->height,
                           buf->handle, pitch, buf->pixel_format, &buf->fb) < 0) {
        printf("create_framebuffer failed\n");
        goto err_destroy;
    }

    memset(&mreq, 0, sizeof(mreq));
    mreq.handle = buf->handle;
    ret = drmIoctl(handle->fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
    if (ret) {
        printf("cannot map dumb buffer\n");
        goto err_fb;
    }

    buf->fb_base = mmap(0, buf->size, PROT_READ | PROT_WRITE, MAP_SHARED,
            handle->fd, mreq.offset);
    if (buf->fb_base == MAP_FAILED) {
        printf("cannot mmap dumb buffer\n");
        goto err_fb;
    }

    memset(buf->fb_base, 255, buf->size);

    // Get DMA-BUF file descriptor for V4L2 import
    ret = drmPrimeHandleToFD(handle->fd, buf->handle, DRM_CLOEXEC, &buf->dma_buf_fd);
    if (ret) {
        printf("Failed to get DMA-BUF file descriptor: %s\n", strerror(errno));
        buf->dma_buf_fd = -1;  // Mark as invalid
        goto err_unmap;
    }

    printf("%s: Created DMA-BUF fd: %d for handle: %u (for V4L2 import)\n",
           __func__, buf->dma_buf_fd, buf->handle);

    return 0;

err_unmap:
    munmap(buf->fb_base, buf->size);


err_fb:
    drmModeRmFB(handle->fd, buf->fb);
err_destroy:
    memset(&mreq, 0, sizeof(mreq));
    mreq.handle = buf->handle;
    drmIoctl(handle->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &mreq);

    return ret;
}

static void drm_destroy_fb(drm_dev_t *handle, drm_buffer_t *buf)
{
    struct drm_mode_destroy_dumb dreq;

    munmap(buf->fb_base, buf->size);
    drmModeRmFB(handle->fd, buf->fb);

    // Close DMA-BUF file descriptor if valid
    if (buf->dma_buf_fd >= 0) {
        close(buf->dma_buf_fd);
        buf->dma_buf_fd = -1;
    }

    memset(&dreq, 0, sizeof(dreq));
    dreq.handle = buf->handle;
    drmIoctl(handle->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
    buf->fb_base = NULL;
}

static int modset_fb(drm_dev_t *handle)
{
    int ret = 0;
    for (int i = 0; i < handle->num_display_buf; i++) {
        ret = drm_create_fb(handle, &handle->bufs[i]);
        if (ret) {
            drm_destroy_fb(handle, &handle->bufs[i]);
            return ret;
        }
    }

    return 0;
}

static int modeset_setup_dev(drm_dev_t *handle, drmModeRes *res, drmModeConnector *conn, int fourcc)
{
    int ret;

    if (conn->connection != DRM_MODE_CONNECTED) {
        printf("ignore unused connector %u\n", conn->connector_id);
        return -1;
    }

    if (conn->count_modes == 0) {
        printf("no valid mode for connector %u\n", conn->connector_id);
        return -1;
    }

    handle->count_modes = conn->count_modes;
    handle->mode_list = (drmModeModeInfo *)malloc(sizeof(drmModeModeInfo) * conn->count_modes);
    if (handle->mode_list == NULL) {
        printf("malloc mode list failed\n");
        return -1;
    }

    for (int i = 0; i < conn->count_modes; i++) {
       printf("mode for connector%u is %ux%u\n", conn->connector_id,
                    conn->modes[i].hdisplay, conn->modes[i].vdisplay);
        memcpy(&handle->mode_list[i], &conn->modes[i], sizeof(drmModeModeInfo));
    }

    memcpy(&handle->mode, &conn->modes[0], sizeof(handle->mode));

    int corrected_fmt = format_conversion(fourcc);
    if (corrected_fmt != -1)
        fourcc = corrected_fmt;

    for (int i = 0; i < handle->num_display_buf; i++) {
        handle->bufs[i].width = conn->modes[0].hdisplay;
        handle->bufs[i].height = conn->modes[0].vdisplay;
        handle->bufs[i].pixel_format = fourcc;
    }

    ret = modeset_find_crtc(handle, res, conn);
    if (ret) {
        printf("cannot find suitable CRTC for connector %d\n", conn->connector_id);
        return -1;
    }

    return modset_fb(handle);
}

static int drm_get_dma_buf_fd(void *handle, int buffer_index)
{
    drm_dev_t *drm_dev = (drm_dev_t *)handle;

    if (!drm_dev || buffer_index < 0 || buffer_index >= drm_dev->num_display_buf) {
        printf("Invalid handle or buffer index\n");
        return -1;
    }

    return drm_dev->bufs[buffer_index].dma_buf_fd;
}

static uint32_t get_prop_id(int fd, uint32_t obj, uint32_t type, const char *name)
{
    drmModeObjectPropertiesPtr props = drmModeObjectGetProperties(fd, obj, type);
    if (!props) {
        printf("drmModeObjectGetProperties failed for %s\n", name);
        return -1;
    }

    for (uint32_t i = 0; i < props->count_props; ++i) {
        drmModePropertyPtr prop = drmModeGetProperty(fd, props->props[i]);
        if (!prop) {
            printf("Failed to get drm mode property\n");
            return -1;
        }
        if (!strcmp(prop->name, name)) {
            uint32_t id = prop->prop_id;
            drmModeFreeProperty(prop);
            drmModeFreeObjectProperties(props);

            return id;
        }
        drmModeFreeProperty(prop);
    }

    fprintf(stderr, "Property %s not found\n", name);
    drmModeFreeObjectProperties(props);

    return 0;
}
//struct timespec last;

static void vblank_plane_handler(int fd, unsigned int frame,
        unsigned int sec, unsigned int usec, void *data)
{
    unsigned cur_index;
    drm_buffer_t *buf;
    drm_dev_t* drm_dev = (drm_dev_t*)data;
    int ret = 0;

#if 0
	struct timespec now;
	(void)clock_gettime(CLOCK_MONOTONIC, &now);

	if (first++) {
		long seconds = now.tv_sec - last.tv_sec;
		long nanoseconds = now.tv_nsec - last.tv_nsec;

		if (nanoseconds < 0) {
			seconds -= 1;
			nanoseconds += 1000000000L; // 1 second in nanoseconds
		}

		double elapsed_ms = (seconds * 1000.0) + (nanoseconds / 1000000.0);

		printf("DRM Time taken: %.3f ms\n", elapsed_ms);
	}
	last = now;
#endif

    /* Prepare atomic commit for next buffer */
    ret = read(drm_dev->pipefd[PIPE_READ_INDEX], &cur_index, sizeof(int));
    if (ret  > 0) {
        buf = &drm_dev->bufs[cur_index % drm_dev->num_display_buf];
        if (!drm_dev->atomic_req) {
            printf("drmModeAtomicAlloc failed\n");
            return;
        }

        drmModeAtomicAddProperty(drm_dev->atomic_req, drm_dev->plane_id,
                drm_dev->props.fb_id, buf->fb);

        if (drmModeAtomicCommit(drm_dev->fd, drm_dev->atomic_req,
                    drm_dev->atomic_flags, drm_dev) < 0) {
            printf("drmModeAtomicCommit failed\n");
        }

        push_buffer((cur_index-1) % drm_dev->num_display_buf);
    }

    drmVBlank vbl = {0};
    vbl.request.type = DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT;
    vbl.request.sequence = 1;
    vbl.request.signal = (uintptr_t)drm_dev;
    drmWaitVBlank(fd, &vbl);
}

/* TODO:: Replace vblank with page flip */
static void *plane_handler_thread(void *arg)
{
    drm_dev_t* drm_dev = (drm_dev_t*)arg;
    drmEventContext evctx;
    drm_context_t ctx;
    drmVBlank vbl;
    memset(&evctx, 0, sizeof evctx);
    memset(&vbl, 0, sizeof vbl);
    memset(&ctx, 0, sizeof(ctx));
    evctx.version = DRM_EVENT_CONTEXT_VERSION;
    evctx.vblank_handler = vblank_plane_handler;
    vbl.request.type = DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT;
    vbl.request.sequence = 1;
    vbl.request.signal = drm_dev;

    drmWaitVBlank(drm_dev->fd, &vbl);

    while (drm_dev->is_thread_running){
        drmHandleEvent(drm_dev->fd, &evctx);
    }
    return NULL;
}

static void init_plane_property_ids(drm_dev_t *handle)
{
    handle->props.fb_id = get_prop_id(handle->fd, handle->plane_id, DRM_MODE_OBJECT_PLANE, "FB_ID");
    handle->props.crtc_id = get_prop_id(handle->fd, handle->plane_id, DRM_MODE_OBJECT_PLANE, "CRTC_ID");
    handle->props.src_x = get_prop_id(handle->fd, handle->plane_id, DRM_MODE_OBJECT_PLANE, "SRC_X");
    handle->props.src_y = get_prop_id(handle->fd, handle->plane_id, DRM_MODE_OBJECT_PLANE, "SRC_Y");
    handle->props.src_w = get_prop_id(handle->fd, handle->plane_id, DRM_MODE_OBJECT_PLANE, "SRC_W");
    handle->props.src_h = get_prop_id(handle->fd, handle->plane_id, DRM_MODE_OBJECT_PLANE, "SRC_H");
    handle->props.crtc_x = get_prop_id(handle->fd, handle->plane_id, DRM_MODE_OBJECT_PLANE, "CRTC_X");
    handle->props.crtc_y = get_prop_id(handle->fd, handle->plane_id, DRM_MODE_OBJECT_PLANE, "CRTC_Y");
    handle->props.crtc_w = get_prop_id(handle->fd, handle->plane_id, DRM_MODE_OBJECT_PLANE, "CRTC_W");
    handle->props.crtc_h = get_prop_id(handle->fd, handle->plane_id, DRM_MODE_OBJECT_PLANE, "CRTC_H");
}

int make_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);  // Get current flags
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }

    // Add O_NONBLOCK flag
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }

    return 0;
}

static int init_vblank_thread(drm_dev_t *drm_dev)
{
    if (pipe(drm_dev->pipefd) == -1) {
        perror("pipe");
        return -1;
    }

	//make_non_blocking(drm_dev->pipefd[PIPE_READ_INDEX]);

    drm_dev->is_thread_running = 1;
    pthread_create(&drm_dev->vblank_thread, NULL, plane_handler_thread, drm_dev);

    return 0;
}

static int drm_prepare(drm_dev_t *handle, int fourcc)
{
    int ret;

    /* Enable access to all plane types (primary, overlay, cursor) */
    if (drmSetClientCap(handle->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) < 0) {
        printf("Failed to enable universal planes: %s\n", strerror(errno));
        return -1;
    }

    if (drmSetClientCap(handle->fd, DRM_CLIENT_CAP_ATOMIC, 1) < 0) {
        printf("Failed to enable atomic capability\n");
        return -1;
    }


    drmModeRes *res;
    res = drmModeGetResources(handle->fd);
    if (!res) {
        printf("cannot retrieve DRM resources\n");
        return -1;
    }

    printf("DRM Resources: %d connectors, %d encoders, %d crtcs\n",
           res->count_connectors, res->count_encoders, res->count_crtcs);

    for (int i = 0; i < res->count_connectors; i++) {
        printf("Connector[%d]: ID=%u\n", i, res->connectors[i]);
    }

    for (int i = 0; i < res->count_crtcs; i++) {
        printf("CRTC[%d]: ID=%u\n", i, res->crtcs[i]);
    }

    for (int i = 0; i < res->count_encoders; i++) {
        printf("Encoder[%d]: ID=%u\n", i, res->encoders[i]);
    }

    drmModeConnector *conn;
    for (int i = 0; i < res->count_connectors; i++) {
        conn = drmModeGetConnector(handle->fd, res->connectors[i]);
        if (!conn) {
            printf("cannot retrieve DRM connector %d:%u (%d)\n",
                    i, res->connectors[i], errno);
            continue;
        }
        handle->conn = conn->connector_id;

        printf("Connector[%d]: ID=%u, connection=%s, count_modes=%d, count_encoders=%d\n",
               i, conn->connector_id,
               (conn->connection == DRM_MODE_CONNECTED) ? "CONNECTED" :
               (conn->connection == DRM_MODE_DISCONNECTED) ? "DISCONNECTED" : "UNKNOWN",
               conn->count_modes, conn->count_encoders);

        for (int e = 0; e < conn->count_encoders; e++) {
            printf("  Encoder[%d]: ID=%u\n", e, conn->encoders[e]);
        }

        ret = modeset_setup_dev(handle, res, conn, fourcc);
        if (ret) {
            printf("cannot setup device for connector %u\n", res->connectors[i]);
            drmModeFreeConnector(conn);
            handle->conn = -1;
            continue;
        }

        drmModeFreeConnector(conn);
        break;
    }

    if(handle->conn == -1) {
        printf("Failed to find a corresponding connector\n");
        return -1;
    }

    /* crtc_index is required to match compatible planes via possible_crtcs bitmask */
    handle->crtc_index = -1;
    for (int i = 0; i < res->count_crtcs; i++)
        if (res->crtcs[i] == handle->crtc_id) {
            handle->crtc_index = i;
            break;
        }

    if (handle->crtc_index < 0) {
        printf("CRTC not found in resource list\n");
        drmModeFreeResources(res);
        return -1;
    }

    drmModePlaneRes *pres = drmModeGetPlaneResources(handle->fd);
    if (!pres) {
        printf("drmModeGetPlaneResources failed\n");
        drmModeFreeResources(res);
        return -1;
    }

    printf("DRM Plane Resources: %d planes available\n", pres->count_planes);
    for (uint32_t i = 0; i < pres->count_planes; i++) {
        printf("Plane[%d]: ID=%u\n", i, pres->planes[i]);
    }

    int corrected_fmt = format_conversion(fourcc);
    if (corrected_fmt != -1)
        fourcc = corrected_fmt;

    handle->plane_id = -1;
    for (uint32_t i = 0; i < pres->count_planes; i++) {
        drmModePlane *p = drmModeGetPlane(handle->fd, pres->planes[i]);
        if (!p) continue;

        printf("  Plane[%d]: ID=%u, possible_crtcs=0x%x, count_formats=%d\n",
               i, p->plane_id, p->possible_crtcs, p->count_formats);

        if (p->possible_crtcs & (1 << handle->crtc_index)) {
            printf("    Compatible with CRTC index %d\n", handle->crtc_index);
            for (uint32_t f = 0; f < p->count_formats; f++) {
                printf("    Format[%d]: 0x%x (%c%c%c%c)\n", f, p->formats[f],
                       (p->formats[f] >> 0) & 0xFF,
                       (p->formats[f] >> 8) & 0xFF,
                       (p->formats[f] >> 16) & 0xFF,
                       (p->formats[f] >> 24) & 0xFF);
                if (p->formats[f] == fourcc) {
                    handle->plane_id = p->plane_id;
                    printf("    Found matching format for plane %u\n", p->plane_id);
                    break;
                }
            }
        } else {
            printf("    Not compatible with CRTC index %d\n", handle->crtc_index);
        }
        drmModeFreePlane(p);

        if (handle->plane_id != -1)
            break;
    }

    printf("Plane_id: %d\n", handle->plane_id);

    drmModeFreePlaneResources(pres);
    drmModeFreeResources(res);

    if (handle->plane_id == -1) {
        printf("no %d capable plane found for CRTC %u\n", fourcc, handle->crtc_id);
        return -1;
    }

    ret = drmSetMaster(handle->fd);
    if (ret < 0) {
        printf("drmSetMaster failed, error: %s\n", strerror(errno));
        return -1;
    }

    handle->crtc_set = 0;
    // Init plane property IDs
    init_plane_property_ids(handle);

    return 0;
}

static void *display_drm_open(int busid, int fourcc, int num_display_buf)
{
    int ret = 0;
    int i = 0;
    drm_dev_t *drm_dev = NULL;
    char drm_dev_name[64];
	buf_index = 0;
	first = 0;

    drm_dev = (drm_dev_t*)malloc(sizeof(drm_dev_t));
    if (NULL == drm_dev) {
        printf("malloc drm_dev failed\n");
        return NULL;
    }
    memset(drm_dev, 0, sizeof(drm_dev_t));
    drm_dev->num_display_buf = num_display_buf;

    drm_dev->bufs = (drm_buffer_t*)malloc(sizeof(drm_buffer_t) * num_display_buf);
    if (NULL == drm_dev->bufs) {
        printf("malloc drm_buffer failed\n");
        return NULL;
    }
    memset(drm_dev->bufs, 0, sizeof(drm_buffer_t) * num_display_buf);

    // Initialize DMA-BUF file descriptors to invalid value
    for (int i = 0; i < num_display_buf; i++) {
        drm_dev->bufs[i].dma_buf_fd = -1;
    }

    sprintf(drm_dev_name, "/dev/dri/card%d", busid);
    ret = drm_open(drm_dev, drm_dev_name);
    if (0 != ret) {
        printf("drm_open /dev/dri/card%d failed\n", busid);
        printf("start to iterate /dev/dri\n");
    } else {
        goto success;
    }

loop:
    if (i == busid) {
        i++;
        goto loop;
    }
    sprintf(drm_dev_name, "/dev/dri/card%d", i++);
    ret = drm_open(drm_dev, drm_dev_name);
    if (-1 == ret) {
        printf("can not open drm_dev\n");
        printf("iterate over\n");
        goto err_free_handle;
    } else if (-2 == ret) {
        goto loop;
    }

success:
    ret = drm_prepare(drm_dev, fourcc);
    if (ret) {
        printf("drm_prepare return failed\n");
        goto err_close_fd;
    }
    yuv_to_rgb_bt601_limit_range_init();
    return drm_dev;

err_close_fd:
    close(drm_dev->fd);
err_free_handle:
    free(drm_dev);

    return NULL;
}

static void display_drm_close(void *handle)
{
    drm_dev_t *drm_dev = (drm_dev_t*)handle;
    if (!drm_dev)
        return;

    if(drm_dev->fd > 0) {
        drmDropMaster(drm_dev->fd);
        close(drm_dev->fd);
        if (drm_dev->mode_list)
            free(drm_dev->mode_list);
    }

    if (drm_dev->atomic_req)
        drmModeAtomicFree(drm_dev->atomic_req);

    free(drm_dev);

    return;
}

static int display_drm_adaptive_resolution(drm_dev_t *handle, int width, int height, int fourcc)
{
    int ret = 0;

    drmModeModeInfo *mode = NULL;
    for (int i = 0; i < handle->count_modes ; i++) {
        if ((width == handle->mode_list[i].hdisplay) && (height == handle->mode_list[i].vdisplay)) {
            mode = &handle->mode_list[i];
            break;
        }
    }

    if (!mode) {
        printf("%s drm does not support %d*%d resolution\n", __func__, width, height);
        return -1;
    }

    int corrected_fmt = format_conversion(fourcc);
    if (corrected_fmt != -1)
        fourcc = corrected_fmt;

    memcpy(&handle->mode, mode, sizeof(drmModeModeInfo));

    for (int i = 0; i < handle->num_display_buf; i++) {
        handle->bufs[i].width = width;
        handle->bufs[i].height = height;
        handle->bufs[i].pixel_format = fourcc;

        if (handle->bufs[i].fb_base != NULL)
            drm_destroy_fb(handle, &handle->bufs[i]);

        ret =  drm_create_fb(handle, &handle->bufs[i]);
        if (ret) {
            drm_destroy_fb(handle, &handle->bufs[i]);
            printf("Failed to create display frame buffer\n");
            return -1;
        }
    }

    handle->crtc_set = 0;

    return 0;
}

void fill_plane(void* dest_buf, void* src_buf, int width, int height, int stride)
{
    int dest_offset = 0;
    int src_offset = 0;

    if (src_buf) {
        for(int i = 0; i < height; i++) {
            memcpy(dest_buf + dest_offset, src_buf + src_offset, width);
            dest_offset += width;
            src_offset += stride;
        }
    }
}

static void drm_add_atomic_property(drm_dev_t *drm_dev)
{
    drm_dev->atomic_flags = DRM_MODE_ATOMIC_NONBLOCK | DRM_MODE_PAGE_FLIP_EVENT;
    drm_dev->atomic_req = drmModeAtomicAlloc();

    if (!drm_dev->atomic_req)
        printf("drmModeAtomicAlloc failed\n");


    drmModeAtomicAddProperty(drm_dev->atomic_req, drm_dev->plane_id, drm_dev->props.crtc_id,
                                    drm_dev->crtc_id);
    drmModeAtomicAddProperty(drm_dev->atomic_req, drm_dev->plane_id, drm_dev->props.src_x, 0);
    drmModeAtomicAddProperty(drm_dev->atomic_req, drm_dev->plane_id, drm_dev->props.src_y, 0);
    drmModeAtomicAddProperty(drm_dev->atomic_req, drm_dev->plane_id, drm_dev->props.src_w,
                                    (drm_dev->width) << 16);
    drmModeAtomicAddProperty(drm_dev->atomic_req, drm_dev->plane_id, drm_dev->props.src_h,
                                    (drm_dev->height) << 16);
    drmModeAtomicAddProperty(drm_dev->atomic_req, drm_dev->plane_id, drm_dev->props.crtc_x, 0);
    drmModeAtomicAddProperty(drm_dev->atomic_req, drm_dev->plane_id, drm_dev->props.crtc_y, 0);
    drmModeAtomicAddProperty(drm_dev->atomic_req, drm_dev->plane_id, drm_dev->props.crtc_w,
                                    drm_dev->width);
    drmModeAtomicAddProperty(drm_dev->atomic_req, drm_dev->plane_id, drm_dev->props.crtc_h,
                                    drm_dev->height);
}



static int display_drm_show_mp(void *handle, void *data1, void *data2,
        int width, int height, int fourcc, int iommu)
{
    drm_dev_t *drm_dev = (drm_dev_t*)handle;
    drm_buffer_t *buf;
    int width_align = width;

    if (!drm_dev) {
        printf("ERROR: drm_dev handle is NULL in show_mp\n");
        return -1;
    }

    if (!drm_dev->bufs) {
        printf("ERROR: drm_dev->bufs is NULL in show_mp\n");
        return -1;
    }

    if (drm_dev->num_display_buf == 0) {
        printf("ERROR: num_display_buf is 0 in show_mp\n");
        return -1;
    }

    buf = &drm_dev->bufs[buf_index % drm_dev->num_display_buf];

    if (!buf->fb_base) {
        printf("ERROR: Buffer fb_base is NULL for buffer index %d\n", buf_index % drm_dev->num_display_buf);
        return -1;
    }

    if (iommu)
        width_align = ALIGN(width, WIDTH_ALIGN_SIZE);

    if ((buf->width != width) || (buf->height != height)) {
        display_drm_adaptive_resolution(drm_dev, width, height, fourcc);
    }

    struct timespec before, after;
    (void)clock_gettime(CLOCK_MONOTONIC, &before);

    // For DMA-BUF mode, data is already in the buffer, no copy needed
    if (data1 == NULL) {
        //printf("DEBUG: DMA-BUF mode - data already in buffer, skipping copy\n");
    } else {
        // Traditional mode - copy data to framebuffer
        switch(fourcc) {
            case V4L2_PIX_FMT_NV12M:
                fill_plane(buf->fb_base, data1, width, height, width_align);
                fill_plane(buf->fb_base + (width * height), data2, width, height/2, width_align);
                break;
            case V4L2_PIX_FMT_NV12:
                /*
                 * Display plane has support of NV12
                 * no need of further convertion from NV12 to RGB
                 */
                memcpy(buf->fb_base, data1, (3 * width * height) / 2);
                break;
            case V4L2_PIX_FMT_BGR24:
                //bgr24_to_rgb32(buf->fb_base, data1, width, height);
                break;
            case V4L2_PIX_FMT_RGB24:
                //rgb24_to_rgb32(buf->fb_base, data1, width, height);
                //memcpy(buf->fb_base, data1, width * height * 3);
                memcpy_a55_optimized(buf->fb_base, data1, width * height * 3);
                break;
            case V4L2_PIX_FMT_SBGGR10:
            case V4L2_PIX_FMT_SGBRG10:
            case V4L2_PIX_FMT_SGRBG10:
            case V4L2_PIX_FMT_SRGGB10:
                /*  ISP buffers are 16 bit aligned */
                raw16_to_rgb32(buf->fb_base, data1, width, height, 10);
                break;
            case V4L2_PIX_FMT_SBGGR12:
            case V4L2_PIX_FMT_SGBRG12:
            case V4L2_PIX_FMT_SGRBG12:
            case V4L2_PIX_FMT_SRGGB12:
                raw16_to_rgb32(buf->fb_base, data1, width, height, 12);
                break;
            default:
                printf("%s: Unsupported format: %d\n", __func__, fourcc);
                return -1;
        }
    }
#if 1
    (void)clock_gettime(CLOCK_MONOTONIC, &after);

	/* ---- Calculate the difference ---- */
	long seconds = after.tv_sec - before.tv_sec;
	long nanoseconds = after.tv_nsec - before.tv_nsec;

	if (nanoseconds < 0) {
		seconds -= 1;
        nanoseconds += 1000000000L; // 1 second in nanoseconds
	}

	double elapsed_ms = (seconds * 1000.0) + (nanoseconds / 1000000.0);

	if (count_frames % default_frame_count == 0)
		printf("Time taken: %.3f ms %x\n", elapsed_ms, fourcc);

	count_frames++;
#endif
    if (!drm_dev->crtc_set) {
        drmModeSetCrtc(drm_dev->fd, drm_dev->crtc_id, 0, 0, 0, &drm_dev->conn, 1, &drm_dev->mode);
        drm_dev->crtc_set = 1;
        drm_dev->width  = width;
        drm_dev->height = height;

        if (drm_dev-> vblank_thread) {
            drm_dev->is_thread_running = 0;
            pthread_cancel(drm_dev->vblank_thread);
            pthread_join(drm_dev->vblank_thread, NULL);
            if (drm_dev->atomic_req)
                drmModeAtomicFree(drm_dev->atomic_req);
        }

        drm_add_atomic_property(drm_dev);
        init_vblank_thread(drm_dev);
    }

    if (write(drm_dev->pipefd[PIPE_WRITE_INDEX], &buf_index, sizeof(int)) < 0) {
        printf("%s: write fails %d and erro %s\n", __func__, errno, strerror(errno));
    }

    buf_index++;

    return 0;
}

static int display_drm_show(void *handle, void *data, int width, int height, int fourcc)
{
    drm_dev_t *drm_dev = (drm_dev_t*)handle;
    drm_buffer_t *buf;

    buf = &drm_dev->bufs[buf_index & 0x01];

    if ((buf->width != width) || (buf->height != height)) {
        display_drm_adaptive_resolution(drm_dev, width, height, fourcc);
    }

    switch(fourcc) {
        case V4L2_PIX_FMT_NV16:
            nv16_to_rgb32(buf->fb_base, data, width, height);
            break;
        case V4L2_PIX_FMT_YUYV:
            yuyv_to_rgb32(buf->fb_base, data, width, height);
            break;
        case V4L2_PIX_FMT_NV12:
            memcpy(buf->fb_base, data, (3 * width * height) / 2);
            break;
        case V4L2_PIX_FMT_P010:
            p010_to_rgb32(buf->fb_base, data, width, height, true);
            break;
        case V4L2_PIX_FMT_BGR24:
            bgr24_to_rgb32(buf->fb_base, data, width, height);
            break;
        case V4L2_PIX_FMT_SBGGR8:
        case V4L2_PIX_FMT_SGBRG8:
        case V4L2_PIX_FMT_SGRBG8:
        case V4L2_PIX_FMT_SRGGB8:
            raw8_to_rgb32(buf->fb_base, data, width, height);
            break;
        case V4L2_PIX_FMT_SBGGR10:
        case V4L2_PIX_FMT_SGBRG10:
        case V4L2_PIX_FMT_SGRBG10:
        case V4L2_PIX_FMT_SRGGB10:
            raw16_to_rgb32(buf->fb_base, data, width, height, 10);
            break;
        case V4L2_PIX_FMT_SBGGR12:
        case V4L2_PIX_FMT_SGBRG12:
        case V4L2_PIX_FMT_SGRBG12:
        case V4L2_PIX_FMT_SRGGB12:
            raw16_to_rgb32(buf->fb_base, data, width, height, 12);
            break;
        default:
            printf("%s: Unsupported format: %d\n", __func__, fourcc);
            return -1;
    }

    drmModeSetCrtc(drm_dev->fd, drm_dev->crtc_id, buf->fb, 0, 0, &drm_dev->conn, 1, &drm_dev->mode);
    buf_index++;

    return 0;
}

struct display_ops drm_display_ops = {
    .open = display_drm_open,
    .close = display_drm_close,
    .show = display_drm_show,
    .show_mp = display_drm_show_mp,
    .get_dma_buf_fd = drm_get_dma_buf_fd,
};
