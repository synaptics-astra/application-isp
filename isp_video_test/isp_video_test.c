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
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <memory.h>
#include <semaphore.h>
#include <poll.h>
#include <signal.h>
#include <time.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>

#include "display.h"

#define VIDEO_BUFFER_CNT 4
#define VIDEO_FORMAT_CHAR_MAX 16

#define VVCAM_VIDEO_CID_IOMMU_BASE              (V4L2_CID_USER_BASE + 0x3F00)
#define VVCAM_VIDEO_CID_IOMMU_ENABLED           (VVCAM_VIDEO_CID_IOMMU_BASE + 0x0000)
#define VVCAM_VIDEO_CID_MCM_RAW_DUMP_ENABLED    (VVCAM_VIDEO_CID_IOMMU_BASE + 0x0002)

char g_output_path[256] = "";

//#define ISP_CROP_SUPPORT
//#define ISP_SCALE_SUPPORT
//#define MCM_RAW_DUMP_SUPPORT
//#define ISP_IOMMU_SUPPORT

#if defined(ISP_IOMMU_SUPPORT) || defined(MCM_RAW_DUMP_SUPPORT)
static int v4l2_ctrl_set(int CamDevice, int ctrl_id, int ctrl_val)
{
    struct v4l2_ext_controls ctrls;
    struct v4l2_ext_control ctrl;
    int ret = 0;

    memset (&ctrl, 0, sizeof (struct v4l2_ext_control));
    memset (&ctrls, 0, sizeof (struct v4l2_ext_controls));

    ctrl.id = ctrl_id;
    ctrl.value = ctrl_val;

    ctrls.ctrl_class = V4L2_CTRL_CLASS_USER;
    ctrls.controls = &ctrl;
    ctrls.count = 1;

    ret = ioctl (CamDevice, VIDIOC_S_EXT_CTRLS, &ctrls);
    if (ret < 0) {
        printf("Failed to set ctrl with id: %x, val: %d and error %s\n",
                ctrl.id, ctrl.value, strerror(errno));
        return -1;
    }

    printf("V4l2 controls are set with id: %x, val: %d\n", ctrl.id, ctrl.value);

    return 0;
}
#endif

static int ConvertStringToV4l2Format(char *String, int *Format)
{
    if (strncmp(String, "NV12", strlen("NV12")) == 0) {
        *Format = V4L2_PIX_FMT_NV12;
        return 0;
    } else if (strncmp(String, "NM12", strlen("NM12")) == 0) {
        *Format = V4L2_PIX_FMT_NV12M;
        return 0;
    } else if (strncmp(String, "NV16", strlen("NV16")) == 0) {
        *Format = V4L2_PIX_FMT_NV16;
        return 0;
    } else if (strncmp(String, "YUYV", strlen("YUYV")) == 0) {
        *Format = V4L2_PIX_FMT_YUYV;
        return 0;
    } else if (strncmp(String, "BGR24", strlen("BGR24")) == 0) {
        *Format = V4L2_PIX_FMT_BGR24;
        return 0;
    } else if (strncmp(String, "RGB24", strlen("RGB24")) == 0) {
        *Format = V4L2_PIX_FMT_RGB24;
        return 0;
    } else if (strncmp(String, "RAW", strlen("RAW")) == 0) {
        *Format = 0;
        return 0;
    } else if (strncmp(String, "P010", strlen("P010")) == 0) {
        *Format = V4L2_PIX_FMT_P010;
        return 0;
    }
    else {
        return -1;
    }
}

void help(void)
{
    printf("Usage:\n");
    printf("\t./isp_video_test  -w 640 -h 480 -f RAW -m 0 -t 2 -d 0 -n 2 -p /path_to_store_file\n");
#ifdef ISP_IOMMU_SUPPORT
    printf("\t./isp_video_test  -w 1920 -h 1080 -f NV12 -m 0 -t 3 -u 0 -d 4 -p /path_to_store_file\n");
#endif
#ifdef MCM_RAW_DUMP_SUPPORT
    printf("\t./isp_video_test  -w 1920 -h 1080 -f NV12 -m 0 -t 3 -u 0 -r 0 -d 4 -p /path_to_store_file\n");
#endif
#ifdef ISP_SCALE_SUPPORT
    printf("\t./isp_video_test  -w 1280 -h 720 -f NV12 -m 0 -t 0 -d 0 -s\n");
#endif
#ifdef ISP_CROP_SUPPORT
    printf("\t./isp_video_test  -w 1280 -h 720 -f NV12 -m 0 -t 0 -d 0 -c 0_0_1280_720\n");
    printf("\t./isp_video_test  -w 1920 -h 1080 -f NV12 -m 0 -t 0 -d 0 -c 0_0_1280_720 -s\n");
#endif
    printf("Options:\n");
    printf("\t-w  image width\n");
    printf("\t-h  image height\n");
    printf("\t-f  image format NV12, NM12, RAW, RGB24, BGR24, YUV3\n");
    printf("\t-m  sensor mode index\n");
    printf("\t-t  display type 0:drm 2:file 3:Wayland\n");
    printf("\t-d  video device id\n");
    printf("\t-n  number of capture buffers (default %d)\n", VIDEO_BUFFER_CNT);
#ifdef ISP_SCALE_SUPPORT
    printf("\t-s  enable scale\n");
#endif
#ifdef ISP_CROP_SUPPORT
    printf("\t-c  enable crop\n");
#endif
#ifdef ISP_IOMMU_SUPPORT
    printf("\t-u  IOMMU 1:enable 0:disable\n");
#endif
#ifdef MCM_RAW_DUMP_SUPPORT
    printf("\t-r  MCM dump 1:enable 0:disable\n");
#endif
    printf("\t-p  Path to store the output file. If not specified, PWD will be used\n");
    return;
}

static int CamDevice = -1;
static struct display *pDisplay = NULL;
static void stop(int sig);
static int PixelFormat;
static uint8_t g_display_type = 0;  // Global display type for push_buffer
static volatile int display_in_use = 0;  // Flag to track display usage

// Function to verify display structure integrity
static int verify_display_integrity(struct display *disp, const char *context) {
    if (!disp) {
        printf("ERROR [%s]: Display pointer is NULL\n", context);
        return -1;
    }

    if (!disp->ops) {
        printf("ERROR [%s]: Display ops pointer is NULL\n", context);
        return -1;
    }

    // Check critical function pointers
    if (!disp->ops->show_mp) {
        printf("ERROR [%s]: show_mp function pointer is NULL\n", context);
        return -1;
    }

    if (g_display_type == VIDEO_DISPLAY_TYPE_DRM && !disp->ops->get_dma_buf_fd) {
        printf("ERROR [%s]: get_dma_buf_fd function pointer is NULL for DRM display\n", context);
        return -1;
    }

    //printf("DEBUG [%s]: Display structure integrity verified\n", context);
    return 0;
}

#if 1
void push_buffer(unsigned index)
{
    struct v4l2_buffer Buffer;
    struct v4l2_plane planes[2];

    // Verify display integrity before use in push_buffer
    if (verify_display_integrity(pDisplay, "push_buffer") != 0) {
        return;
    }

    // Add buffer index bounds checking
    if (CamDevice < 0) {
        printf("ERROR: Camera device not initialized in push_buffer\n");
        return;
    }

    memset(&Buffer, 0, sizeof(Buffer));
    memset(&planes, 0, sizeof(planes));

    Buffer.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    // Use correct memory type based on display type
    if (g_display_type == VIDEO_DISPLAY_TYPE_DRM) {
        Buffer.memory = V4L2_MEMORY_DMABUF;
        // For DMA-BUF, get file descriptor from display
        if (pDisplay && pDisplay->ops && pDisplay->ops->get_dma_buf_fd) {
            int dma_fd = pDisplay->ops->get_dma_buf_fd(pDisplay->handle, index);
            for (int j = 0; j < ((PixelFormat == V4L2_PIX_FMT_NV12M) ? 2 : 1); j++) {
                planes[j].m.fd = dma_fd;
                planes[j].length = 0; // Will be set by driver
            }
        } else {
            printf("ERROR: Invalid display structure in push_buffer for DRM\n");
            return;
        }
    } else {
        Buffer.memory = V4L2_MEMORY_MMAP;
    }

    Buffer.index    = index;
    Buffer.m.planes = planes;
    if (PixelFormat == V4L2_PIX_FMT_NV12M)
        Buffer.length = 2;
    else
        Buffer.length = 1;

	//printf("%s: QBUF index %d\n", __func__, Buffer.index);
    if (ioctl(CamDevice, VIDIOC_QBUF, &Buffer) != 0)
        printf("VIDIOC_QBUF failed: %s\n", strerror(errno));
}
#else

void push_buffer(unsigned index)
{
    struct v4l2_buffer Buffer;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];  // VIDEO_MAX_PLANES is usually 8, safe for most formats

    memset(&Buffer, 0, sizeof(Buffer));
    memset(planes, 0, sizeof(planes));

    Buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    Buffer.memory = V4L2_MEMORY_MMAP;
    Buffer.index = index;
    Buffer.m.planes = planes;

    // Determine number of planes based on pixel format
    if (PixelFormat == V4L2_PIX_FMT_NV12M)
        Buffer.length = 2;  // NV12M has 2 planes: Y and UV
    else
        Buffer.length = 1;  // Most other formats have 1 plane

    // Step 1: Query buffer info
    if (ioctl(CamDevice, VIDIOC_QUERYBUF, &Buffer) != 0) {
        printf("VIDIOC_QUERYBUF failed for buffer %u: %s\n", index, strerror(errno));
        return;
    }

	printf("%s: QBUF index is %d\n", __func__, Buffer.index);
    // Optional: print buffer and plane info for debugging
    printf("Buffer %u queried: memory offset plane0=%u plane1=%u\n",
           index,
           planes[0].m.mem_offset,
           (Buffer.length > 1) ? planes[1].m.mem_offset : 0);

    // Initialize bytesused to 0 before queueing (important for capture buffers)
    for (unsigned int i = 0; i < Buffer.length; i++) {
        planes[i].bytesused = 0;
    }

    // Step 2: Queue the buffer
    if (ioctl(CamDevice, VIDIOC_QBUF, &Buffer) != 0) {
        printf("VIDIOC_QBUF failed for buffer %u: %s\n", index, strerror(errno));
        return;
    }

    printf("Buffer %u queued successfully.\n", index);
}
#endif


int main(int argc, char **argv)
{
    int Ch;
    int Width;
    int Height;
    int SensorMode = 0;
    int Device = 0;
    int BufferCount = VIDEO_BUFFER_CNT;
    char FormartString[VIDEO_FORMAT_CHAR_MAX + 1];
    uint8_t display_type = 0;
#ifdef ISP_SCALE_SUPPORT
    int scale_enable = 0;
#endif
#ifdef ISP_IOMMU_SUPPORT
    int iommu_enable = 0;
#endif
#ifdef MCM_RAW_DUMP_SUPPORT
    int mcm_dump_enable = 0;
#endif
#ifdef ISP_CROP_SUPPORT
    int crop_enable = 0;
    int crop_left = 0;
    int crop_top = 0;
    int crop_width = 0;
    int crop_height = 0;
    char* crop_token = NULL;
#endif
    int ret = 1;

    if (argc < 11)
        help();

    while((Ch = getopt(argc, argv, "w:h:f:m:t:d:c:u:r:s::p:n:")) != -1) {
        switch(Ch) {
            case 'w':
                Width = atoi(optarg);
                break;
            case 'h':
                Height = atoi(optarg);
                break;
            case 'f':
                strncpy(FormartString, optarg, sizeof(char) * VIDEO_FORMAT_CHAR_MAX);
                FormartString[VIDEO_FORMAT_CHAR_MAX] = '\0';
                break;
            case 'm':
                SensorMode = atoi(optarg);
                break;
            case 'd':
                Device = atoi(optarg);
                break;
            case 'n':
                BufferCount = atoi(optarg);
                if (BufferCount <= 0) BufferCount = 1;
                break;
            case 't':
                display_type = atoi(optarg);
                break;
#ifdef ISP_SCALE_SUPPORT
            case 's':
                scale_enable = 1;
                break;
#endif
#ifdef ISP_IOMMU_SUPPORT
            case 'u':
                iommu_enable = atoi(optarg);
                break;
#endif
#ifdef MCM_RAW_DUMP_SUPPORT
            case 'r':
                mcm_dump_enable = atoi(optarg);
                break;
#endif
#ifdef ISP_CROP_SUPPORT
            case 'c':
                crop_enable = 1;
                if (optarg != NULL) {
                    crop_token = strtok(optarg, "_");
                    crop_left = atoi(crop_token);
                    crop_token = strtok(NULL, "_");
                    crop_top = atoi(crop_token);
                    crop_token = strtok(NULL, "_");
                    crop_width = atoi(crop_token);
                    crop_token = strtok(NULL, "_");
                    crop_height = atoi(crop_token);
                    if (crop_left < 0 || crop_top < 0 || crop_width <= 0 || crop_height <= 0) {
                        printf("invalid crop from (%d, %d), size (wxh): %dx%d",
                                crop_left, crop_top, crop_width, crop_height);
                        return -1;
                    }
                }
                else
                {
                    printf("input format of -c is error!\n");
                    return -1;
                }
                break;
#endif
            case 'p':
                strncpy(g_output_path, optarg, sizeof(g_output_path) - 1);
                g_output_path[sizeof(g_output_path) - 1] = '\0';
                break;
            default:
                help();
                return -1;
        }
    }

    if (ConvertStringToV4l2Format(FormartString, &PixelFormat) != 0) {
        printf("Not Support Format %s\n", FormartString);
        return -1;
    }

    printf("Width %d Height %d Format %s Mode %d Device %d\n",
            Width, Height, FormartString, SensorMode, Device);

    signal(SIGINT, stop);
    signal(SIGTERM, stop);

    char CamDeviceName[64];
    sprintf(CamDeviceName, "/dev/video%d", Device);
    CamDevice = open(CamDeviceName, O_RDWR | O_NONBLOCK);
    if (CamDevice < 0) {
        printf("Open camera device %s failed\n", CamDeviceName);
        return -1;
    }
#ifdef ISP_IOMMU_SUPPORT
    if (v4l2_ctrl_set(CamDevice, VVCAM_VIDEO_CID_IOMMU_ENABLED, iommu_enable) != 0) {
        printf("Set IOMMU failed \n");
        goto ErrCloseCamDevice;
    }
#endif
#ifdef MCM_RAW_DUMP_SUPPORT
    if (v4l2_ctrl_set(CamDevice, VVCAM_VIDEO_CID_MCM_RAW_DUMP_ENABLED, mcm_dump_enable) != 0) {
        printf("Set Raw frame dump failed \n");
        goto ErrCloseCamDevice;
    }
#endif
    struct v4l2_capability CamerCapbility;
    if (ioctl(CamDevice, VIDIOC_QUERYCAP, &CamerCapbility) != 0) {
        printf("VIDIOC_QUERYCAP failed: %s\n", strerror(errno));
        goto ErrCloseCamDevice;
    }

    struct v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc, 0, sizeof(fmtdesc));
    while(1) {

        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        if (ioctl(CamDevice, VIDIOC_ENUM_FMT, &fmtdesc) == 0)
            printf("Suport Format %s\n",(char *)&fmtdesc.pixelformat);
        else
            break;
        fmtdesc.index++;

        if (PixelFormat == 0) {
            switch (fmtdesc.pixelformat) {
                case V4L2_PIX_FMT_SBGGR8:
                case V4L2_PIX_FMT_SGBRG8:
                case V4L2_PIX_FMT_SGRBG8:
                case V4L2_PIX_FMT_SRGGB8:
                case V4L2_PIX_FMT_SBGGR10:
                case V4L2_PIX_FMT_SGBRG10:
                case V4L2_PIX_FMT_SGRBG10:
                case V4L2_PIX_FMT_SRGGB10:
                case V4L2_PIX_FMT_SBGGR12:
                case V4L2_PIX_FMT_SGBRG12:
                case V4L2_PIX_FMT_SGRBG12:
                case V4L2_PIX_FMT_SRGGB12:
                    PixelFormat = fmtdesc.pixelformat;
                    break;
                default:
                    break;
            }
        }
    }

    if (PixelFormat == 0) {
        printf("Not support raw format\n");
        goto ErrCloseCamDevice;
    }
#ifdef ISP_CROP_SUPPORT
    if (crop_enable)
    {
        struct v4l2_selection sel;
        memset(&sel, 0, sizeof(sel));
        sel.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        sel.target = V4L2_SEL_TGT_CROP_BOUNDS;

        ret = ioctl(CamDevice, VIDIOC_G_SELECTION, &sel);
        if (ret) {
            printf("Video Driver: Get Crop Bounds Failed: %s\n", strerror(errno));
            goto skip_crop;
        }
        printf("Video Driver: Get Crop Bounds from (%d, %d), size: %dx%d\n",
                sel.r.left, sel.r.top, sel.r.width, sel.r.height);

        sel.target = V4L2_SEL_TGT_CROP_DEFAULT;
        ret = ioctl(CamDevice, VIDIOC_G_SELECTION, &sel);
        if (ret) {
            printf("Video Driver: Get Default Crop Size Failed: %s\n", strerror(errno));
            goto skip_crop;
        }
        printf("Video Driver: Get Default Crop from (%d, %d), size: %dx%d\n",
                sel.r.left, sel.r.top, sel.r.width, sel.r.height);

        // Get current crop bounds for validation
        struct v4l2_selection bounds_sel;
        memset(&bounds_sel, 0, sizeof(bounds_sel));
        bounds_sel.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        bounds_sel.target = V4L2_SEL_TGT_CROP_BOUNDS;
        ret = ioctl(CamDevice, VIDIOC_G_SELECTION, &bounds_sel);
        if (ret) {
            printf("Video Driver: Failed to get crop bounds for validation: %s\n", strerror(errno));
            goto skip_crop;
        }

        sel.target = V4L2_SEL_TGT_CROP;
        if (crop_left >= 0 && crop_top >= 0 && crop_width > 0 && crop_height > 0 &&
            (unsigned int)(crop_left + crop_width) <= bounds_sel.r.width &&
            (unsigned int)(crop_top + crop_height) <= bounds_sel.r.height) {
            sel.r.left = crop_left;
            sel.r.top = crop_top;
            sel.r.height = crop_height;
            sel.r.width = crop_width;
            printf("Video Driver: Set Crop from (%d, %d), Size %dx%d\n",
                    crop_left, crop_top, crop_width, crop_height);
        } else if (crop_width == 0 && crop_height == 0) {
            sel.r.left = 0;
            sel.r.top = 0;
            sel.r.height = Height;
            sel.r.width = Width;
            printf("Video Driver: Set Crop from (0, 0), Size %dx%d\n", Width, Height);
        } else {
            printf("Not Support crop from (%d, %d), size: {crop_width:%d crop_height:%d}\n",
                    crop_left, crop_top, crop_width, crop_height);
            printf("Available bounds: (%d, %d), size: %dx%d\n",
                    bounds_sel.r.left, bounds_sel.r.top, bounds_sel.r.width, bounds_sel.r.height);
            return -1;
        }

        printf("==== VIDIOC_S_SELECTION, crop %dx%d\n", sel.r.width, sel.r.height);
        ret = ioctl(CamDevice, VIDIOC_S_SELECTION, &sel);
        if (ret) {
            printf("Video Driver: Set Crop Size Failed %d: %s\n", ret, strerror(errno));
            return -1;
        }

        // Verify the crop was set correctly
        ret = ioctl(CamDevice, VIDIOC_G_SELECTION, &sel);
        if (ret == 0) {
            printf("Video Driver: Verified crop: (%d, %d), size: %dx%d\n",
                    sel.r.left, sel.r.top, sel.r.width, sel.r.height);
        }

        printf("Video Driver: Crop configuration completed successfully\n");
    }
skip_crop:
    ; /* null statement required after label */
#endif
#ifdef ISP_SCALE_SUPPORT
    if (scale_enable)
    {
        struct v4l2_selection sel;
        memset(&sel,0,sizeof(sel));
        sel.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        sel.target = V4L2_SEL_TGT_COMPOSE_DEFAULT;

        ret = ioctl(CamDevice, VIDIOC_G_SELECTION, &sel);
        if (ret)
        {
            printf("Video Test: Get Default Scale Size Failed\n");
            return -1;
        }

        printf("Video Test: Get Default Scale Size %dX%d\n",sel.r.width,sel.r.height);
        printf("Video Test: Set Scale Size %dX%d\n",Width,Height);

        sel.target = V4L2_SEL_TGT_COMPOSE;
        sel.r.top = 0;
        sel.r.left = 0;
        sel.r.height = Height;
        sel.r.width = Width;
        ret = ioctl(CamDevice, VIDIOC_S_SELECTION, &sel);
        if (ret)
        {
            printf("Video Test:Set Scale Size Failed %d\n",ret);
            return -1;
        }
    } ; /* null statement required after label */
#endif
    /* Format configuration */
    struct v4l2_format Format;
    Format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(CamDevice, VIDIOC_G_FMT, &Format) != 0) {
        printf("VIDIOC_G_FMT failed: %s\n", strerror(errno));
    }

    memset(&Format, 0, sizeof(Format));
    Format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    Format.fmt.pix_mp.pixelformat = PixelFormat;
    Format.fmt.pix_mp.width = Width;
    Format.fmt.pix_mp.height = Height;
    if(PixelFormat == V4L2_PIX_FMT_NV12M)
        Format.fmt.pix_mp.num_planes = 2;  // Multi-plane
    else
        Format.fmt.pix_mp.num_planes = 1;  // Single-plane

    if (ioctl(CamDevice, VIDIOC_S_FMT, &Format) != 0)
        printf("VIDIOC_S_FMT failed: %s\n", strerror(errno));

    printf("VIDIOC_S_FMT %dx%d %s\n",Format.fmt.pix_mp.width, Format.fmt.pix_mp.height,
            (char *)&Format.fmt.pix_mp.pixelformat);
    Width = Format.fmt.pix_mp.width;
    Height = Format.fmt.pix_mp.height;

    struct v4l2_requestbuffers RequestBuffers;
    memset(&RequestBuffers, 0, sizeof(RequestBuffers));
    RequestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    // Store display type globally for push_buffer function
    g_display_type = display_type;

    // Create display first to determine buffer count for DRM
    pDisplay = create_display(display_type);
    if (!pDisplay) {
        printf("ERROR: Failed to create display of type %d\n", display_type);
        goto ErrCloseCamDevice;
    }

    // Verify display ops structure is properly initialized
    if (!pDisplay->ops) {
        printf("ERROR: Display ops structure is NULL after creation\n");
        free(pDisplay);
        pDisplay = NULL;
        goto ErrCloseCamDevice;
    }

    printf("Display created successfully with type %d\n", display_type);

    // Verify display integrity after creation
    if (verify_display_integrity(pDisplay, "after_creation") != 0) {
        goto ErrCloseCamDevice;
    }

    // Use DMA-BUF for DRM display, MMAP for others
    if (display_type == VIDEO_DISPLAY_TYPE_DRM) {
        RequestBuffers.memory = V4L2_MEMORY_DMABUF;
        printf("Using DMA-BUF memory for DRM display\n");
        // For DRM, use the BufferCount but it will be aligned with DRM buffer count
        RequestBuffers.count = BufferCount;
    } else {
        RequestBuffers.memory = V4L2_MEMORY_MMAP;
        printf("Using MMAP memory for non-DRM display\n");
        RequestBuffers.count = BufferCount;
    }

    if (pDisplay->ops->open) {
        if (display_type == VIDEO_DISPLAY_TYPE_WL)
            pDisplay->handle = pDisplay->ops->open(1, PixelFormat, BufferCount);
        else
            pDisplay->handle = pDisplay->ops->open(0, PixelFormat, BufferCount);
        if (!pDisplay->handle) {
            printf("ERROR: Failed to open display handle\n");
            goto ErrCloseCamDevice;
        }
        printf("Display opened successfully\n");
    } else {
        printf("ERROR: Display open function is NULL\n");
        goto ErrCloseCamDevice;
    }

    if (ioctl(CamDevice, VIDIOC_REQBUFS, &RequestBuffers) != 0) {
        printf("VIDIOC_REQBUFS failed: %s\n", strerror(errno));
        goto ErrCloseCamDevice;
    }
    printf("RequestBuffers.count: %d\n", RequestBuffers.count);

    //void *BufferVirtAddrp[VIDEO_BUFFER_CNT][2];
    void ***BufferVirtAddrp = malloc(RequestBuffers.count * sizeof(void**));
    struct v4l2_buffer Buffer;
    struct v4l2_plane planes[2];

    for (int i = 0; i < RequestBuffers.count; i++) {
        memset(&Buffer, 0, sizeof(Buffer));
        memset(&planes, 0, sizeof(planes));

        Buffer.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        Buffer.memory   = RequestBuffers.memory;
        Buffer.index    = i;
        Buffer.m.planes = planes;
        if (PixelFormat == V4L2_PIX_FMT_NV12M)
            Buffer.length = 2;
        else
            Buffer.length = 1;

        if (RequestBuffers.memory == V4L2_MEMORY_DMABUF) {
            // For DMA-BUF, get file descriptor from DRM display
            if (!pDisplay->ops->get_dma_buf_fd) {
                printf("ERROR: get_dma_buf_fd function is NULL\n");
                goto ErrCloseCamDevice;
            }

            int dma_fd = pDisplay->ops->get_dma_buf_fd(pDisplay->handle, i);
            if (dma_fd < 0) {
                printf("ERROR: Failed to get DMA-BUF fd for buffer %d\n", i);
                goto ErrCloseCamDevice;
            }

            for (int j = 0; j < Buffer.length; j++) {
                planes[j].m.fd = dma_fd;
                planes[j].length = 0; // Will be set by driver
            }

            printf("Using DMA-BUF fd %d for buffer %d\n", dma_fd, i);
            BufferVirtAddrp[i] = NULL; // No virtual mapping for DMA-BUF
        } else {
            // For MMAP, query buffer and map memory
            if (ioctl(CamDevice, VIDIOC_QUERYBUF, &Buffer) != 0) {
                printf("VIDIOC_QUERYBUF failed: %s\n", strerror(errno));
                goto ErrCloseCamDevice;
            }

            BufferVirtAddrp[i] = malloc(Buffer.length * sizeof(void*));
            for (int j = 0; j < Buffer.length; j++) {
                BufferVirtAddrp[i][j] = mmap(NULL, planes[j].length,
                        PROT_READ | PROT_WRITE, MAP_SHARED,
                        CamDevice, planes[j].m.mem_offset);
                if (BufferVirtAddrp[i][j] == MAP_FAILED) {
                    printf("mmap failed: %s\n", strerror(errno));
                    goto ErrCloseCamDevice;
                }
            }
        }

		//printf("%s: QBUF index %d\n", __func__, Buffer.index);
        if (ioctl(CamDevice, VIDIOC_QBUF, &Buffer) != 0) {
            printf("VIDIOC_QBUF failed: %s\n", strerror(errno));
            goto ErrCloseCamDevice;
        }
    }

    int Type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(CamDevice, VIDIOC_STREAMON, &Type) < 0) {
        printf("VIDIOC_STREAMON failed: %s\n", strerror(errno));
        goto ErrCloseCamDevice;
    }

    struct pollfd Pollfds;
    int timeout = 10;
    Pollfds.fd = CamDevice;
    Pollfds.events = POLLIN | POLLPRI;
    int Retval;

    int run_time = 10;
    while(run_time) {
        //printf("running %d\n", run_time);
        Retval = poll(&Pollfds, 1, -1);
        if (Retval <= 0)
            continue;

        if (ioctl(CamDevice, VIDIOC_DQBUF, &Buffer) != 0) {
            printf("VIDIOC_DQBUF failed: %s\n", strerror(errno));
        } else {
			//printf("%s: DQBUF index %d\n", __func__, Buffer.index);
            //run_time--;
#if 1
            //struct timespec before, after;
            //(void)clock_gettime(CLOCK_MONOTONIC, &before);

            // Validate buffer index bounds
            if (Buffer.index >= RequestBuffers.count) {
                printf("ERROR: Buffer index %d exceeds buffer count %d\n",
                       Buffer.index, RequestBuffers.count);
                goto err_stop;
            }

            void *data2 = NULL;
            if (PixelFormat == V4L2_PIX_FMT_NV12M) {
                if (BufferVirtAddrp[Buffer.index] && BufferVirtAddrp[Buffer.index][1]) {
                    data2 = BufferVirtAddrp[Buffer.index][1];
                } else {
                    printf("ERROR: Invalid buffer virtual address for plane 1 at index %d\n", Buffer.index);
                    goto err_stop;
                }
            }

            // Validate primary buffer data for non-DMA cases
            if (RequestBuffers.memory != V4L2_MEMORY_DMABUF) {
                if (!BufferVirtAddrp[Buffer.index] || !BufferVirtAddrp[Buffer.index][0]) {
                    printf("ERROR: Invalid buffer virtual address for plane 0 at index %d\n", Buffer.index);
                    goto err_stop;
                }
            }

            // Verify display integrity before use
            if (verify_display_integrity(pDisplay, "main_loop") != 0) {
                goto err_stop;
            }

            // Set display in use flag
            display_in_use = 1;

#ifdef ISP_IOMMU_SUPPORT
            if(pDisplay->ops->show_mp(pDisplay->handle,
                        (RequestBuffers.memory == V4L2_MEMORY_DMABUF) ? NULL : BufferVirtAddrp[Buffer.index][0],
                        data2,
                        Width, Height, PixelFormat, iommu_enable)) {
#else
            if(pDisplay->ops->show_mp(pDisplay->handle,
                        (RequestBuffers.memory == V4L2_MEMORY_DMABUF) ? NULL : BufferVirtAddrp[Buffer.index][0],
                        data2,
                        Width, Height, PixelFormat, 0)) {
#endif
                printf("show buffer failed\n");
                display_in_use = 0;  // Clear flag on failure
                goto err_stop;
            }

            // Clear display in use flag
            display_in_use = 0;
#if 0
            (void)clock_gettime(CLOCK_MONOTONIC, &after);

			/* ---- Calculate the difference ---- */
			long seconds = after.tv_sec - before.tv_sec;
			long nanoseconds = after.tv_nsec - before.tv_nsec;

			if (nanoseconds < 0) {
				seconds -= 1;
				nanoseconds += 1000000000L; // 1 second in nanoseconds
			}

			double elapsed_ms = (seconds * 1000.0) + (nanoseconds / 1000000.0);

			printf("Time taken: %.3f ms\n", elapsed_ms);
#endif

            if (display_type != VIDEO_DISPLAY_TYPE_DRM) {
				//printf("%s: QBUF index %d\n", __func__, Buffer.index);
                if (ioctl(CamDevice, VIDIOC_QBUF, &Buffer) != 0)
                    printf("VIDIOC_QBUF failed: %s\n", strerror(errno));
            }
#endif
        }
    }

    if (BufferVirtAddrp) {
        for (int i = 0; i < RequestBuffers.count; i++) {
            if (BufferVirtAddrp[i]) {
                for (int j = 0; j < ((PixelFormat == V4L2_PIX_FMT_NV12M) ? 2 : 1); j++) {
                    if (BufferVirtAddrp[i][j])
                        munmap(BufferVirtAddrp[i][j], planes[j].length);
                }
                free(BufferVirtAddrp[i]);
            }
        }
        free(BufferVirtAddrp);
    }

err_stop:
    Type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (ioctl(CamDevice, VIDIOC_STREAMOFF, &Type)) {
        printf("VIDIOC_STREAMOFF failed: %s\n", strerror(errno));
        return -1;
    }

    if (pDisplay && pDisplay->ops->close) {
        pDisplay->ops->close(pDisplay->handle);
        free(pDisplay);
        pDisplay = NULL;
    }

ErrCloseCamDevice:
    close(CamDevice);
    CamDevice = -1;

    return -1;
}

static void stop(int sig)
{
    int Type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    int retry_count = 0;

    printf("Received signal %d, stopping gracefully...\n", sig);

    if (CamDevice < 0)
        exit(0);

    // Wait for display to finish if in use
    while (display_in_use && retry_count < 100) {
        usleep(10000); // Wait 10ms
        retry_count++;
    }

    if (display_in_use) {
        printf("WARNING: Display still in use after timeout, forcing cleanup\n");
    }

    if (ioctl(CamDevice, VIDIOC_STREAMOFF, &Type)) {
        printf("VIDIOC_STREAMOFF failed: %s\n", strerror(errno));
        exit(0);
    }

    if (pDisplay) {
        // Verify display structure before cleanup
        if (pDisplay->ops && pDisplay->ops->close && pDisplay->handle) {
            pDisplay->ops->close(pDisplay->handle);
        } else {
            printf("WARNING: Display structure corrupted during cleanup\n");
        }
        free(pDisplay);
        pDisplay = NULL;
    }

    close(CamDevice);
    CamDevice = -1;

    printf("Cleanup completed successfully\n");
    exit(0);
}
