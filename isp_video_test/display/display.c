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
#include <string.h>
#include "display.h"
#ifdef DRM
#include "drm_display.h"
#endif
#include "file_display.h"
#ifdef WAYLAND
#include "wayland/wayland_display.h"
#endif

struct display *create_display(uint8_t type)
{
    struct display *pDisplay = NULL;
    if (type >= VIDEO_DISPLAY_TYPE_MAX) {
        printf("Error: unsupported display type, range: 0 ~ %d\n",
                    VIDEO_DISPLAY_TYPE_MAX - 1);
        return NULL;
    }

    pDisplay = (struct display*)malloc(sizeof(struct display));
    if (pDisplay == NULL) {
        printf("Error: failed to alloc memory\n");
        return NULL;
    }
    memset(pDisplay, 0, sizeof(struct display));

    switch (type) {
#ifdef DRM
    case VIDEO_DISPLAY_TYPE_DRM:
        pDisplay->ops = &drm_display_ops;
        break;
#endif
    case VIDEO_DISPLAY_TYPE_FILE:
        pDisplay->ops = &file_display_ops;
        break;
#ifdef WAYLAND
    case VIDEO_DISPLAY_TYPE_WL:
        pDisplay->ops = &wl_display_ops;
        break;
#endif
    default:
        printf("Error: unsupported display type %d\n", type);
        free(pDisplay);
        pDisplay = NULL;
        break;
    }

    return pDisplay;
}
