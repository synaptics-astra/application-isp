/******************************************************************************\
|* Copyright (c) 2020 by VeriSilicon Holdings Co., Ltd. ("VeriSilicon")       *|
|* All Rights Reserved.                                                       *|
|*                                                                            *|
|* The material in this file is confidential and contains trade secrets of    *|
|* of VeriSilicon.  This is proprietary information owned or licensed by      *|
|* VeriSilicon.  No part of this work may be disclosed, reproduced, copied,   *|
|* transmitted, or used in any way for any purpose, without the express       *|
|* written permission of VeriSilicon.                                         *|
|*                                                                            *|
\******************************************************************************/

#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <common/return_codes.h>
#include <common/misc.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "isi.h"
#include "isi_iss.h"
#include "vvsensor.h"

#include "OS02k10_priv.h"

CREATE_TRACER(OS02k10_INFO , "OS02k10: ", INFO,    0);
CREATE_TRACER(OS02k10_WARN , "OS02k10: ", WARNING, 1);
CREATE_TRACER(OS02k10_ERROR, "OS02k10: ", ERROR,   1);
CREATE_TRACER(OS02k10_DEBUG,     "OS02k10: ", INFO, 1);
CREATE_TRACER(OS02k10_REG_INFO , "OS02k10: ", INFO, 1);
CREATE_TRACER(OS02k10_REG_DEBUG, "OS02k10: ", INFO, 1);

#ifdef SUBDEV_V4L2
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#undef TRACE
#define TRACE(x, ...)
#endif

#define OS02k10_MIN_GAIN_STEP    (1.0f/256.0f)  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain) */

/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static struct vvsensor_mode_s pos02k10_mode_info[] = {
    {
        .index     = 0,
        .size      = {
            .bounds_width  = 1920,
            .bounds_height = 1080,
            .top           = 0,
            .left          = 0,
            .width         = 1920,
            .height        = 1080,
        },
        .fps       = 30 * ISI_FPS_QUANTIZE,
        .hdr_mode  = SENSOR_MODE_LINEAR,
        .bit_width = 12,
        .bayer_pattern = BAYER_BGGR,
    },
    {
        .index     = 1,
        .size      = {
            .bounds_width  = 1920,
            .bounds_height = 1080,
            .top           = 0,
            .left          = 0,
            .width         = 1920,
            .height        = 1080,
        },
        .fps      = 30 * ISI_FPS_QUANTIZE,
        .hdr_mode = SENSOR_MODE_HDR_STITCH,
        .stitching_mode =  SENSOR_STITCHING_L_AND_S,//SENSOR_STITCHING_DUAL_DCG_NOWAIT,
        .bit_width = 10,
        .bayer_pattern = BAYER_BGGR,
    },
    {
        .index     = 2,
        .size      = {
            .bounds_width  = 1920,
            .bounds_height = 1080,
            .top           = 0,
            .left          = 0,
            .width         = 1920,
            .height        = 1080,
        },
        .fps      = 24 * ISI_FPS_QUANTIZE,
        .hdr_mode = SENSOR_MODE_HDR_STITCH,
        .stitching_mode = SENSOR_STITCHING_LINEBYLINE,
        .bit_width = 10,
        .bayer_pattern = BAYER_BGGR,
    }
};

static RESULT OS02k10_IsiSetPowerIss(IsiSensorHandle_t handle, bool_t on) 
{
    RESULT result = RET_SUCCESS;

    int ret = 0;
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);

    int32_t enable = on;
    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_POWER, &enable);
    if (ret != 0) {
        TRACE(OS02k10_ERROR, "%s: sensor set power error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OS02k10_INFO, "%s (exit)\n", __func__);
    return (result);
}

#ifdef SUBDEV_CHAR
static RESULT OS02k10_IsiSetClkIss(IsiSensorHandle_t handle, uint32_t Clk) 
{
    RESULT result = RET_SUCCESS;
    int32_t ret = 0;

    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_CLK, &Clk);
    if (ret != 0) {
        TRACE(OS02k10_ERROR, "%s: sensor set clk error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OS02k10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS02k10_IsiGetClkIss(IsiSensorHandle_t handle, uint32_t *pClk)
{
    RESULT result = RET_SUCCESS;
    int ret = 0;

    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_CLK, pClk);
    if (ret != 0) {
        TRACE(OS02k10_ERROR, "%s: sensor get clk error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OS02k10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS02k10_IsiConfigSCCBIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;
    int ret = 0;
    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);

    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    static IsiBus_t SensorSccbInfo;
    SensorSccbInfo.i2c.slaveAddr = (0x6c >> 1);
    SensorSccbInfo.i2c.addrWidth = 2;
    SensorSccbInfo.i2c.dataWidth = 1;

    struct vvsensor_sccb_cfg_s sensor_sccb_config;
    sensor_sccb_config.slave_addr = SensorSccbInfo.i2c.slaveAddr;
    sensor_sccb_config.addr_byte  = SensorSccbInfo.i2c.addrWidth;
    sensor_sccb_config.data_byte  = SensorSccbInfo.i2c.dataWidth;

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_SENSOR_SCCB_CFG, &sensor_sccb_config);
    if (ret != 0) {
        TRACE(OS02k10_ERROR, "%s: sensor config sccb info error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OS02k10_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}
#endif

static RESULT OS02k10_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig) 
{
    RESULT result = RET_SUCCESS;
    OS02k10_Context_t *pOS02k10Ctx;

    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);

    if (!pConfig || !pConfig->pSensor)
        return (RET_NULL_POINTER);

    pOS02k10Ctx = (OS02k10_Context_t *) malloc(sizeof(OS02k10_Context_t));
    if (!pOS02k10Ctx) {
        TRACE(OS02k10_ERROR, "%s: Can't allocate os02k10 context\n",__func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pOS02k10Ctx, 0, sizeof(OS02k10_Context_t));

    result = HalAddRef(pConfig->halHandle);
    if (result != RET_SUCCESS) {
        free(pOS02k10Ctx);
        return (result);
    }

    pOS02k10Ctx->IsiCtx.HalHandle = pConfig->halHandle;
    pOS02k10Ctx->IsiCtx.pSensor = pConfig->pSensor;
    pOS02k10Ctx->GroupHold = BOOL_FALSE;
    pOS02k10Ctx->OldGain = 0;
    pOS02k10Ctx->OldIntegrationTime = 0;
    pOS02k10Ctx->Configured = BOOL_FALSE;
    pOS02k10Ctx->Streaming = BOOL_FALSE;
    pOS02k10Ctx->TestPattern = BOOL_FALSE;
    pOS02k10Ctx->isAfpsRun = BOOL_FALSE;
    pOS02k10Ctx->SensorMode.index = pConfig->sensorModeIndex;
    pConfig->hSensor = (IsiSensorHandle_t) pOS02k10Ctx;
#ifdef SUBDEV_CHAR
    struct vvsensor_mode_s *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pos02k10_mode_info)/sizeof(struct vvsensor_mode_s); i++) {
        if (pos02k10_mode_info[i].index == pOS02k10Ctx->SensorMode.index) {
            SensorDefaultMode = &(pos02k10_mode_info[i]);
            break;
        }
    }
    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                memcpy(pOS02k10Ctx->SensorRegCfgFile, "OS02k10_mipi2lane_1080p_init.txt", strlen("OS02k10_mipi2lane_1080p_init.txt"));
                break;
            case 1: //3Dol mode
                memcpy(pOS02k10Ctx->SensorRegCfgFile, "OS02k10_mipi2lane_1080p_2dol_init.txt", strlen("OS02k10_mipi2lane_1080p_2dol_init.txt"));
                break;
            case 2: //3Dol mode
                memcpy(pOS02k10Ctx->SensorRegCfgFile, "OS02k10_mipi4lane_1080p_3dol_init.txt", strlen("OS02k10_mipi4lane_1080p_3dol_init.txt"));
                break;
            default:
                break;
        }

        if (access(pOS02k10Ctx->SensorRegCfgFile, F_OK) == 0) {
            pOS02k10Ctx->KernelDriverFlag = 0;
            memcpy(&(pOS02k10Ctx->SensorMode), SensorDefaultMode,
                    sizeof(struct vvsensor_mode_s));
        } else {
            pOS02k10Ctx->KernelDriverFlag = 1;
        }
    } else {
        pOS02k10Ctx->KernelDriverFlag = 1;
    }

    result = OS02k10_IsiSetPowerIss(pOS02k10Ctx, BOOL_TRUE);
    RETURN_RESULT_IF_DIFFERENT(RET_SUCCESS, result);

    uint32_t SensorClkIn = 0;
    if (pOS02k10Ctx->KernelDriverFlag) {
        result = OS02k10_IsiGetClkIss(pOS02k10Ctx, &SensorClkIn);
        RETURN_RESULT_IF_DIFFERENT(RET_SUCCESS, result);
    }

    result = OS02k10_IsiSetClkIss(pOS02k10Ctx, SensorClkIn);
    RETURN_RESULT_IF_DIFFERENT(RET_SUCCESS, result);

    if (!pOS02k10Ctx->KernelDriverFlag) {
        result = OS02k10_IsiConfigSCCBIss(pOS02k10Ctx);
        RETURN_RESULT_IF_DIFFERENT(RET_SUCCESS, result);
    }

    pOS02k10Ctx->pattern = ISI_BPAT_BGGR;
#endif

#ifdef SUBDEV_V4L2
    pOS02k10Ctx->pattern = ISI_BPAT_BGGR;
    pOS02k10Ctx->subdev = HalGetFdHandle(pConfig->halHandle, HAL_MODULE_SENSOR);
    pOS02k10Ctx->KernelDriverFlag = 1;
#endif
    TRACE(OS02k10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS02k10_IsiReleaseIss(IsiSensorHandle_t handle) {
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);

    if (pOS02k10Ctx == NULL)
        return (RET_WRONG_HANDLE);

    (void)OS02k10_IsiSetStreamingIss(pOS02k10Ctx, BOOL_FALSE);
    (void)OS02k10_IsiSetPowerIss(pOS02k10Ctx, BOOL_FALSE);
    (void)HalDelRef(pOS02k10Ctx->IsiCtx.HalHandle);

    MEMSET(pOS02k10Ctx, 0, sizeof(OS02k10_Context_t));
    free(pOS02k10Ctx);
    TRACE(OS02k10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS02k10_IsiReadRegIss(IsiSensorHandle_t handle, const uint32_t Addr, uint32_t *pValue) 
{
    RESULT result = RET_SUCCESS;
    int32_t ret = 0;
    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);

    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    struct vvsensor_sccb_data_s sccb_data;
    sccb_data.addr = Addr;
    sccb_data.data = 0;
    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_READ_REG, &sccb_data);
    if (ret != 0) {
        TRACE(OS02k10_ERROR, "%s: read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    *pValue = sccb_data.data;

    TRACE(OS02k10_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OS02k10_IsiWriteRegIss(IsiSensorHandle_t handle, const uint32_t Addr, const uint32_t Value)
{
    RESULT result = RET_SUCCESS;
    int ret = 0;
    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);

    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    struct vvsensor_sccb_data_s sccb_data;
    sccb_data.addr = Addr;
    sccb_data.data = Value;

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_WRITE_REG, &sccb_data);
    if (ret != 0) {
        TRACE(OS02k10_ERROR, "%s: write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OS02k10_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OS02k10_IsiGetModeIss(IsiSensorHandle_t handle, IsiMode_t *pMode)
{
    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);
    const OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pOS02k10Ctx->SensorMode), sizeof(pOS02k10Ctx->SensorMode));

    TRACE(OS02k10_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static RESULT OS02k10_IsiSetModeIss(IsiSensorHandle_t handle, IsiMode_t *pMode)
{
    int ret = 0;
    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);

    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_SENSOR_MODE, pMode);
    if (ret != 0) {
        TRACE(OS02k10_ERROR, "%s: write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OS02k10_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT OS02k10_IsiEnumModeIss(IsiSensorHandle_t handle, IsiEnumMode_t *pEnumMode)
{
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pos02k10_mode_info)/sizeof(pos02k10_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pos02k10_mode_info)/sizeof(pos02k10_mode_info[0])); i++) {
        if (pos02k10_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pos02k10_mode_info[i],sizeof(IsiMode_t));
            TRACE(OS02k10_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT OS02k10_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps) 
{
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);

    if (pOS02k10Ctx == NULL)
        return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pOS02k10Ctx->SensorMode.bit_width;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pOS02k10Ctx->SensorMode.bayer_pattern;
    pCaps->resolution.width  = pOS02k10Ctx->SensorMode.size.width;
    pCaps->resolution.height = pOS02k10Ctx->SensorMode.size.height;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;
    if (pOS02k10Ctx->SensorMode.index == 2) {
        pCaps->mipiLanes         = ISI_MIPI_4LANES;
    } else {
        pCaps->mipiLanes         = ISI_MIPI_2LANES;
    }

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }
    
    
    TRACE(OS02k10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS02k10_IsiGetRegCfgIss(const char *registerFileName, struct vvsensor_sccb_array_s *array)
{
    if (NULL == registerFileName) {
        TRACE(OS02k10_ERROR, "%s:registerFileName is NULL\n", __func__);
        return (RET_NULL_POINTER);
    }
#ifdef SUBDEV_CHAR
    FILE *fp = NULL;
    fp = fopen(registerFileName, "rb");
    if (!fp) {
        TRACE(OS02k10_ERROR, "%s:load register file  %s error!\n", __func__, registerFileName);
        return (RET_FAILURE);
    }

    if (errno) {
        TRACE(OS02k10_WARN, "%s:Clearing unhandled errno before calling fgets: %d\n", __func__, errno);
        errno = 0;
    }

    char LineBuf[512];
    uint32_t FileTotalLine = 0;
    while (!feof(fp)) {
        if (NULL == fgets(LineBuf, 512, fp)) {
                if (errno) {
                    TRACE(OS02k10_ERROR, "%s:gets linebuf error: %d\n", __func__, errno);
                    return (RET_FAILURE);
                }
        }
        FileTotalLine++;
    }

    array->sccb_data =
        malloc(FileTotalLine * sizeof(struct vvsensor_sccb_data_s));
    if (array->sccb_data == NULL) {
        TRACE(OS02k10_ERROR, "%s:malloc failed NULL Point!\n", __func__, registerFileName);
        return (RET_FAILURE);
    }
    rewind(fp);

    array->count = 0;
    while (!feof(fp)) {
        memset(LineBuf, 0, sizeof(LineBuf));
        if (NULL == fgets(LineBuf, 512, fp)) {
                if (errno) {
                    TRACE(OS02k10_ERROR, "%s:gets linebuf error: %d\n", __func__, errno);
                    return (RET_FAILURE);
                }
        }

        int result = sscanf(LineBuf, "0x%x 0x%x", &(array->sccb_data[array->count].addr), &(array->sccb_data[array->count].data));
        if (result != 2)
            continue;
        array->count++;

    }
#endif

    return 0;
}

static RESULT OS02k10_AecSetModeParameters(IsiSensorHandle_t handle, OS02k10_Context_t *pOS02k10Ctx)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS02k10_INFO, "%s%s: (enter)\n", __func__, pOS02k10Ctx->isAfpsRun ? "(AFPS)" : "");

    pOS02k10Ctx->AecIntegrationTimeIncrement = pOS02k10Ctx->one_line_exp_time;
    pOS02k10Ctx->AecMinIntegrationTime = pOS02k10Ctx->one_line_exp_time * pOS02k10Ctx->MinIntegrationLine;
    pOS02k10Ctx->AecMaxIntegrationTime = pOS02k10Ctx->one_line_exp_time * pOS02k10Ctx->MaxIntegrationLine;

    TRACE(OS02k10_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pOS02k10Ctx->isAfpsRun ? "(AFPS)" : "", pOS02k10Ctx->AecMaxIntegrationTime);

    pOS02k10Ctx->AecGainIncrement = OS02k10_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pOS02k10Ctx->AecCurGain = pOS02k10Ctx->AecMinGain;
    pOS02k10Ctx->AecCurIntegrationTime = 0.0f;
    pOS02k10Ctx->OldGain = 0;
    pOS02k10Ctx->OldIntegrationTime = 0;

    TRACE(OS02k10_INFO, "%s%s: (exit)\n", __func__, pOS02k10Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT OS02k10_IsiSetupIss(IsiSensorHandle_t handle)
{
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    if (!pOS02k10Ctx) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;
    int ret = 0;

    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);

    if (pOS02k10Ctx->Streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    if (pOS02k10Ctx->KernelDriverFlag) {
#ifdef SUBDEV_CHAR

        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_INIT, &(pOS02k10Ctx->SensorMode));
        if (ret != 0) {
            TRACE(OS02k10_ERROR, "%s:sensor init error!\n", __func__);
            return (RET_FAILURE);
        }

        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_SENSOR_MODE, &(pOS02k10Ctx->SensorMode));
        if (ret != 0) {
            TRACE(OS02k10_ERROR, "%s:sensor get mode info error!\n", __func__);
            return (RET_FAILURE);
        }

        /*struct vvcam_ae_info_s ae_info;
        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_AE_INFO, &ae_info);
        if (ret != 0) {
            TRACE(OS02k10_ERROR, "%s:sensor get ae info error!\n",
                  __func__);
            return (RET_FAILURE);
        }
        pOS02k10Ctx->one_line_exp_time =
            (float)ae_info.one_line_exp_time_ns / 1000000000;
        pOS02k10Ctx->MaxIntegrationLine = ae_info.max_integration_time;
        pOS02k10Ctx->MinIntegrationLine = ae_info.min_integration_time;
        pOS02k10Ctx->gain_accuracy = ae_info.gain_accuracy;
        pOS02k10Ctx->AecMinGain = (float)(ae_info.min_gain) / ae_info.gain_accuracy;
        pOS02k10Ctx->AecMaxGain = (float)(ae_info.max_gain) / ae_info.gain_accuracy;

        pOS02k10Ctx->MaxFps  = pOS02k10Ctx->SensorMode.fps;
        pOS02k10Ctx->MinFps  = 1;
        pOS02k10Ctx->CurrFps = pOS02k10Ctx->MaxFps;*/
#endif

#ifdef SUBDEV_V4L2
        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_SENSOR_MODE, &(pOS02k10Ctx->SensorMode));
        if (ret != 0) {
            TRACE(OS02k10_ERROR, "%s:2-sensor get mode info error!\n", __func__);
            return (RET_FAILURE);
        }

        pOS02k10Ctx->one_line_exp_time  = (float) (pOS02k10Ctx->SensorMode.ae_info.one_line_exp_time_ns) / 1000000000;
        pOS02k10Ctx->MaxIntegrationLine = pOS02k10Ctx->SensorMode.ae_info.max_integration_time;
        pOS02k10Ctx->MinIntegrationLine = pOS02k10Ctx->SensorMode.ae_info.min_integration_time;
        pOS02k10Ctx->gain_accuracy      = pOS02k10Ctx->SensorMode.ae_info.gain_accuracy;
        pOS02k10Ctx->AecMaxGain         = (float)(pOS02k10Ctx->SensorMode.ae_info.max_gain) /pOS02k10Ctx->gain_accuracy ;
        pOS02k10Ctx->AecMinGain         = (float)(pOS02k10Ctx->SensorMode.ae_info.min_gain) / pOS02k10Ctx->gain_accuracy ;
        pOS02k10Ctx->MaxFps             = pOS02k10Ctx->SensorMode.fps;
        pOS02k10Ctx->CurrFps            = pOS02k10Ctx->MaxFps;

#endif

    } else {
        struct vvsensor_sccb_array_s array;
        result = OS02k10_IsiGetRegCfgIss(pOS02k10Ctx->SensorRegCfgFile, &array);
        if (result != 0) {
            TRACE(OS02k10_ERROR, "%s:OS02k10_IsiGetRegCfgIss error!\n", __func__);
            return (RET_FAILURE);
        }

        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_WRITE_ARRAY, &array);
        if (ret != 0) {
            TRACE(OS02k10_ERROR, "%s:Sensor Write Reg array error!\n", __func__);
            return (RET_FAILURE);
        }

        switch(pOS02k10Ctx->SensorMode.index)
        {
            case 0:
                pOS02k10Ctx->one_line_exp_time = 0.00006306;
                pOS02k10Ctx->FrameLengthLines = 0x4e2;
                pOS02k10Ctx->CurFrameLengthLines = pOS02k10Ctx->FrameLengthLines;
                pOS02k10Ctx->MaxIntegrationLine = pOS02k10Ctx->CurFrameLengthLines - 8;
                pOS02k10Ctx->MinIntegrationLine = 1;
                pOS02k10Ctx->AecMaxGain = 240;
                pOS02k10Ctx->AecMinGain = 1;
                break;
            case 1:
                pOS02k10Ctx->one_line_exp_time = 0.00006306;
                pOS02k10Ctx->FrameLengthLines = 0x4e2;
                pOS02k10Ctx->CurFrameLengthLines = pOS02k10Ctx->FrameLengthLines;
                pOS02k10Ctx->MaxIntegrationLine = pOS02k10Ctx->CurFrameLengthLines - 8;
                pOS02k10Ctx->MinIntegrationLine = 1;
                pOS02k10Ctx->AecMaxGain = 240;
                pOS02k10Ctx->AecMinGain = 1;
                break;
              case 2:
                pOS02k10Ctx->one_line_exp_time = 0.00006306;
                pOS02k10Ctx->FrameLengthLines = 0x4e2;
                pOS02k10Ctx->CurFrameLengthLines = pOS02k10Ctx->FrameLengthLines;
                pOS02k10Ctx->MaxIntegrationLine = pOS02k10Ctx->CurFrameLengthLines - 8;
                pOS02k10Ctx->MinIntegrationLine = 1;
                pOS02k10Ctx->AecMaxGain = 240;
                pOS02k10Ctx->AecMinGain = 1;
                break;
            default:
                return (RET_NOTAVAILABLE);
                break;
        }
        pOS02k10Ctx->MaxFps  = pOS02k10Ctx->SensorMode.fps;
        pOS02k10Ctx->MinFps  = 1 * ISI_FPS_QUANTIZE;
        pOS02k10Ctx->CurrFps = pOS02k10Ctx->MaxFps;
    }

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(OS02k10_DEBUG, "%s: OS02k10 System-Reset executed\n", __func__);
    osSleep(100);

    result = OS02k10_AecSetModeParameters(handle, pOS02k10Ctx);
    if (result != RET_SUCCESS) {
        TRACE(OS02k10_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pOS02k10Ctx->Configured = BOOL_TRUE;
    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT OS02k10_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;
    uint32_t correct_id = 0x5302;
    uint32_t sensor_id = 0;

    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);

    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    if (pOS02k10Ctx->KernelDriverFlag) {
        /*ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_RESERVE_ID,
              &correct_id);
        if (ret != 0) {
            TRACE(OS02k10_ERROR,
                  "%s: Read Sensor correct ID Error! \n", __func__);
            return (RET_FAILURE);
        }*/

        int ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_CHIP_ID, &sensor_id);
        if (ret != 0) {
            TRACE(OS02k10_ERROR, "%s: Read Sensor chip ID Error! \n", __func__);
            return (RET_FAILURE);
        }
    } else {

        result = OS02k10_IsiGetRevisionIss(handle, &sensor_id);
        if (result != RET_SUCCESS) {
            TRACE(OS02k10_ERROR, "%s: Read Sensor ID Error! \n", __func__);
            return (RET_FAILURE);
        }
    }

    if (correct_id != sensor_id) {
        TRACE(OS02k10_ERROR, "%s:ChipID =0x%x sensor_id=%x error! \n", __func__, correct_id, sensor_id);
        return (RET_FAILURE);
    }

    TRACE(OS02k10_INFO, "%s ChipID = 0x%08x, sensor_id = 0x%08x, success! \n", __func__, correct_id, sensor_id);
    TRACE(OS02k10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS02k10_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint32_t reg_val;
    uint32_t sensor_id;

    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);

    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    if (!pValue)
        return (RET_NULL_POINTER);

    if (pOS02k10Ctx->KernelDriverFlag) {
        int ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_CHIP_ID, &sensor_id);
        if (ret != 0) {
            TRACE(OS02k10_ERROR, "%s: Read Sensor ID Error! \n", __func__);
            return (RET_FAILURE);
        }
    } else {
        reg_val = 0;
        result = OS02k10_IsiReadRegIss(handle, 0x300a, &reg_val);
        sensor_id = (reg_val & 0xff) << 8;

        reg_val = 0;
        result |= OS02k10_IsiReadRegIss(handle, 0x300b, &reg_val);
        sensor_id |= (reg_val & 0xff);
    }

    *pValue = sensor_id;
    TRACE(OS02k10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS02k10_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on) 
{
    RESULT result = RET_SUCCESS;
    int ret = 0;
    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);

    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    if (pOS02k10Ctx->Configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    int32_t enable = (uint32_t) on;
    if (pOS02k10Ctx->KernelDriverFlag) {
        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_STREAM, &enable);

    } else {
        ret = OS02k10_IsiWriteRegIss(handle, 0x0100, on);
    }

    if (ret != 0) {
        return (RET_FAILURE);
    }

    pOS02k10Ctx->Streaming = on;

    TRACE(OS02k10_INFO, "%s (exit)\n", __func__);
    return (result);
}

RESULT OS02k10_IsiExposureControlIss(IsiSensorHandle_t handle,float NewGain,
                float NewIntegrationTime,uint8_t * pNumberOfFramesToSkip,
                float *pSetGain, float *pSetIntegrationTime, float *hdr_ratio)
{
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (pOS02k10Ctx == NULL) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if ((pNumberOfFramesToSkip == NULL) || (pSetGain == NULL) || (pSetIntegrationTime == NULL)) {
        TRACE(OS02k10_ERROR, "%s: Invalid parameter (NULL pointer detected)\n",__func__);
        return (RET_NULL_POINTER);
    }
    if(hdr_ratio[0] ==0 ||hdr_ratio[1] == 0) {
        hdr_ratio[0] = 16;
        hdr_ratio[1] = 16;
    }
    TRACE(OS02k10_DEBUG, "%s: g=%f, Ti=%f\n", __func__, NewGain,NewIntegrationTime);

    float long_gain   = 0, long_exp    = 0;
    float short_gain  = 0, short_exp   = 0;
    float vs_gain     = 0, vs_exp      = 0;
    if (pOS02k10Ctx->SensorMode.hdr_mode == SENSOR_MODE_HDR_STITCH) {
        float long_exposure_measure  = NewIntegrationTime * NewGain*hdr_ratio[0];
        float short_exposure_measure = NewIntegrationTime*NewGain;
        float vs_exp_measure         = short_exposure_measure / hdr_ratio[1]; 
        
        //2exposure
        if (pOS02k10Ctx->SensorMode.stitching_mode == SENSOR_STITCHING_L_AND_S ||
             pOS02k10Ctx->SensorMode.stitching_mode == SENSOR_STITCHING_2DOL ||
             pOS02k10Ctx->SensorMode.stitching_mode == SENSOR_STITCHING_DUAL_DCG_NOWAIT) {
          if ((short_exposure_measure + long_exposure_measure)  < (pOS02k10Ctx->FrameLengthLines -10)*pOS02k10Ctx->one_line_exp_time) {
                long_exp  = long_exposure_measure ;
                long_gain = 1.0;
                short_exp = short_exposure_measure;
                short_gain= 1.0;
          } else {
                long_exp  = (pOS02k10Ctx->FrameLengthLines -10)*pOS02k10Ctx->one_line_exp_time/(hdr_ratio[0]+1)*hdr_ratio[0]; 
                long_gain = long_exposure_measure / long_exp;

                short_exp = (pOS02k10Ctx->FrameLengthLines -10)*pOS02k10Ctx->one_line_exp_time/(hdr_ratio[0]+1);
                short_gain= short_exposure_measure / short_exp;
          }
        }
        //3exposure
        if (pOS02k10Ctx->SensorMode.stitching_mode == SENSOR_STITCHING_3DOL ||
           pOS02k10Ctx->SensorMode.stitching_mode == SENSOR_STITCHING_DUAL_DCG_NOWAIT ||
           pOS02k10Ctx->SensorMode.stitching_mode == SENSOR_STITCHING_LINEBYLINE) {
           if ((short_exposure_measure + long_exposure_measure + vs_exp_measure) < (pOS02k10Ctx->FrameLengthLines -12)*pOS02k10Ctx->one_line_exp_time){
                long_exp  = long_exposure_measure;
                long_gain = 1.0;
                short_exp = short_exposure_measure;
                short_gain= 1.0;
                vs_exp    = vs_exp_measure;
                vs_gain   = 1.0; 
          } else {
                long_exp  =  (((pOS02k10Ctx->FrameLengthLines -12)*pOS02k10Ctx->one_line_exp_time) / (hdr_ratio[0] +1+ 1/hdr_ratio[1]))*hdr_ratio[0];
                long_gain =  long_exposure_measure / long_exp;

                short_exp = (((pOS02k10Ctx->FrameLengthLines -12)*pOS02k10Ctx->one_line_exp_time) / (hdr_ratio[0] +1+ 1/hdr_ratio[1]));
                short_gain = short_exposure_measure / short_exp;

                vs_exp  =(((pOS02k10Ctx->FrameLengthLines -12)*pOS02k10Ctx->one_line_exp_time) / (hdr_ratio[0] +1+ 1/hdr_ratio[1])) / hdr_ratio[1];
                vs_gain    =  vs_exp_measure / vs_exp ;
          }
        }
    } else {
        short_exp  = NewIntegrationTime;
        short_gain = NewGain;
    }

    if (pOS02k10Ctx->SensorMode.hdr_mode != SENSOR_MODE_LINEAR) {
        TRACE(OS02k10_DEBUG, "%s: long_exp: %f long_gain:%f short_exp:%f short_gain:%f vs_exp:%f vs_gain:% f \n", __func__,
                                   long_exp,long_gain,short_exp,short_gain,vs_exp,vs_gain);
        result = OS02k10_IsiSetVSIntegrationTimeIss(handle,vs_exp,pSetIntegrationTime,pNumberOfFramesToSkip,hdr_ratio);
        result |= OS02k10_IsiSetVSGainIss(handle, vs_exp, vs_gain,pSetGain, hdr_ratio);

        result |= OS02k10_IsiSetLongGainIss(handle, long_gain);
        result |= OS02k10_IsiSetLongIntegrationTimeIss(handle,long_exp);

        result |= OS02k10_IsiSetIntegrationTimeIss(handle, short_exp,pSetIntegrationTime,pNumberOfFramesToSkip, hdr_ratio);
        result |= OS02k10_IsiSetGainIss(handle, short_gain, pSetGain, hdr_ratio);
    } else {
     
        result = OS02k10_IsiSetIntegrationTimeIss(handle, short_exp,pSetIntegrationTime,pNumberOfFramesToSkip, hdr_ratio);
        result |= OS02k10_IsiSetGainIss(handle, short_gain, pSetGain, hdr_ratio);
    }
    
    pOS02k10Ctx->CurHdrRatio = *hdr_ratio;
    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);

    return result;
}

static RESULT OS02k10_IsiGetGainLimitsIss (IsiSensorHandle_t handle, float *pMinGain, float *pMaxGain) 
{
    const OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (pOS02k10Ctx == NULL) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if ((pMinGain == NULL) || (pMaxGain == NULL)) {
        TRACE(OS02k10_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    *pMinGain = pOS02k10Ctx->AecMinGain;
    *pMaxGain = pOS02k10Ctx->AecMaxGain;

    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);
    return (result);
}

static RESULT OS02k10_IsiGetIntegrationTimeLimitsIss(IsiSensorHandle_t handle, float *pMinIntegrationTime, float *pMaxIntegrationTime) 
{
    const OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);
    if (pOS02k10Ctx == NULL) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if ((pMinIntegrationTime == NULL) || (pMaxIntegrationTime == NULL)) {
        TRACE(OS02k10_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    *pMinIntegrationTime = pOS02k10Ctx->AecMinIntegrationTime;
    *pMaxIntegrationTime = pOS02k10Ctx->AecMaxIntegrationTime;

    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT OS02k10_IsiGetCurrentExposureIss(IsiSensorHandle_t handle, float *pSetGain, float *pSetIntegrationTime, float *hdr_ratio)
{
    const OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (pOS02k10Ctx == NULL) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if ((pSetGain == NULL) || (pSetIntegrationTime == NULL))
        return (RET_NULL_POINTER);

    *pSetGain = pOS02k10Ctx->AecCurGain;
    *pSetIntegrationTime = pOS02k10Ctx->AecCurIntegrationTime;
    *hdr_ratio = pOS02k10Ctx->CurHdrRatio;

    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS02k10_IsiGetGainIss(IsiSensorHandle_t handle, float *pSetGain)
{
    const OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (pOS02k10Ctx == NULL) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSetGain == NULL) {
        return (RET_NULL_POINTER);
    }

    *pSetGain = pOS02k10Ctx->AecCurGain;

    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS02k10_IsiGetVSGainIss(IsiSensorHandle_t handle, float *pSetGain)
{
    const OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (pOS02k10Ctx == NULL) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSetGain == NULL) {
        return (RET_NULL_POINTER);
    }

    *pSetGain = pOS02k10Ctx->AecCurVSGain;

    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);

    return (result);
}

RESULT OS02k10_IsiGetLongGainIss(IsiSensorHandle_t handle, float *gain)
{
    const OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;

    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (pOS02k10Ctx == NULL) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (gain == NULL) {
        return (RET_NULL_POINTER);
    }

    *gain = pOS02k10Ctx->AecCurLongGain;

    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);

    return (RET_SUCCESS);
}

RESULT OS02k10_IsiGetGainIncrementIss(IsiSensorHandle_t handle, float *pIncr) 
{
    const OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (pOS02k10Ctx == NULL) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pIncr == NULL)
        return (RET_NULL_POINTER);

    *pIncr = pOS02k10Ctx->AecGainIncrement;

    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);

    return (result);
}

RESULT OS02k10_IsiSetGainIss(IsiSensorHandle_t handle,float NewGain, float *pSetGain, const float *hdr_ratio)
{
    RESULT result = RET_SUCCESS;
    float dGainLcg = 0.0f,aGainLcg = 0.0;
    uint32_t dgainLcg = 0, againLcg = 0;
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    const HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    if (NewGain < pOS02k10Ctx->AecMinGain) {
        NewGain = pOS02k10Ctx->AecMinGain;
    }
    if (NewGain > pOS02k10Ctx->AecMaxGain) {
        NewGain = pOS02k10Ctx->AecMaxGain;
    }

    TRACE(OS02k10_INFO, "%s: (Linear mode enter)\n", __func__);

    if ((pSetGain == NULL) || (hdr_ratio == NULL)) {
        return (RET_NULL_POINTER);
    }

    if (pOS02k10Ctx->SensorMode.hdr_mode != SENSOR_MODE_LINEAR) {
        if (NewGain <15.5) {
            dGainLcg = 1;
            aGainLcg = NewGain;
        } else {
            aGainLcg = 15.5;
            dGainLcg = NewGain /15.5;
        }
        if (dGainLcg < 1.0) dGainLcg = 1.0;
        againLcg = (uint32_t) aGainLcg * 16;
        dgainLcg = (uint32_t) dGainLcg * 1024;
        if ((dGainLcg != pOS02k10Ctx->OldGainLcg) || (aGainLcg != pOS02k10Ctx->OldAGainLcg)) {
            
            result = OS02k10_IsiWriteRegIss(handle, 0x3548,(againLcg >> 4) & 0xf);
            result |= OS02k10_IsiWriteRegIss(handle, 0x3549,(againLcg & 0xf)<<4);
            
        
            result |= OS02k10_IsiWriteRegIss(handle, 0x354a,(dgainLcg >>10) & 0xf);
            result |= OS02k10_IsiWriteRegIss(handle, 0x354b,(dgainLcg & 0x3ff) >>2);
            result |= OS02k10_IsiWriteRegIss(handle, 0x354c,(dgainLcg & 0x3) <<6);
            pOS02k10Ctx->OldAGainLcg = aGainLcg;
            pOS02k10Ctx->OldGainLcg = dGainLcg;
    }
    } else {

            if (NewGain < 15.5) {
                dGainLcg = 1;
                aGainLcg = NewGain;
            }
            againLcg = (uint32_t) aGainLcg * 16;
            dgainLcg = (uint32_t) dGainLcg * 1024;
        if ((dGainLcg != pOS02k10Ctx->OldGainLcg)|| (aGainLcg != pOS02k10Ctx->OldAGainLcg)) {
            result = OS02k10_IsiWriteRegIss(handle, 0x3508,(againLcg >> 4) & 0xf);
            result |= OS02k10_IsiWriteRegIss(handle, 0x3509,againLcg & 0xf);

            result |= OS02k10_IsiWriteRegIss(handle, 0x350a,(dgainLcg >>10) & 0xf);
            result |= OS02k10_IsiWriteRegIss(handle, 0x350b,(dgainLcg & 0x3ff) >>2);
            result |= OS02k10_IsiWriteRegIss(handle, 0x350c,(dgainLcg & 0x3ff) <<6);
        }

            pOS02k10Ctx->OldGainLcg = dGainLcg;
            pOS02k10Ctx->OldAGainLcg = aGainLcg;
    }
    pOS02k10Ctx->AecCurGain =(againLcg/16)*(dgainLcg/1024);

    *pSetGain = pOS02k10Ctx->AecCurGain;
    TRACE(OS02k10_DEBUG, "%s: g=%f\n", __func__, *pSetGain);
    return (result);
}

RESULT OS02k10_IsiSetVSGainIss(IsiSensorHandle_t handle,float NewIntegrationTime, float NewGain, float *pSetGain, const float *hdr_ratio)
{
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    const HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;
    RESULT result = RET_SUCCESS;

    float Gain = 0.0f,aGain= 0.0f;
    uint32_t ucgain = 0U;
    uint32_t again = 0U;
    if (NewGain < pOS02k10Ctx->AecMinGain) {
        NewGain = pOS02k10Ctx->AecMinGain;
    }
    if (NewGain > pOS02k10Ctx->AecMaxGain) {
        NewGain = pOS02k10Ctx->AecMaxGain;
    }

    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (!pOS02k10Ctx) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSetGain || !hdr_ratio)
        return (RET_NULL_POINTER);

        if (NewGain < 15.5) {
            aGain = NewGain;
            Gain = 1;
        } else {
            aGain = 15.5;
            Gain = NewGain /15.5;
        }
    if (aGain < 1.0) {
        aGain = 1.0;
    }
    again = (uint32_t) (aGain * 16);
    ucgain = (uint32_t) (Gain * 1024);
    TRACE(OS02k10_INFO, "%s: set Gain ucGainvs:=0x%03x NewIntegrationTime = %f,NewGain=%f\n", __func__, ucgain, NewIntegrationTime, NewGain);

    if (pOS02k10Ctx->OldVsGain != ucgain) {
        result = OS02k10_IsiWriteRegIss(handle, 0x3588,(again >> 4) & 0x0f);
        result |= OS02k10_IsiWriteRegIss(handle, 0x3589,(again & 0xff)<<4);
        result |= OS02k10_IsiWriteRegIss(handle, 0x358a, (ucgain>>10) & 0x0f);
        result |= OS02k10_IsiWriteRegIss(handle, 0x358b, (ucgain & 0x3ff)>>2);
        result |= OS02k10_IsiWriteRegIss(handle, 0x358c, (ucgain & 0x3)<<6);
        pOS02k10Ctx->OldVsGain = ucgain;
    }

    pOS02k10Ctx->AecCurVSGain = (again/16)*(ucgain/1024);
    *pSetGain = pOS02k10Ctx->AecCurGain;
    TRACE(OS02k10_DEBUG, "%s: g=%f\n", __func__, *pSetGain);
    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OS02k10_IsiSetLongGainIss(IsiSensorHandle_t handle, float gain)
{
    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    float aGain= 0,dGain = 0;
    uint32_t  again = 0,dgain= 0;
    if (gain < pOS02k10Ctx->AecMinGain) {
        gain = pOS02k10Ctx->AecMinGain;
    }
    if (gain > pOS02k10Ctx->AecMaxGain) {
        gain = pOS02k10Ctx->AecMaxGain;
    }

    if (!pOS02k10Ctx || !pOS02k10Ctx->IsiCtx.HalHandle) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    const HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;
    uint32_t SensorGain = 0;
        if (gain < 15.5) {
            dGain = 1;
            aGain = gain;
        } else {
            aGain = 15.5;
            dGain = gain / 15.5;
        }
    again= (uint32_t) aGain* 16;
    dgain= (uint32_t) dGain * 1024;
    if (pOS02k10Ctx->LastLongGain != SensorGain) {
        again= (uint32_t) aGain* 16;
        dgain= (uint32_t) dGain * 1024;

        int ret = OS02k10_IsiWriteRegIss(handle, 0x3508,(again >>4)& 0xf);
        ret |= OS02k10_IsiWriteRegIss(handle, 0x3509,(again & 0xf) << 4);
        ret |= OS02k10_IsiWriteRegIss(handle, 0x350a,(dgain >>10) & 0xf);
        ret |= OS02k10_IsiWriteRegIss(handle, 0x350b,(dgain & 0x3ff) >>2);
        ret |= OS02k10_IsiWriteRegIss(handle, 0x350c,(dgain & 0x3ff) <<6);
    }
    pOS02k10Ctx->LastLongGain = SensorGain;
    pOS02k10Ctx->AecCurLongGain = (again/16)*(dgain/1024);

    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return (RET_SUCCESS);
}

RESULT OS02k10_IsiGetIntegrationTimeIss(IsiSensorHandle_t handle, float *pSetIntegrationTime)
{
    const OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (!pOS02k10Ctx) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSetIntegrationTime)
        return (RET_NULL_POINTER);
    *pSetIntegrationTime = pOS02k10Ctx->AecCurIntegrationTime;
    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS02k10_IsiGetVSIntegrationTimeIss(IsiSensorHandle_t handle, float *pSetIntegrationTime)
{
    const OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (!pOS02k10Ctx) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    if (!pSetIntegrationTime)
        return (RET_NULL_POINTER);

    *pSetIntegrationTime = pOS02k10Ctx->AecCurVSIntegrationTime;
    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS02k10_IsiGetLongIntegrationTimeIss(IsiSensorHandle_t handle, float *pIntegrationTime)
{
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (!pOS02k10Ctx) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    if (!pIntegrationTime)
        return (RET_NULL_POINTER);

    pOS02k10Ctx->AecCurLongIntegrationTime =  pOS02k10Ctx->AecCurIntegrationTime;

    *pIntegrationTime = pOS02k10Ctx->AecCurLongIntegrationTime;
    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return (RET_SUCCESS);
}

RESULT OS02k10_IsiGetIntegrationTimeIncrementIss(IsiSensorHandle_t handle, float *pIncr)
{
    const OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (!pOS02k10Ctx) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pIncr)
        return (RET_NULL_POINTER);

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pOS02k10Ctx->AecIntegrationTimeIncrement;
    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS02k10_IsiSetIntegrationTimeIss(IsiSensorHandle_t handle, float NewIntegrationTime,float *pSetIntegrationTime,
                                        uint8_t * pNumberOfFramesToSkip, float *hdr_ratio)
{
    RESULT result = RET_SUCCESS;

    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (!pOS02k10Ctx) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    uint32_t exp_line = 0;
    uint32_t exp_line_old = 0;

    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (!pSetIntegrationTime || !pNumberOfFramesToSkip) {
        TRACE(OS02k10_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __func__);
        return (RET_NULL_POINTER);
    }

    exp_line = NewIntegrationTime / pOS02k10Ctx->one_line_exp_time;
    exp_line_old = exp_line;
    exp_line = MIN(pOS02k10Ctx->MaxIntegrationLine,MAX(pOS02k10Ctx->MinIntegrationLine, exp_line));

    TRACE(OS02k10_DEBUG, "%s: set AEC_PK_EXPO=0x%05x\n", __func__, exp_line);

    if (exp_line != pOS02k10Ctx->OldIntegrationTime) {
        if (pOS02k10Ctx->KernelDriverFlag) {
            ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_EXP, &exp_line);
        } else {
            if (pOS02k10Ctx->SensorMode.hdr_mode != SENSOR_MODE_LINEAR) {
                result = OS02k10_IsiWriteRegIss(handle, 0x3541,(exp_line >> 8) & 0xff);
                result |= OS02k10_IsiWriteRegIss(handle, 0x3542,exp_line & 0xff);
            } else {
                result = OS02k10_IsiWriteRegIss(handle, 0x3501,(exp_line >> 8) & 0xff);
                result |= OS02k10_IsiWriteRegIss(handle, 0x3502,exp_line & 0xff);
            }
        }

        pOS02k10Ctx->OldIntegrationTime = exp_line;    // remember current integration time
        pOS02k10Ctx->AecCurIntegrationTime = exp_line * pOS02k10Ctx->one_line_exp_time;

        *pNumberOfFramesToSkip = 2U;    //skip 1 frame
    } else {
        *pNumberOfFramesToSkip = 0U;    //no frame skip
    }

    if (exp_line_old != exp_line) {
        *pSetIntegrationTime = pOS02k10Ctx->AecCurIntegrationTime;
    } else {
        *pSetIntegrationTime = NewIntegrationTime;
    }

    TRACE(OS02k10_DEBUG, "%s: Ti=%f\n", __func__, *pSetIntegrationTime);
    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS02k10_IsiSetVSIntegrationTimeIss(IsiSensorHandle_t handle, float NewIntegrationTime,float *pSetVSIntegrationTime,
                                            uint8_t * pNumberOfFramesToSkip, float *hdr_ratio)
{
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (!pOS02k10Ctx) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;
    RESULT result = RET_SUCCESS;
    uint32_t exp_line = 0;

    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (!pSetVSIntegrationTime || !pNumberOfFramesToSkip) {
        TRACE(OS02k10_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __func__);
        return (RET_NULL_POINTER);
    }

    TRACE(OS02k10_INFO, "%s:  maxIntegrationTime-=%f minIntegrationTime = %f\n",
         __func__,pOS02k10Ctx->AecMaxIntegrationTime, pOS02k10Ctx->AecMinIntegrationTime);


    exp_line = NewIntegrationTime / pOS02k10Ctx->one_line_exp_time;
    exp_line = MIN(pOS02k10Ctx->MaxIntegrationLine,MAX(pOS02k10Ctx->MinIntegrationLine, exp_line));
    if (exp_line <1) exp_line = 1;
    if (exp_line != pOS02k10Ctx->OldVsIntegrationTime) {
        if (pOS02k10Ctx->KernelDriverFlag) {
            ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_SEXP, &exp_line);
        } else {
           // result = OS02k10_IsiWriteRegIss(handle, 0x3208, 0x00);

            result = OS02k10_IsiWriteRegIss(handle, 0x3581,(exp_line >> 8) & 0xff);
            result |= OS02k10_IsiWriteRegIss(handle, 0x3582, exp_line & 0xff);

           // result = OS02k10_IsiWriteRegIss(handle, 0x3208, 0x10);
        }

        pOS02k10Ctx->OldVsIntegrationTime = exp_line;
        pOS02k10Ctx->AecCurVSIntegrationTime = exp_line * pOS02k10Ctx->one_line_exp_time;    //remember current integration time
        *pNumberOfFramesToSkip = 2U;    //skip 1 frame
    } else {
        *pNumberOfFramesToSkip = 0U;    //no frame skip
    }

    *pSetVSIntegrationTime = pOS02k10Ctx->AecCurVSIntegrationTime;

    TRACE(OS02k10_DEBUG, "%s: NewIntegrationTime=%f\n", __func__,NewIntegrationTime);
    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OS02k10_IsiSetLongIntegrationTimeIss(IsiSensorHandle_t handle, float IntegrationTime)
{
    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (!handle || !pOS02k10Ctx->IsiCtx.HalHandle) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    const HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    uint32_t exp_line = 0;
    exp_line = IntegrationTime / pOS02k10Ctx->one_line_exp_time;
    exp_line = MIN(pOS02k10Ctx->MaxIntegrationLine, MAX(pOS02k10Ctx->MinIntegrationLine, exp_line));

    if (exp_line != pOS02k10Ctx->LastLongExpLine) {
        int ret = OS02k10_IsiWriteRegIss(handle, 0x3501,(exp_line >> 8) & 0xff);
        ret |= OS02k10_IsiWriteRegIss(handle, 0x3502,exp_line & 0xff);

        pOS02k10Ctx->LastLongExpLine = exp_line;
        pOS02k10Ctx->AecCurLongIntegrationTime =  exp_line*pOS02k10Ctx->one_line_exp_time;
    }


    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return (RET_SUCCESS);
}

RESULT OS02k10_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t *pFps)
{
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (pOS02k10Ctx == NULL) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    if (pOS02k10Ctx->KernelDriverFlag) {
        ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_FPS, pFps);
        pOS02k10Ctx->CurrFps = *pFps;
    }

    *pFps = pOS02k10Ctx->CurrFps;

    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS02k10_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t Fps)
{
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    if (pOS02k10Ctx == NULL) {
        TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    if (Fps > pOS02k10Ctx->MaxFps) {
        TRACE(OS02k10_ERROR, "%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, Fps, pOS02k10Ctx->MaxFps, pOS02k10Ctx->MinFps, pOS02k10Ctx->MaxFps);
        Fps = pOS02k10Ctx->MaxFps;
    }
    if (Fps < pOS02k10Ctx->MinFps) {
        TRACE(OS02k10_ERROR, "%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, Fps, pOS02k10Ctx->MinFps, pOS02k10Ctx->MinFps, pOS02k10Ctx->MaxFps);
        Fps = pOS02k10Ctx->MinFps;
    }
    if (pOS02k10Ctx->KernelDriverFlag) {
        int ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_FPS, &Fps);
        if (ret != 0) {
            TRACE(OS02k10_ERROR, "%s: set sensor fps=%d error\n", __func__);
            return (RET_FAILURE);
        }

        ret |= ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_SENSOR_MODE, &(pOS02k10Ctx->SensorMode));
        pOS02k10Ctx->MaxIntegrationLine = pOS02k10Ctx->SensorMode.ae_info.max_integration_time;
        pOS02k10Ctx->AecMaxIntegrationTime = pOS02k10Ctx->MaxIntegrationLine * pOS02k10Ctx->one_line_exp_time;
    } else {
        uint16_t FrameLengthLines;
        FrameLengthLines = pOS02k10Ctx->FrameLengthLines * pOS02k10Ctx->MaxFps / Fps;
        result = OS02k10_IsiWriteRegIss(handle, 0x30b2, (FrameLengthLines >> 8) & 0xff);
        result |= OS02k10_IsiWriteRegIss(handle, 0x30b3, FrameLengthLines & 0xff);
        if (result != RET_SUCCESS) {
            TRACE(OS02k10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
            return (RET_FAILURE);
        }
        pOS02k10Ctx->CurrFps = Fps;
        pOS02k10Ctx->CurFrameLengthLines = FrameLengthLines;
        pOS02k10Ctx->MaxIntegrationLine  = pOS02k10Ctx->CurFrameLengthLines - 3;
        pOS02k10Ctx->AecMaxIntegrationTime = pOS02k10Ctx->MaxIntegrationLine * pOS02k10Ctx->one_line_exp_time;
    }

    TRACE(OS02k10_INFO, "%s: set sensor fps = %d\n", __func__, pOS02k10Ctx->CurrFps);

    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS02k10_IsiGetAutoFpsInfoIss(IsiSensorHandle_t handle, IsiAutoFps_t *pAutoFpsInfo)
{
    RESULT result = RET_SUCCESS;
    return (result);
}

RESULT OS02k10_IsiGetStartEvIss(IsiSensorHandle_t handle, uint64_t *pStartEv)
{
    RESULT result = RET_SUCCESS;
    return (result);
}

RESULT OS02k10_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    if (pOS02k10Ctx->SensorMode.hdr_mode == SENSOR_MODE_HDR_NATIVE) {
        pIspStatus->useSensorAWB = true;
    } else {
        pIspStatus->useSensorAWB = false;
    }

    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT OS02k10_IsiSetTpgIss(IsiSensorHandle_t handle, IsiTpg_t Tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS02k10_INFO, "%s: (enter)\n", __func__);

    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOS02k10Ctx->Configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (Tpg.enable == 0) {
        result = OS02k10_IsiWriteRegIss(handle, 0x3253, 0x00);
    } else {
        result = OS02k10_IsiWriteRegIss(handle, 0x3253, 0x80);
    }

    pOS02k10Ctx->TestPattern = Tpg.enable;

    TRACE(OS02k10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS02k10_IsiSetBlcIss(IsiSensorHandle_t handle, IsiBlc_t *pBlc)
{
    int32_t ret = 0;
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)
        return RET_NULL_POINTER;

    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_BLC, pBlc);
    if (ret != 0) {
        TRACE(OS02k10_ERROR, "%s: set wb error\n", __func__);
    }

    return RET_SUCCESS;
}

RESULT OS02k10_IsiSetWBIss(IsiSensorHandle_t handle, IsiWB_t *pWb)
{
    int32_t ret = 0;
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_WRONG_HANDLE;
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    if (pWb == NULL)
        return RET_NULL_POINTER;

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_WB, pWb);
    if (ret != 0) {
        TRACE(OS02k10_ERROR, "%s: set wb error\n", __func__);
    }

    return RET_SUCCESS;
}

RESULT OS02k10_IsiGetExpandCurveIss(IsiSensorHandle_t handle, IsiCompandCurve_t *pCurve)
{
    int32_t ret = 0;
    OS02k10_Context_t *pOS02k10Ctx = (OS02k10_Context_t *) handle;
    if (pOS02k10Ctx == NULL || pOS02k10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOS02k10Ctx->IsiCtx.HalHandle;

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_EXPAND_CURVE, pCurve);
    if (ret != 0) {
        TRACE(OS02k10_ERROR, "%s: get  expand cure error\n", __func__);
        return RET_FAILURE;
    }

    return RET_SUCCESS;
}

RESULT OS02k10_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "OS02k10";
    TRACE(OS02k10_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = OS02k10_IsiCreateIss;
        pIsiSensor->pIsiReleaseIss                      = OS02k10_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = OS02k10_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = OS02k10_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = OS02k10_IsiGetModeIss;
        pIsiSensor->pIsiSetModeIss                      = OS02k10_IsiSetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = OS02k10_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = OS02k10_IsiGetCapsIss;
        pIsiSensor->pIsiSetupIss                        = OS02k10_IsiSetupIss;
        pIsiSensor->pIsiCheckConnectionIss              = OS02k10_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = OS02k10_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = OS02k10_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiExposureControlIss              = OS02k10_IsiExposureControlIss;
        pIsiSensor->pIsiGetGainLimitsIss                = OS02k10_IsiGetGainLimitsIss;
        pIsiSensor->pIsiGetIntegrationTimeLimitsIss     = OS02k10_IsiGetIntegrationTimeLimitsIss;
        // pIsiSensor->pIsiGetCurrentExposureIss        = OS02k10_IsiGetCurrentExposureIss;
        pIsiSensor->pIsiGetGainIss                      = OS02k10_IsiGetGainIss;
        pIsiSensor->pIsiGetVSGainIss                    = OS02k10_IsiGetVSGainIss;
        pIsiSensor->pIsiGetLongGainIss                  = OS02k10_IsiGetLongGainIss;
        pIsiSensor->pIsiGetGainIncrementIss             = OS02k10_IsiGetGainIncrementIss;
        pIsiSensor->pIsiSetGainIss                      = OS02k10_IsiSetGainIss;
        pIsiSensor->pIsiGetIntegrationTimeIss           = OS02k10_IsiGetIntegrationTimeIss;
        pIsiSensor->pIsiGetVSIntegrationTimeIss         = OS02k10_IsiGetVSIntegrationTimeIss;
        pIsiSensor->pIsiGetLongIntegrationTimeIss       = OS02k10_IsiGetLongIntegrationTimeIss;
        pIsiSensor->pIsiGetIntegrationTimeIncrementIss  = OS02k10_IsiGetIntegrationTimeIncrementIss;
        pIsiSensor->pIsiSetIntegrationTimeIss           = OS02k10_IsiSetIntegrationTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = OS02k10_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = OS02k10_IsiSetFpsIss;
        pIsiSensor->pIsiGetAutoFpsInfoIss               = OS02k10_IsiGetAutoFpsInfoIss;
        pIsiSensor->pIsiGetStartEvIss                   = OS02k10_IsiGetStartEvIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = OS02k10_IsiGetIspStatusIss;
        pIsiSensor->pIsiSetBlcIss                       = OS02k10_IsiSetBlcIss;
        pIsiSensor->pIsiSetWBIss                        = OS02k10_IsiSetWBIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetPowerIss                     = OS02k10_IsiSetPowerIss;
        pIsiSensor->pIsiSetTpgIss                       = OS02k10_IsiSetTpgIss;
        pIsiSensor->pIsiGetExpandCurveIss               = OS02k10_IsiGetExpandCurveIss;

        /* AF */
        pIsiSensor->pIsiFocusCreateIss                  = NULL;
        pIsiSensor->pIsiFocusReleaseIss                 = NULL;
        pIsiSensor->pIsiFocusGetCalibrateIss            = NULL;
        pIsiSensor->pIsiFocusSetIss                     = NULL;
        pIsiSensor->pIsiFocusGetIss                     = NULL;

    } else {
        result = RET_NULL_POINTER;
    }

    TRACE(OS02k10_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t OS02k10_IsiCamDrvConfig = {
    .cameraDriverID      = 0x5302,
    .pIsiGetSensorIss    = OS02k10_IsiGetSensorIss,
};
