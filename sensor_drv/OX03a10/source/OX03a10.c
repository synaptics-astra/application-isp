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

#include "OX03a10_priv.h"

CREATE_TRACER(OX03a10_INFO , "OX03a10: ", INFO,    1);
CREATE_TRACER(OX03a10_WARN , "OX03a10: ", WARNING, 1);
CREATE_TRACER(OX03a10_ERROR, "OX03a10: ", ERROR,   1);
CREATE_TRACER(OX03a10_DEBUG,     "OX03a10: ", INFO, 1);
CREATE_TRACER(OX03a10_REG_INFO , "OX03a10: ", INFO, 1);
CREATE_TRACER(OX03a10_REG_DEBUG, "OX03a10: ", INFO, 1);

#ifdef SUBDEV_V4L2
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#undef TRACE
#define TRACE(x, ...)
#endif

#define OX03a10_MIN_GAIN_STEP    (1.0f/16.0f)  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain) */
#define OX03a10_MAX_GAIN_AEC     (32.0f)       /**< max. gain used by the AEC (arbitrarily chosen, hardware limit = 62.0, driver limit = 32.0) */
#define OX03a10_VS_MAX_INTEGRATION_TIME (0.0018)

/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static struct vvsensor_mode_s pox03a10_mode_info[] = {
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
        .stitching_mode = SENSOR_STITCHING_16BIT_COMPRESS,
        .bit_width = 12,
        .bayer_pattern = BAYER_BGGR,
    },
};

RESULT OX03a10_IsiSetPowerIss(IsiSensorHandle_t handle, bool_t on)
{

    RESULT result = RET_SUCCESS;

    int ret = 0;
    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);

    int32_t enable = on;
    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_POWER, &enable);
    if (ret != 0) {
        TRACE(OX03a10_ERROR, "%s: sensor set power error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OX03a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

#ifdef SUBDEV_CHAR
RESULT OX03a10_IsiSetClkIss(IsiSensorHandle_t handle, uint32_t Clk)
{

    RESULT result = RET_SUCCESS;
    int32_t ret = 0;

    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_CLK, &Clk);

    if (ret != 0) {
        TRACE(OX03a10_ERROR, "%s: sensor set clk error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OX03a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiGetClkIss(IsiSensorHandle_t handle, uint32_t *pClk)
{

    RESULT result = RET_SUCCESS;
    int ret = 0;

    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_CLK, pClk);
    if (ret != 0) {
        TRACE(OX03a10_ERROR, "%s: sensor get clk error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OX03a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiConfigSCCBIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;
    int ret = 0;
    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);

    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    static IsiBus_t SensorSccbInfo;
    SensorSccbInfo.i2c.slaveAddr = (0x6c >> 1);
    SensorSccbInfo.i2c.addrWidth = 2;
    SensorSccbInfo.i2c.dataWidth = 1;

    struct vvsensor_sccb_cfg_s sensor_sccb_config;
    sensor_sccb_config.slave_addr = SensorSccbInfo.i2c.slaveAddr;
    sensor_sccb_config.addr_byte  = SensorSccbInfo.i2c.addrWidth;
    sensor_sccb_config.data_byte  = SensorSccbInfo.i2c.dataWidth;

    ret =ioctl(pHalCtx->sensor_fd, VVSENSORIOC_SENSOR_SCCB_CFG,&sensor_sccb_config);
    if (ret != 0) {
        TRACE(OX03a10_ERROR, "%s: sensor config sccb info error!\n",__func__);
        return (RET_FAILURE);
    }

    TRACE(OX03a10_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}
#endif

RESULT OX03a10_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig)
{

    RESULT result = RET_SUCCESS;
    OX03a10_Context_t *pOX03a10Ctx;

    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);

    if (!pConfig || !pConfig->pSensor) {
        return (RET_NULL_POINTER);
    }

    pOX03a10Ctx = (OX03a10_Context_t *) malloc(sizeof(OX03a10_Context_t));
    if (!pOX03a10Ctx) {
        TRACE(OX03a10_ERROR, "%s: Can't allocate ox03a10 context\n",__func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pOX03a10Ctx, 0, sizeof(OX03a10_Context_t));

    result = HalAddRef(pConfig->halHandle);
    if (result != RET_SUCCESS) {
        free(pOX03a10Ctx);
        return (result);
    }

    pOX03a10Ctx->IsiCtx.HalHandle = pConfig->halHandle;
    pOX03a10Ctx->IsiCtx.pSensor = pConfig->pSensor;
    pOX03a10Ctx->GroupHold = BOOL_FALSE;
    pOX03a10Ctx->OldGain = 0;
    pOX03a10Ctx->OldIntegrationTime = 0;
    pOX03a10Ctx->Configured = BOOL_FALSE;
    pOX03a10Ctx->Streaming = BOOL_FALSE;
    pOX03a10Ctx->TestPattern = BOOL_FALSE;
    pOX03a10Ctx->isAfpsRun = BOOL_FALSE;
    pOX03a10Ctx->SensorMode.index = pConfig->sensorModeIndex;
    pConfig->hSensor = (IsiSensorHandle_t) pOX03a10Ctx;
#ifdef SUBDEV_CHAR
    struct vvsensor_mode_s *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pox03a10_mode_info)/ sizeof(struct vvsensor_mode_s); i++) {
        if (pox03a10_mode_info[i].index == pOX03a10Ctx->SensorMode.index) {
            SensorDefaultMode = &(pox03a10_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
        case 0:
            memcpy(pOX03a10Ctx->SensorRegCfgFile,"OX03a10_mipi4lane_1080p_init.txt", strlen("OX03a10_mipi4lane_1080p_init.txt"));
            break;
        case 1:
            memcpy(pOX03a10Ctx->SensorRegCfgFile,"OX03a10_mipi4lane_1080p_16DCG_12VS_init.txt", strlen("OX03a10_mipi4lane_1080p_16DCG_12VS_init.txt"));
            break;
        default:
            break;
        }

        if (access(pOX03a10Ctx->SensorRegCfgFile, F_OK) == 0) {
            pOX03a10Ctx->KernelDriverFlag = 0;
            memcpy(&(pOX03a10Ctx->SensorMode),SensorDefaultMode,sizeof(struct vvsensor_mode_s));
        } else {
            pOX03a10Ctx->KernelDriverFlag = 1;
        }
    } else {
        pOX03a10Ctx->KernelDriverFlag = 1;
    }

    result = OX03a10_IsiSetPowerIss(pOX03a10Ctx, BOOL_TRUE);
    RETURN_RESULT_IF_DIFFERENT(RET_SUCCESS, result);

    uint32_t SensorClkIn = 0;
    if (pOX03a10Ctx->KernelDriverFlag) {
        result = OX03a10_IsiGetClkIss(pOX03a10Ctx, &SensorClkIn);
        RETURN_RESULT_IF_DIFFERENT(RET_SUCCESS, result);
    }

    result = OX03a10_IsiSetClkIss(pOX03a10Ctx, SensorClkIn);
    RETURN_RESULT_IF_DIFFERENT(RET_SUCCESS, result);

    if (!pOX03a10Ctx->KernelDriverFlag) {
        result = OX03a10_IsiConfigSCCBIss(pOX03a10Ctx);
        RETURN_RESULT_IF_DIFFERENT(RET_SUCCESS, result);
    }

    pOX03a10Ctx->pattern = ISI_BPAT_GBRG;
#endif

#ifdef SUBDEV_V4L2
    pOX03a10Ctx->pattern = ISI_BPAT_BGGR;
    pOX03a10Ctx->subdev = HalGetFdHandle(pConfig->halHandle, HAL_MODULE_SENSOR);
    pOX03a10Ctx->KernelDriverFlag = 1;
#endif
    TRACE(OX03a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiReleaseIss(IsiSensorHandle_t handle) {
    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);

    if (pOX03a10Ctx == NULL)
        return (RET_WRONG_HANDLE);

    (void)OX03a10_IsiSetStreamingIss(pOX03a10Ctx, BOOL_FALSE);
    (void)OX03a10_IsiSetPowerIss(pOX03a10Ctx, BOOL_FALSE);
    (void)HalDelRef(pOX03a10Ctx->IsiCtx.HalHandle);

    MEMSET(pOX03a10Ctx, 0, sizeof(OX03a10_Context_t));
    free(pOX03a10Ctx);
    TRACE(OX03a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiReadRegIss(IsiSensorHandle_t handle, const uint32_t Addr, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    int32_t ret = 0;
    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);

    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    struct vvsensor_sccb_data_s sccb_data;
    sccb_data.addr = Addr;
    sccb_data.data = 0;
    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_READ_REG, &sccb_data);
    if (ret != 0) {
        TRACE(OX03a10_ERROR, "%s: read sensor register error!\n",__func__);
        return (RET_FAILURE);
    }

    *pValue = sccb_data.data;

    TRACE(OX03a10_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

RESULT OX03a10_IsiWriteRegIss(IsiSensorHandle_t handle, const uint32_t Addr, const uint32_t Value)
{
    RESULT result = RET_SUCCESS;
    int ret = 0;
    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);

    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    struct vvsensor_sccb_data_s sccb_data;
    sccb_data.addr = Addr;
    sccb_data.data = Value;

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_WRITE_REG, &sccb_data);
    if (ret != 0) {
        TRACE(OX03a10_ERROR, "%s: write sensor register error!\n",__func__);
        return (RET_FAILURE);
    }

    TRACE(OX03a10_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OX03a10_IsiGetModeIss(IsiSensorHandle_t handle, IsiMode_t *pMode)
{
    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);
    const OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pOX03a10Ctx->SensorMode), sizeof(pOX03a10Ctx->SensorMode));

    TRACE(OX03a10_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static RESULT OX03a10_IsiSetModeIss(IsiSensorHandle_t handle, IsiMode_t *pMode)
{
    int ret = 0;
    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);

    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_SENSOR_MODE, pMode);
    if (ret != 0) {
        TRACE(OX03a10_ERROR, "%s: write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OX03a10_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT OX03a10_IsiEnumModeIss(IsiSensorHandle_t handle, IsiEnumMode_t *pEnumMode)
{
    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pox03a10_mode_info)/sizeof(pox03a10_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pox03a10_mode_info)/sizeof(pox03a10_mode_info[0])); i++) {
        if (pox03a10_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pox03a10_mode_info[i],sizeof(IsiMode_t));
            TRACE(OX03a10_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

RESULT OX03a10_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);

    if (pOX03a10Ctx == NULL)
        return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pOX03a10Ctx->SensorMode.bit_width;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pOX03a10Ctx->SensorMode.bayer_pattern;
    pCaps->resolution.width  = pOX03a10Ctx->SensorMode.size.width;
    pCaps->resolution.height = pOX03a10Ctx->SensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(OX03a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OX03a10_IsiGetRegCfgIss(const char *registerFileName, struct vvsensor_sccb_array_s *array)
{
    if (NULL == registerFileName) {
        TRACE(OX03a10_ERROR, "%s:registerFileName is NULL\n", __func__);
        return (RET_NULL_POINTER);
    }
#ifdef SUBDEV_CHAR
    FILE *fp = NULL;
    fp = fopen(registerFileName, "rb");
    if (!fp) {
        TRACE(OX03a10_ERROR, "%s:load register file  %s error!\n", __func__, registerFileName);
        return (RET_FAILURE);
    }

    if (errno) {
        TRACE(OX03a10_WARN, "%s:Clearing unhandled errno before calling fgets: %d\n", __func__, errno);
        errno = 0;
    }

    char LineBuf[512];
    uint32_t FileTotalLine = 0;
    while (!feof(fp)) {
        if (NULL == fgets(LineBuf, 512, fp)) {
                    if (errno) {
                        TRACE(OX03a10_ERROR, "%s:gets linebuf error: %d\n", __func__, errno);
                        return (RET_FAILURE);
                    }
                }
        FileTotalLine++;
    }
    array->sccb_data = malloc(FileTotalLine * sizeof(struct vvsensor_sccb_data_s));
    if (array->sccb_data == NULL) {
        TRACE(OX03a10_ERROR, "%s:malloc failed NULL Point!\n", __func__,registerFileName);
        return (RET_FAILURE);
    }
    rewind(fp);

    array->count = 0;
    while (!feof(fp)) {
        memset(LineBuf, 0, sizeof(LineBuf));
        if (NULL == fgets(LineBuf, 512, fp)) {
                    if (errno) {
                        TRACE(OX03a10_ERROR, "%s:gets linebuf error: %d\n", __func__, errno);
                        return (RET_FAILURE);
                    }
                }
        int result = sscanf(LineBuf, "0x%x 0x%x",&(array->sccb_data[array->count].addr), &(array->sccb_data[array->count].data));
        if (result != 2)
        continue;
        array->count++;
    }
#endif

    return 0;
}

RESULT OX03a10_AecSetModeParameters(IsiSensorHandle_t handle, OX03a10_Context_t *pOX03a10Ctx) 
{
    RESULT result = RET_SUCCESS;
    TRACE(OX03a10_INFO, "%s%s: (enter)\n", __func__, pOX03a10Ctx->isAfpsRun ? "(AFPS)" : "");

    pOX03a10Ctx->AecIntegrationTimeIncrement = pOX03a10Ctx->one_line_exp_time;
    pOX03a10Ctx->AecMinIntegrationTime = pOX03a10Ctx->one_line_exp_time * pOX03a10Ctx->MinIntegrationLine;
    pOX03a10Ctx->AecMaxIntegrationTime = pOX03a10Ctx->one_line_exp_time * pOX03a10Ctx->MaxIntegrationLine;

    TRACE(OX03a10_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pOX03a10Ctx->isAfpsRun ? "(AFPS)" : "", pOX03a10Ctx->AecMaxIntegrationTime);

    pOX03a10Ctx->AecGainIncrement = OX03a10_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pOX03a10Ctx->AecCurGain = pOX03a10Ctx->AecMinGain;
    pOX03a10Ctx->AecCurIntegrationTime = 0.0f;
    pOX03a10Ctx->OldGain = 0;
    pOX03a10Ctx->OldIntegrationTime = 0;

    TRACE(OX03a10_INFO, "%s%s: (exit)\n", __func__, pOX03a10Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

RESULT OX03a10_IsiSetupIss(IsiSensorHandle_t handle)
{
    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    if (!pOX03a10Ctx) {
        TRACE(OX03a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;
    int ret = 0;

    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);

    if (pOX03a10Ctx->Streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    if (pOX03a10Ctx->KernelDriverFlag) {
#ifdef SUBDEV_CHAR

        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_INIT, &(pOX03a10Ctx->SensorMode));
        if (ret != 0) {
            TRACE(OX03a10_ERROR, "%s:sensor init error!\n", __func__);
            return (RET_FAILURE);
        }

        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_SENSOR_MODE, &(pOX03a10Ctx->SensorMode));
        if (ret != 0) {
            TRACE(OX03a10_ERROR, "%s:sensor get mode info error!\n", __func__);
            return (RET_FAILURE);
        }

        /*struct vvcam_ae_info_s ae_info;
        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_AE_INFO, &ae_info);
        if (ret != 0) {
            TRACE(OX03a10_ERROR, "%s:sensor get ae info error!\n",
                  __func__);
            return (RET_FAILURE);
        }
        pOX03a10Ctx->one_line_exp_time =
            (float)ae_info.one_line_exp_time_ns / 1000000000;
        pOX03a10Ctx->MaxIntegrationLine = ae_info.max_integration_time;
        pOX03a10Ctx->MinIntegrationLine = ae_info.min_integration_time;
        pOX03a10Ctx->gain_accuracy = ae_info.gain_accuracy;
        pOX03a10Ctx->AecMinGain = (float)(ae_info.min_gain) / ae_info.gain_accuracy;
        pOX03a10Ctx->AecMaxGain = (float)(ae_info.max_gain) / ae_info.gain_accuracy;

        pOX03a10Ctx->MaxFps  = pOX03a10Ctx->SensorMode.fps;
        pOX03a10Ctx->MinFps  = 1;
        pOX03a10Ctx->CurrFps = pOX03a10Ctx->MaxFps;*/
#endif

#ifdef SUBDEV_V4L2
        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_SENSOR_MODE, &(pOX03a10Ctx->SensorMode));
        if (ret != 0) {
            TRACE(OX03a10_ERROR, "%s:sensor get mode info error!\n", __func__);
            return (RET_FAILURE);
        }

        pOX03a10Ctx->one_line_exp_time  = (float) (pOX03a10Ctx->SensorMode.ae_info.one_line_exp_time_ns) / 1000000000;
        pOX03a10Ctx->MaxIntegrationLine = pOX03a10Ctx->SensorMode.ae_info.max_integration_time;
        pOX03a10Ctx->MinIntegrationLine = pOX03a10Ctx->SensorMode.ae_info.min_integration_time;
        pOX03a10Ctx->gain_accuracy      = pOX03a10Ctx->SensorMode.ae_info.gain_accuracy;
        pOX03a10Ctx->AecMaxGain         = (float)(pOX03a10Ctx->SensorMode.ae_info.max_gain) /pOX03a10Ctx->gain_accuracy ;
        pOX03a10Ctx->AecMinGain         = (float)(pOX03a10Ctx->SensorMode.ae_info.min_gain) / pOX03a10Ctx->gain_accuracy ;
        pOX03a10Ctx->MaxFps             = pOX03a10Ctx->SensorMode.fps;
        pOX03a10Ctx->CurrFps            = pOX03a10Ctx->MaxFps;

#endif

    } else {
        struct vvsensor_sccb_array_s array;
        result = OX03a10_IsiGetRegCfgIss(pOX03a10Ctx->SensorRegCfgFile, &array);
        if (result != 0) {
            TRACE(OX03a10_ERROR, "%s:OX03a10_IsiGetRegCfgIss error!\n", __func__);
            return (RET_FAILURE);
        }

        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_WRITE_ARRAY, &array);
        if (ret != 0) {
            TRACE(OX03a10_ERROR, "%s:Sensor Write Reg array error!\n", __func__);
            return (RET_FAILURE);
        }

        switch(pOX03a10Ctx->SensorMode.index)
        {
            case 0:
                pOX03a10Ctx->one_line_exp_time = 0.000019;
                pOX03a10Ctx->FrameLengthLines = 0x532;
                pOX03a10Ctx->CurFrameLengthLines = pOX03a10Ctx->FrameLengthLines;
                pOX03a10Ctx->MaxIntegrationLine = pOX03a10Ctx->CurFrameLengthLines - 10;
                pOX03a10Ctx->MinIntegrationLine = 1;
                pOX03a10Ctx->AecMaxGain = 64;
                pOX03a10Ctx->AecMinGain = 1.0;
                break;
            case 1:
                pOX03a10Ctx->one_line_exp_time = 0.000019;
                pOX03a10Ctx->FrameLengthLines = 0x5a0;
                pOX03a10Ctx->CurFrameLengthLines = pOX03a10Ctx->FrameLengthLines;
                pOX03a10Ctx->MaxIntegrationLine = pOX03a10Ctx->CurFrameLengthLines - 10;
                pOX03a10Ctx->MinIntegrationLine = 1;
                pOX03a10Ctx->AecMaxGain = 64;
                pOX03a10Ctx->AecMinGain = 1.0;
                break;
        default:
                return (RET_NOTAVAILABLE);
                break;
        }
        pOX03a10Ctx->MaxFps  = pOX03a10Ctx->SensorMode.fps;
        pOX03a10Ctx->MinFps  = 1 * ISI_FPS_QUANTIZE;
        pOX03a10Ctx->CurrFps = pOX03a10Ctx->MaxFps;
    }

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(OX03a10_DEBUG, "%s: OX03a10 System-Reset executed\n", __func__);
    osSleep(100);

    result = OX03a10_AecSetModeParameters(handle, pOX03a10Ctx);
    if (result != RET_SUCCESS) {
        TRACE(OX03a10_ERROR, "%s: SetupOutputWindow failed.\n",__func__);
        return (result);
    }

    pOX03a10Ctx->Configured = BOOL_TRUE;
    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return 0;
}

RESULT OX03a10_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;
    uint32_t correct_id = 0x5803;
    uint32_t sensor_id = 0;

    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);

    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    if (pOX03a10Ctx->KernelDriverFlag) {
        int ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_CHIP_ID,&sensor_id);
        if (ret != 0) {
            TRACE(OX03a10_ERROR,"%s: Read Sensor chip ID Error! \n", __func__);
            return (RET_FAILURE);
        }
    } else {

        result = OX03a10_IsiGetRevisionIss(handle, &sensor_id);
        if (result != RET_SUCCESS) {
            TRACE(OX03a10_ERROR, "%s: Read Sensor ID Error! \n",__func__);
            return (RET_FAILURE);
        }
    }

    if (correct_id != sensor_id) {
        TRACE(OX03a10_ERROR, "%s:ChipID =0x%x sensor_id=%x error! \n",__func__, correct_id, sensor_id);
        return (RET_FAILURE);
    }

    TRACE(OX03a10_INFO,"%s ChipID = 0x%08x, sensor_id = 0x%08x, success! \n", __func__,correct_id, sensor_id);
    TRACE(OX03a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint32_t reg_val;
    uint32_t sensor_id;

    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);

    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    if (!pValue)
        return (RET_NULL_POINTER);

    if (pOX03a10Ctx->KernelDriverFlag) {
        int ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_CHIP_ID,&sensor_id);
        if (ret != 0) {
            TRACE(OX03a10_ERROR, "%s: Read Sensor ID Error! \n",__func__);
            return (RET_FAILURE);
        }
    } else {
        reg_val = 0;
        result = OX03a10_IsiReadRegIss(handle, 0x300a, &reg_val);
        sensor_id = (reg_val & 0xff) << 8;

        reg_val = 0;
        result |= OX03a10_IsiReadRegIss(handle, 0x300b, &reg_val);
        sensor_id |= (reg_val & 0xff);

    }

    *pValue = sensor_id;
    TRACE(OX03a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{

    RESULT result = RET_SUCCESS;
    int ret = 0;
    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);

    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    if (pOX03a10Ctx->Configured != BOOL_TRUE) {
        return RET_WRONG_STATE;
    }

    int32_t enable = (uint32_t) on;
    if (pOX03a10Ctx->KernelDriverFlag) {
        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_STREAM, &enable);
    } else {
        ret = OX03a10_IsiWriteRegIss(handle, 0x0100, on);
    }

    if (ret != 0) {
        return (RET_FAILURE);
    }

    pOX03a10Ctx->Streaming = on;

    TRACE(OX03a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OX03a10_IsiGetGainLimitsIss(IsiSensorHandle_t handle, float *pMinGain, float *pMaxGain)
{
    const OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (pOX03a10Ctx == NULL) {
        TRACE(OX03a10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if ((pMinGain == NULL) || (pMaxGain == NULL)) {
        TRACE(OX03a10_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    *pMinGain = pOX03a10Ctx->AecMinGain;
    *pMaxGain = pOX03a10Ctx->AecMaxGain;

    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);
    return (result);
}

static RESULT OX03a10_IsiGetIntegrationTimeLimitsIss(IsiSensorHandle_t handle,float *pMinIntegrationTime, float *pMaxIntegrationTime)
{
    const OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);
    if (pOX03a10Ctx == NULL) {
        TRACE(OX03a10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if ((pMinIntegrationTime == NULL) || (pMaxIntegrationTime == NULL)) {
        TRACE(OX03a10_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    *pMinIntegrationTime = pOX03a10Ctx->AecMinIntegrationTime;
    *pMaxIntegrationTime = pOX03a10Ctx->AecMaxIntegrationTime;

    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiGetGainIss(IsiSensorHandle_t handle, float *pSetGain)
{
    const OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (pOX03a10Ctx == NULL) {
        TRACE(OX03a10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSetGain == NULL) {
        return (RET_NULL_POINTER);
    }

    *pSetGain = pOX03a10Ctx->AecCurGain;

    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiGetLongGainIss(IsiSensorHandle_t handle, float *gain)
{
    const OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;

    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (pOX03a10Ctx == NULL) {
        TRACE(OX03a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (gain == NULL) {
        return (RET_NULL_POINTER);
    }

    *gain = pOX03a10Ctx->AecCurLongGain;

    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);

    return (RET_SUCCESS);
}

RESULT OX03a10_IsiGetVSGainIss(IsiSensorHandle_t handle, float *pSetGain)
{
    const OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (pOX03a10Ctx == NULL) {
        TRACE(OX03a10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSetGain == NULL) {
        return (RET_NULL_POINTER);
    }

    *pSetGain = pOX03a10Ctx->AecCurVSGain;

    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);

    return (result);
}

RESULT OX03a10_IsiGetGainIncrementIss(IsiSensorHandle_t handle, float *pIncr)
{
    const OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (pOX03a10Ctx == NULL) {
        TRACE(OX03a10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pIncr == NULL)
        return (RET_NULL_POINTER);

    *pIncr = pOX03a10Ctx->AecGainIncrement;

    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);

    return (result);
}

RESULT OX03a10_IsiSetGainIss(IsiSensorHandle_t handle,float NewGain, float *pSetGain, const float *hdr_ratio)
{
    RESULT result = RET_SUCCESS;
    float gainA = 0.0f, gainD = 0.0f;
    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    if (NewGain < pOX03a10Ctx->AecMinGain) {
        NewGain = pOX03a10Ctx->AecMinGain;
    }
    if (NewGain > pOX03a10Ctx->AecMaxGain) {
        NewGain = pOX03a10Ctx->AecMaxGain;
    }
    if (pOX03a10Ctx->KernelDriverFlag) {
        uint32_t SensorGain = 0;
        SensorGain = NewGain * pOX03a10Ctx->gain_accuracy;
        int ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_GAIN, &SensorGain);
        if (ret != 0) {
            TRACE(OX03a10_ERROR, "%s: set sensor gain error\n",__func__);
            return RET_FAILURE;
        }
    } else {
        // uint32_t data = 0;
        TRACE(OX03a10_INFO, "%s: (Linear mode enter)\n", __func__);
        if ((pSetGain == NULL) || (hdr_ratio == NULL)) {
            return (RET_NULL_POINTER);
        }
        if (NewGain < pOX03a10Ctx->AecMinGain) {
            NewGain = pOX03a10Ctx->AecMinGain;
        }
        if (NewGain > pOX03a10Ctx->AecMaxGain) {
            NewGain = pOX03a10Ctx->AecMaxGain;
        }
        uint32_t GainA;
        uint32_t GainD;
        NewGain *= 13;
        if (NewGain < 15.5) {
            gainA = NewGain;
            gainD = 1.0;
        } else {
            gainA = 15.5;
            gainD = NewGain / gainA;
            if(gainD > 15.999)
            gainD = 15.999;
        }

        GainA=(uint32_t)(gainA*16);
        GainD= (uint32_t)(gainD*1024);
        //Again HCG
        // result = OX03a10_IsiWriteRegIss(handle, 0x3208,  0x00);
        result = OX03a10_IsiWriteRegIss(handle, 0x3508,  (GainA>>4));
        result |= OX03a10_IsiWriteRegIss(handle, 0x3509,  ((GainA & 0x0f) << 4));
        result |= OX03a10_IsiWriteRegIss(handle, 0x350a, (GainD>>10) & 0xf);
        result |= OX03a10_IsiWriteRegIss(handle, 0x350b, (GainD >> 2) & 0xff);
        result |= OX03a10_IsiWriteRegIss(handle, 0x350c, (GainD & 0x3) << 6);
        //  result = OX03a10_IsiWriteRegIss(handle, 0x3208,  0x10);
        NewGain /= 13;
        if (NewGain < 15.5) {
            gainA = NewGain;
            gainD = 1.0;
        } else {
            gainA = 15.5;
            gainD = NewGain / gainA;

            if(gainD > 15.999) {
                gainD = 15.99;
            }
        }
        GainA=(uint32_t)(gainA*16);
        GainD= (uint32_t)(gainD*1024);
        //LCG Agian
        // result = OX03a10_IsiWriteRegIss(handle, 0x3208,  0x00);
        result |= OX03a10_IsiWriteRegIss(handle, 0x3548, (GainA>>4));
        result |= OX03a10_IsiWriteRegIss(handle, 0x3549, (GainA & 0x000f) << 4);

        result |= OX03a10_IsiWriteRegIss(handle, 0x354a, (GainD>>10) & 0xf);
        result |= OX03a10_IsiWriteRegIss(handle, 0x354b, (GainD >> 2) & 0xff);
        result |= OX03a10_IsiWriteRegIss(handle, 0x354c, (GainD & 0x3) << 6);
        //result = OX03a10_IsiWriteRegIss(handle, 0x3208,  0x10);
    }
    pOX03a10Ctx->AecCurGain= gainA * gainD;
    *pSetGain = pOX03a10Ctx->AecCurGain;
    TRACE(OX03a10_DEBUG, "%s: g=%f\n", __func__, *pSetGain);
    return (result);
    }

RESULT OX03a10_IsiSetLongGainIss(IsiSensorHandle_t handle, float gain)
{
    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;

    if (!pOX03a10Ctx || !pOX03a10Ctx->IsiCtx.HalHandle) {
        TRACE(OX03a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;
    if (pOX03a10Ctx->KernelDriverFlag) {
        uint32_t SensorGain = 0;
        SensorGain = gain * pOX03a10Ctx->gain_accuracy;
        if (pOX03a10Ctx->LastLongGain != SensorGain) {
            int ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_LONG_GAIN, &SensorGain);
            if (ret != 0) {
                TRACE(OX03a10_ERROR,"%s: set long gain failed\n");
                return (RET_FAILURE);

            }
            pOX03a10Ctx->LastLongGain = SensorGain;
            pOX03a10Ctx->AecCurLongGain = gain;
        }

    }

    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return (RET_SUCCESS);
}

RESULT OX03a10_IsiSetVSGainIss(IsiSensorHandle_t handle,float NewIntegrationTime,float NewGain, float *pSetGain, const float *hdr_ratio)
{
    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (!pOX03a10Ctx) {
        TRACE(OX03a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }
    const HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;
    RESULT result = RET_SUCCESS;
    float again ,dgain;
    uint32_t a = 0U;
    uint32_t d = 0U;

    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (!pSetGain || !hdr_ratio) {
        return (RET_NULL_POINTER);
    }

    if (NewGain < 15.5) {
        again = NewGain;
        dgain = 1.0;
    } else {
        again = 15.5;
        dgain = NewGain / again;
        if (dgain > 15.999)
        dgain = 15.999;
    }
    a =(uint32_t)(again*16);
    d =(uint32_t)(dgain*1024);
    pOX03a10Ctx->AecCurVSGain = again*dgain;

    //result = OX03a10_IsiWriteRegIss(handle, 0x3208,  0x10);
    result = OX03a10_IsiWriteRegIss(handle, 0x3588, (a>>4));
    result |= OX03a10_IsiWriteRegIss(handle, 0x3589, (a & 0x0f) << 4);
    //Dgain
    result |= OX03a10_IsiWriteRegIss(handle, 0x358A, (d>>10) & 0x0f);
    result |= OX03a10_IsiWriteRegIss(handle, 0x358B, (d>>2) &0xff);
    result |= OX03a10_IsiWriteRegIss(handle, 0x358C, (d&0x3) << 6);
    //result = OX03a10_IsiWriteRegIss(handle, 0x3208,  0x00);

    *pSetGain = pOX03a10Ctx->AecCurVSGain;
    TRACE(OX03a10_DEBUG, "%s: g=%f\n", __func__, *pSetGain);
    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiGetIntegrationTimeIss(IsiSensorHandle_t handle, float *pSetIntegrationTime)
{
    const OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (!pOX03a10Ctx) {
        TRACE(OX03a10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSetIntegrationTime)
        return (RET_NULL_POINTER);
    *pSetIntegrationTime = pOX03a10Ctx->AecCurIntegrationTime;
    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiGetLongIntegrationTimeIss(IsiSensorHandle_t handle, float *pIntegrationTime)
{
    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (!pOX03a10Ctx) {
        TRACE(OX03a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }
    if (!pIntegrationTime)
        return (RET_NULL_POINTER);

    pOX03a10Ctx->AecCurLongIntegrationTime =  pOX03a10Ctx->AecCurIntegrationTime;

    *pIntegrationTime = pOX03a10Ctx->AecCurLongIntegrationTime;
    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return (RET_SUCCESS);
}

RESULT OX03a10_IsiGetVSIntegrationTimeIss(IsiSensorHandle_t handle, float *pSetIntegrationTime)
{
    const OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (!pOX03a10Ctx) {
        TRACE(OX03a10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    if (!pSetIntegrationTime)
        return (RET_NULL_POINTER);

    *pSetIntegrationTime = pOX03a10Ctx->AecCurVSIntegrationTime;
    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiGetIntegrationTimeIncrementIss(IsiSensorHandle_t handle, float *pIncr)
{
    const OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (!pOX03a10Ctx) {
        TRACE(OX03a10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pIncr)
        return (RET_NULL_POINTER);

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pOX03a10Ctx->AecIntegrationTimeIncrement;
    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiSetIntegrationTimeIss(IsiSensorHandle_t handle,float NewIntegrationTime,float *pSetIntegrationTime, uint8_t * pNumberOfFramesToSkip, float *hdr_ratio)
{
    RESULT result = RET_SUCCESS;

    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (!pOX03a10Ctx) {
        TRACE(OX03a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    uint32_t exp_line = 0;
    uint32_t exp_line_old = 0;
    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (!pSetIntegrationTime || !pNumberOfFramesToSkip) {
        TRACE(OX03a10_ERROR,"%s: Invalid parameter (NULL pointer detected)\n",__func__);
        return (RET_NULL_POINTER);
    }

    exp_line = NewIntegrationTime / pOX03a10Ctx->one_line_exp_time;
    exp_line_old = exp_line;
    exp_line = MIN(pOX03a10Ctx->MaxIntegrationLine,MAX(pOX03a10Ctx->MinIntegrationLine, exp_line));

    TRACE(OX03a10_DEBUG, "%s: set AEC_PK_EXPO=0x%05x\n", __func__, exp_line);

    if (exp_line != pOX03a10Ctx->OldIntegrationTime) {
        if (pOX03a10Ctx->KernelDriverFlag) {
            ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_EXP, &exp_line);
        } else {
            // result = OX03a10_IsiWriteRegIss(handle, 0x3208, 0x00);
            result = OX03a10_IsiWriteRegIss(handle, 0x3501, (exp_line >> 8) & 0xff);
            result |= OX03a10_IsiWriteRegIss(handle, 0x3502, exp_line & 0xff);
            //  result = OX03a10_IsiWriteRegIss(handle, 0x3208, 0x10);
        }
        pOX03a10Ctx->OldIntegrationTime = exp_line;    // remember current integration time
        pOX03a10Ctx->AecCurIntegrationTime = exp_line * pOX03a10Ctx->one_line_exp_time;
        *pNumberOfFramesToSkip = 1U;    //skip 1 frame
    } else {
        *pNumberOfFramesToSkip = 0U;    //no frame skip
    }
    if (exp_line_old != exp_line) {
        *pSetIntegrationTime = pOX03a10Ctx->AecCurIntegrationTime;
    } else {
        *pSetIntegrationTime = NewIntegrationTime;
    }
    TRACE(OX03a10_DEBUG, "%s: Ti=%f\n", __func__, *pSetIntegrationTime);
    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiSetLongIntegrationTimeIss(IsiSensorHandle_t handle,float IntegrationTime)
{
    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (!handle || !pOX03a10Ctx->IsiCtx.HalHandle) {
        TRACE(OX03a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    uint32_t exp_line = 0;
    exp_line = IntegrationTime / pOX03a10Ctx->one_line_exp_time;
    exp_line = MIN(pOX03a10Ctx->MaxIntegrationLine, MAX(pOX03a10Ctx->MinIntegrationLine, exp_line));

    if (exp_line != pOX03a10Ctx->LastLongExpLine) {
        if (pOX03a10Ctx->KernelDriverFlag) {
            int ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_LONG_EXP, &exp_line);
            if (ret != 0) {
                TRACE(OX03a10_ERROR,"%s: set long gain failed\n");
                return RET_FAILURE;
            }
        }
        pOX03a10Ctx->LastLongExpLine = exp_line;
        pOX03a10Ctx->AecCurLongIntegrationTime =  pOX03a10Ctx->LastLongExpLine*pOX03a10Ctx->one_line_exp_time;
    }


    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return (RET_SUCCESS);
}

RESULT OX03a10_IsiSetVSIntegrationTimeIss(IsiSensorHandle_t handle,float NewIntegrationTime,float *pSetVSIntegrationTime,
                                        uint8_t * pNumberOfFramesToSkip, const float *hdr_ratio)
{

    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (!pOX03a10Ctx) {
        TRACE(OX03a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    RESULT result = RET_SUCCESS;
    uint32_t exp_line = 0;

    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (!pSetVSIntegrationTime || !pNumberOfFramesToSkip) {
        TRACE(OX03a10_ERROR,"%s: Invalid parameter (NULL pointer detected)\n",__func__);
        return (RET_NULL_POINTER);
    }

    TRACE(OX03a10_INFO,"%s:  maxIntegrationTime-=%f minIntegrationTime = %f\n", __func__, pOX03a10Ctx->AecMaxIntegrationTime,pOX03a10Ctx->AecMinIntegrationTime);
    NewIntegrationTime /= hdr_ratio[0];
    exp_line = NewIntegrationTime / pOX03a10Ctx->one_line_exp_time;
    exp_line = MIN(0x20, MAX(1, exp_line));

    if (exp_line != pOX03a10Ctx->OldVsIntegrationTime) {
        if (pOX03a10Ctx->KernelDriverFlag) {
            ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_SEXP, &exp_line);
        } else {
            //result = OX03a10_IsiWriteRegIss(handle, 0x3208, 0x00);
            result = OX03a10_IsiWriteRegIss(handle, 0x3581, (exp_line >> 8) & 0xff);
            result |= OX03a10_IsiWriteRegIss(handle, 0x3582, exp_line & 0xff);
            // result = OX03a10_IsiWriteRegIss(handle, 0x3208, 0x10);
    }

        pOX03a10Ctx->OldVsIntegrationTime = exp_line;
        pOX03a10Ctx->AecCurVSIntegrationTime = exp_line * pOX03a10Ctx->one_line_exp_time;    //remember current integration time
        *pNumberOfFramesToSkip = 1U;    //skip 1 fram
    } else {
        *pNumberOfFramesToSkip = 0U;    //no frame ski
    }

    *pSetVSIntegrationTime = pOX03a10Ctx->AecCurVSIntegrationTime;
    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiExposureControlIss(IsiSensorHandle_t handle,float NewGain,float NewIntegrationTime,
                    uint8_t * pNumberOfFramesToSkip,float *pSetGain, float *pSetIntegrationTime, float *hdr_ratio)
{
    const OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (pOX03a10Ctx == NULL) {
        TRACE(OX03a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }
    if ((pNumberOfFramesToSkip == NULL) || (pSetGain == NULL)|| (pSetIntegrationTime == NULL)) {
        TRACE(OX03a10_ERROR,"%s: Invalid parameter (NULL pointer detected)\n",__func__);
        return (RET_NULL_POINTER);
    }

    TRACE(OX03a10_DEBUG, "%s: g=%f, Ti=%f\n", __func__, NewGain,NewIntegrationTime);

    if (hdr_ratio[0] >= 1) {
        if (NewGain > 8) {
            result = OX03a10_IsiSetVSIntegrationTimeIss(handle, NewIntegrationTime*NewGain, pSetIntegrationTime, pNumberOfFramesToSkip, hdr_ratio);
        } else{
            result = OX03a10_IsiSetVSIntegrationTimeIss(handle, NewIntegrationTime, pSetIntegrationTime, pNumberOfFramesToSkip, hdr_ratio);
        }
        float vsExpo = *pSetIntegrationTime;
        float vsGain = NewIntegrationTime*NewGain/ (vsExpo * hdr_ratio[0]);

        //printf("NewTime: %f, NewGain %f, vsExpo %f, vsGain %f, ratio %f\n", NewIntegrationTime, NewGain, vsExpo, vsGain, hdr_ratio[0]);
        result |= OX03a10_IsiSetVSGainIss(handle, NewIntegrationTime, vsGain, pSetGain, hdr_ratio);
    }
    result |= OX03a10_IsiSetIntegrationTimeIss(handle, NewIntegrationTime, pSetIntegrationTime, pNumberOfFramesToSkip, hdr_ratio);
    result |= OX03a10_IsiSetGainIss(handle, NewGain*NewIntegrationTime/(*pSetIntegrationTime), pSetGain, hdr_ratio);

    return result;
}

RESULT OX03a10_IsiGetCurrentExposureIss(IsiSensorHandle_t handle, float *pSetGain, float *pSetIntegrationTime, float *hdr_ratio)
{

    const OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (pOX03a10Ctx == NULL) {
        TRACE(OX03a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if ((pSetGain == NULL) || (pSetIntegrationTime == NULL)){
        return (RET_NULL_POINTER);
    }

    *pSetGain = pOX03a10Ctx->AecCurGain;
    *pSetIntegrationTime = pOX03a10Ctx->AecCurIntegrationTime;
    *hdr_ratio = pOX03a10Ctx->CurHdrRatio;

    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t *pFps)
{
    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (pOX03a10Ctx == NULL) {
        TRACE(OX03a10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    if (pOX03a10Ctx->KernelDriverFlag) {
        ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_FPS, pFps);
        pOX03a10Ctx->CurrFps = *pFps;
    }

    *pFps = pOX03a10Ctx->CurrFps;

    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t Fps)
{
    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    if (pOX03a10Ctx == NULL) {
        TRACE(OX03a10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    if (Fps > pOX03a10Ctx->MaxFps) {
        TRACE(OX03a10_ERROR, "%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, Fps, 
                       pOX03a10Ctx->MaxFps, pOX03a10Ctx->MinFps, pOX03a10Ctx->MaxFps);
        Fps = pOX03a10Ctx->MaxFps;
    }
    if (Fps < pOX03a10Ctx->MinFps) {
        TRACE(OX03a10_ERROR, "%s: set fps(%d) out of range, correct to %d (%d, %d)\n",
              __func__, Fps, pOX03a10Ctx->MinFps, pOX03a10Ctx->MinFps, pOX03a10Ctx->MaxFps);
        Fps = pOX03a10Ctx->MinFps;
    }
    if (pOX03a10Ctx->KernelDriverFlag) {
        int ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_FPS, &Fps);
        if (ret != 0) {
            TRACE(OX03a10_ERROR, "%s: set sensor fps=%d error\n", __func__);
            return (RET_FAILURE);
        }

        ret |= ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_SENSOR_MODE, &(pOX03a10Ctx->SensorMode));
            pOX03a10Ctx->MaxIntegrationLine = pOX03a10Ctx->SensorMode.ae_info.max_integration_time;
            pOX03a10Ctx->AecMaxIntegrationTime = pOX03a10Ctx->MaxIntegrationLine * pOX03a10Ctx->one_line_exp_time;
    } else {
        uint16_t FrameLengthLines;
        FrameLengthLines = pOX03a10Ctx->FrameLengthLines * pOX03a10Ctx->MaxFps / Fps;
        result = OX03a10_IsiWriteRegIss(handle, 0x30b2, (FrameLengthLines >> 8) & 0xff);
        result |= OX03a10_IsiWriteRegIss(handle, 0x30b3, FrameLengthLines & 0xff);
        if (result != RET_SUCCESS) {
            TRACE(OX03a10_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
            return (RET_FAILURE);
        }
        pOX03a10Ctx->CurrFps = Fps;
        pOX03a10Ctx->CurFrameLengthLines = FrameLengthLines;
        pOX03a10Ctx->MaxIntegrationLine = pOX03a10Ctx->CurFrameLengthLines - 3;
        pOX03a10Ctx->AecMaxIntegrationTime =
        pOX03a10Ctx->MaxIntegrationLine * pOX03a10Ctx->one_line_exp_time;
    }

    TRACE(OX03a10_INFO, "%s: set sensor fps = %d\n", __func__, pOX03a10Ctx->CurrFps);

    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX03a10_IsiGetAutoFpsInfoIss(IsiSensorHandle_t handle, IsiAutoFps_t *pAutoFpsInfo)
{
    RESULT result = RET_SUCCESS;
    return (result);
}

RESULT OX03a10_IsiGetStartEvIss(IsiSensorHandle_t handle, uint64_t *pStartEv)
{
    RESULT result = RET_SUCCESS;
    return (result);
}

RESULT OX03a10_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    if (pOX03a10Ctx->SensorMode.hdr_mode == SENSOR_MODE_HDR_NATIVE) {
        pIspStatus->useSensorBLC = true;
        pIspStatus->useSensorAWB = true;
    } else {
        pIspStatus->useSensorBLC = false;
        pIspStatus->useSensorAWB = false;
    }

    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

static RESULT OX03a10_IsiSetBlcIss(IsiSensorHandle_t handle, IsiBlc_t *pBlc)
{
    int32_t ret = 0;
    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL) return RET_NULL_POINTER;

    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;
    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_BLC, pBlc);
    if (ret != 0) {
        TRACE(OX03a10_ERROR, "%s: set blc error\n", __func__);
    }

    return RET_SUCCESS;
}

static RESULT OX03a10_IsiSetWBIss(IsiSensorHandle_t handle, IsiWB_t *pWb)
{
    int32_t ret = 0;
    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_WRONG_HANDLE;
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    if (pWb == NULL)
        return RET_NULL_POINTER;

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_WB, pWb);
    if (ret != 0) {
        TRACE(OX03a10_ERROR, "%s: set wb error\n", __func__);
    }

    return RET_SUCCESS;
}

RESULT OX03a10_IsiSetTpgIss(IsiSensorHandle_t handle, IsiTpg_t Tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(OX03a10_INFO, "%s: (enter)\n", __func__);

    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOX03a10Ctx->Configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (Tpg.enable == 0) {
        result = OX03a10_IsiWriteRegIss(handle, 0x3253, 0x00);
    } else {
        result = OX03a10_IsiWriteRegIss(handle, 0x3253, 0x80);
    }

    pOX03a10Ctx->TestPattern = Tpg.enable;

    TRACE(OX03a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OX03a10_IsiGetExpandCurveIss(IsiSensorHandle_t handle, IsiCompandCurve_t *pCurve)
{
    int32_t ret = 0;
    OX03a10_Context_t *pOX03a10Ctx = (OX03a10_Context_t *) handle;
    if (pOX03a10Ctx == NULL || pOX03a10Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }
    HalContext_t *pHalCtx = (HalContext_t *) pOX03a10Ctx->IsiCtx.HalHandle;

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_EXPAND_CURVE, pCurve);
    if (ret != 0) {
        TRACE(OX03a10_ERROR, "%s: get  expand cure error\n", __func__);
        return RET_FAILURE;
    }

    return RET_SUCCESS;
}

RESULT OX03a10_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "OX03a10";
    TRACE(OX03a10_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = OX03a10_IsiCreateIss;
        pIsiSensor->pIsiReleaseIss                      = OX03a10_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = OX03a10_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = OX03a10_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = OX03a10_IsiGetModeIss;
        pIsiSensor->pIsiSetModeIss                      = OX03a10_IsiSetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = OX03a10_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = OX03a10_IsiGetCapsIss;
        pIsiSensor->pIsiSetupIss                        = OX03a10_IsiSetupIss;
        pIsiSensor->pIsiCheckConnectionIss              = OX03a10_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = OX03a10_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = OX03a10_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiExposureControlIss              = OX03a10_IsiExposureControlIss;
        pIsiSensor->pIsiGetGainLimitsIss                = OX03a10_IsiGetGainLimitsIss;
        pIsiSensor->pIsiGetIntegrationTimeLimitsIss     = OX03a10_IsiGetIntegrationTimeLimitsIss;
        pIsiSensor->pIsiGetCurrentExposureIss           = OX03a10_IsiGetCurrentExposureIss;
        pIsiSensor->pIsiGetVSGainIss                    = OX03a10_IsiGetVSGainIss;
        pIsiSensor->pIsiGetGainIss                      = OX03a10_IsiGetGainIss;
        pIsiSensor->pIsiGetLongGainIss                  = OX03a10_IsiGetLongGainIss;
        pIsiSensor->pIsiGetGainIncrementIss             = OX03a10_IsiGetGainIncrementIss;
        pIsiSensor->pIsiSetGainIss                      = OX03a10_IsiSetGainIss;
        pIsiSensor->pIsiGetIntegrationTimeIss           = OX03a10_IsiGetIntegrationTimeIss;
        pIsiSensor->pIsiGetVSIntegrationTimeIss         = OX03a10_IsiGetVSIntegrationTimeIss;
        pIsiSensor->pIsiGetLongIntegrationTimeIss       = OX03a10_IsiGetLongIntegrationTimeIss;
        pIsiSensor->pIsiGetIntegrationTimeIncrementIss  = OX03a10_IsiGetIntegrationTimeIncrementIss;
        pIsiSensor->pIsiSetIntegrationTimeIss           = OX03a10_IsiSetIntegrationTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = OX03a10_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = OX03a10_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = OX03a10_IsiGetIspStatusIss;
        pIsiSensor->pIsiSetBlcIss                       = OX03a10_IsiSetBlcIss;
        pIsiSensor->pIsiSetWBIss                        = OX03a10_IsiSetWBIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetPowerIss                     = OX03a10_IsiSetPowerIss;
        pIsiSensor->pIsiSetTpgIss                       = OX03a10_IsiSetTpgIss;
        pIsiSensor->pIsiGetExpandCurveIss               = OX03a10_IsiGetExpandCurveIss;

        /* AF */
        pIsiSensor->pIsiFocusCreateIss                  = NULL;
        pIsiSensor->pIsiFocusReleaseIss                 = NULL;
        pIsiSensor->pIsiFocusGetCalibrateIss            = NULL;
        pIsiSensor->pIsiFocusSetIss                     = NULL;
        pIsiSensor->pIsiFocusGetIss                     = NULL;

    } else {
        result = RET_NULL_POINTER;
    }

    TRACE(OX03a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t OX03a10_IsiCamDrvConfig = {
    .cameraDriverID      = 0x5803,
    .pIsiGetSensorIss    = OX03a10_IsiGetSensorIss,
};
