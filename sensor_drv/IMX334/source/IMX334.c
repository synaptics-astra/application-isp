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
#include <math.h>
#include "isi.h"
#include "isi_iss.h"

#include "IMX334_priv.h"

CREATE_TRACER(IMX334_INFO , "IMX334: ", INFO,    1);
CREATE_TRACER(IMX334_WARN , "IMX334: ", WARNING, 1);
CREATE_TRACER(IMX334_ERROR, "IMX334: ", ERROR,   1);
CREATE_TRACER(IMX334_DEBUG,     "IMX334: ", INFO, 1);
CREATE_TRACER(IMX334_REG_INFO , "IMX334: ", INFO, 1);
CREATE_TRACER(IMX334_REG_DEBUG, "IMX334: ", INFO, 1);

#ifdef SUBDEV_V4L2
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#undef TRACE
#define TRACE(x, ...)
#endif

#define IMX334_MIN_GAIN_STEP    (1.0f/128.0f)  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain) */
#define IMX334_PLL_PCLK         74250000
#define IMX334_HMAX             0xaec
#define IMX334_VMAX             0xac4
#define IMX334_MAX_GAIN_AEC     (32.0f)       /**< max. gain used by the AEC (arbitrarily chosen, hardware limit = 62.0, driver limit = 32.0) */
#define IMX334_VS_MAX_INTEGRATION_TIME (0.0018)


/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static struct vvsensor_mode_s pimx334_mode_info[] = {
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
        .bit_width = 10,
        .bayer_pattern = BAYER_RGGB,
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
        .stitching_mode = SENSOR_STITCHING_3DOL,
        .bit_width = 10,
        .bayer_pattern = BAYER_RGGB,
    },
    {
        .index     = 2,
        .size      = {
            .bounds_width  = 1280,
            .bounds_height = 720,
            .top           = 0,
            .left          = 0,
            .width         = 1280,
            .height        = 720,
        },
        .fps      = 60 * ISI_FPS_QUANTIZE,
        .hdr_mode = SENSOR_MODE_LINEAR,
        .bit_width = 10,
        .bayer_pattern = BAYER_RGGB,
    }
};

static RESULT IMX334_IsiSetPowerIss(IsiSensorHandle_t handle, bool_t on) {
    RESULT result = RET_SUCCESS;

    int ret = 0;
    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL || pIMX334Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }
    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;

    TRACE(IMX334_INFO, "%s (enter)\n", __func__);

    int32_t enable = on;
    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_POWER, &enable);
    if (ret != 0) {
        TRACE(IMX334_ERROR, "%s: sensor set power error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(IMX334_INFO, "%s (exit)\n", __func__);
    return (result);
}

#ifdef SUBDEV_CHAR
static RESULT IMX334_IsiSetClkIss(IsiSensorHandle_t handle, uint32_t Clk) {
    RESULT result = RET_SUCCESS;
    int32_t ret = 0;

    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL || pIMX334Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }
    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;

    TRACE(IMX334_INFO, "%s (enter)\n", __func__);

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_CLK, &Clk);
    if (ret != 0) {
        TRACE(IMX334_ERROR, "%s: sensor set clk error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(IMX334_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX334_IsiGetClkIss(IsiSensorHandle_t handle, uint32_t * pClk) {
    RESULT result = RET_SUCCESS;
    int ret = 0;

    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL || pIMX334Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;

    TRACE(IMX334_INFO, "%s (enter)\n", __func__);

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_CLK, pClk);
    if (ret != 0) {
        TRACE(IMX334_ERROR, "%s: sensor get clk error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(IMX334_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX334_IsiConfigSCCBIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;
    int ret = 0;
    TRACE(IMX334_INFO, "%s (enter)\n", __func__);

    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL || pIMX334Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;

    static IsiBus_t SensorSccbInfo;
    SensorSccbInfo.i2c.slaveAddr = (0x34 >> 1);
    SensorSccbInfo.i2c.addrWidth = 2;
    SensorSccbInfo.i2c.dataWidth = 1;

    struct vvsensor_sccb_cfg_s sensor_sccb_config;
    sensor_sccb_config.slave_addr = SensorSccbInfo.i2c.slaveAddr;
    sensor_sccb_config.addr_byte  = SensorSccbInfo.i2c.addrWidth;
    sensor_sccb_config.data_byte  = SensorSccbInfo.i2c.dataWidth;

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_SENSOR_SCCB_CFG,
          &sensor_sccb_config);
    if (ret != 0) {
        TRACE(IMX334_ERROR, "%s: sensor config sccb info error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(IMX334_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}
#endif

static RESULT IMX334_IsiCreateIss(IsiSensorInstanceConfig_t * pConfig) {
    RESULT result = RET_SUCCESS;
    IMX334_Context_t *pIMX334Ctx;

    TRACE(IMX334_INFO, "%s (enter)\n", __func__);

    if (!pConfig || !pConfig->pSensor)
        return (RET_NULL_POINTER);

    pIMX334Ctx = (IMX334_Context_t *) malloc(sizeof(IMX334_Context_t));
    if (!pIMX334Ctx) {
        TRACE(IMX334_ERROR, "%s: Can't allocate IMX334 context\n", __func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pIMX334Ctx, 0, sizeof(IMX334_Context_t));

    result = HalAddRef(pConfig->halHandle);
    if (result != RET_SUCCESS) {
        free(pIMX334Ctx);
        return (result);
    }

    pIMX334Ctx->IsiCtx.HalHandle = pConfig->halHandle;
    pIMX334Ctx->IsiCtx.pSensor = pConfig->pSensor;
    pIMX334Ctx->GroupHold = BOOL_FALSE;
    pIMX334Ctx->OldGain = 1.0;
    pIMX334Ctx->OldIntegrationTime = 0.01;
    pIMX334Ctx->Configured = BOOL_FALSE;
    pIMX334Ctx->Streaming = BOOL_FALSE;
    pIMX334Ctx->TestPattern = BOOL_FALSE;
    pIMX334Ctx->isAfpsRun = BOOL_FALSE;
    pIMX334Ctx->SensorMode.index = pConfig->sensorModeIndex;
    pConfig->hSensor = (IsiSensorHandle_t) pIMX334Ctx;
#ifdef SUBDEV_CHAR
    struct vvsensor_mode_s *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pimx334_mode_info)/ sizeof(struct vvsensor_mode_s); i++) {
        if (pimx334_mode_info[i].index == pIMX334Ctx->SensorMode.index) {
            SensorDefaultMode = &(pimx334_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                memcpy(pIMX334Ctx->SensorRegCfgFile, "IMX334_mipi4lane_1080p_init.txt", strlen("IMX334_mipi4lane_1080p_init.txt"));
                break;
            case 1: //3Dol mode
                memcpy(pIMX334Ctx->SensorRegCfgFile, "IMX334_mipi4lane_1080p_3dol_init.txt", strlen("IMX334_mipi4lane_1080p_3dol_init.txt"));
                break;
            default:
                break;
        }

        if (access(pIMX334Ctx->SensorRegCfgFile, F_OK) == 0) {
            pIMX334Ctx->KernelDriverFlag = 0;
            memcpy(&(pIMX334Ctx->SensorMode),SensorDefaultMode,sizeof(struct vvsensor_mode_s));
        } else {
            pIMX334Ctx->KernelDriverFlag = 1;
        }
    } else {
        pIMX334Ctx->KernelDriverFlag = 1;
    }

    result = IMX334_IsiSetPowerIss(pIMX334Ctx, BOOL_TRUE);
    RETURN_RESULT_IF_DIFFERENT(RET_SUCCESS, result);

    uint32_t SensorClkIn = 0;
    if (pIMX334Ctx->KernelDriverFlag) {
        result = IMX334_IsiGetClkIss(pIMX334Ctx, &SensorClkIn);
        RETURN_RESULT_IF_DIFFERENT(RET_SUCCESS, result);
    }

    result = IMX334_IsiSetClkIss(pIMX334Ctx, SensorClkIn);
    RETURN_RESULT_IF_DIFFERENT(RET_SUCCESS, result);

    if (!pIMX334Ctx->KernelDriverFlag) {
        result = IMX334_IsiConfigSCCBIss(pIMX334Ctx);
        RETURN_RESULT_IF_DIFFERENT(RET_SUCCESS, result);
    }

    pIMX334Ctx->pattern = ISI_BPAT_GBRG;
#endif

#ifdef SUBDEV_V4L2
    pIMX334Ctx->pattern = ISI_BPAT_BGGR;
    pIMX334Ctx->subdev = HalGetFdHandle(pConfig->halHandle, HAL_MODULE_SENSOR);//two sensors??
    //HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;
    //pHalCtx->sensor_fd = pIMX334Ctx->subdev;
    pIMX334Ctx->KernelDriverFlag = 1;
#endif
    TRACE(IMX334_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX334_IsiReleaseIss(IsiSensorHandle_t handle) {
    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX334_INFO, "%s (enter)\n", __func__);

    if (pIMX334Ctx == NULL)
        return (RET_WRONG_HANDLE);

    (void)IMX334_IsiSetStreamingIss(pIMX334Ctx, BOOL_FALSE);
    (void)IMX334_IsiSetPowerIss(pIMX334Ctx, BOOL_FALSE);
    (void)HalDelRef(pIMX334Ctx->IsiCtx.HalHandle);

    MEMSET(pIMX334Ctx, 0, sizeof(IMX334_Context_t));
    free(pIMX334Ctx);
    TRACE(IMX334_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX334_IsiReadRegIss (IsiSensorHandle_t handle, const uint32_t Addr, uint32_t *pValue) {
    RESULT result = RET_SUCCESS;
    int32_t ret = 0;
    TRACE(IMX334_INFO, "%s (enter)\n", __func__);

    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL || pIMX334Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;

    struct vvsensor_sccb_data_s sccb_data;
    sccb_data.addr = Addr;
    sccb_data.data = 0;
    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_READ_REG, &sccb_data);
    if (ret != 0) {
        TRACE(IMX334_ERROR, "%s: read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    *pValue = sccb_data.data;

    TRACE(IMX334_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT IMX334_IsiWriteRegIss(IsiSensorHandle_t handle, const uint32_t Addr, const uint32_t Value) {
    RESULT result = RET_SUCCESS;
    int ret = 0;
    TRACE(IMX334_INFO, "%s (enter)\n", __func__);

    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL || pIMX334Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;

    struct vvsensor_sccb_data_s sccb_data;
    sccb_data.addr = Addr;
    sccb_data.data = Value;

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_WRITE_REG, &sccb_data);
    if (ret != 0) {
        TRACE(IMX334_ERROR, "%s: write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(IMX334_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT IMX334_IsiGetModeIss(IsiSensorHandle_t handle, IsiMode_t *pMode)
{
    TRACE(IMX334_INFO, "%s (enter)\n", __func__);
    const IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pIMX334Ctx->SensorMode), sizeof(pIMX334Ctx->SensorMode));

    TRACE(IMX334_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static RESULT IMX334_IsiSetModeIss(IsiSensorHandle_t handle, IsiMode_t *pMode)
{
    int ret = 0;
    TRACE(IMX334_INFO, "%s (enter)\n", __func__);

    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;

    ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_SENSOR_MODE, pMode);
    if (ret != 0) {
        TRACE(IMX334_ERROR, "%s: write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(IMX334_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT IMX334_IsiEnumModeIss(IsiSensorHandle_t handle, IsiEnumMode_t *pEnumMode)
{
    const IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL || pIMX334Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pimx334_mode_info)/sizeof(pimx334_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pimx334_mode_info)/sizeof(pimx334_mode_info[0])); i++) {
        if (pimx334_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pimx334_mode_info[i],sizeof(IsiMode_t));
            TRACE(IMX334_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT IMX334_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(IMX334_INFO, "%s (enter)\n", __func__);

    if (pIMX334Ctx == NULL)
        return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pIMX334Ctx->SensorMode.bit_width;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pIMX334Ctx->SensorMode.bayer_pattern;
    pCaps->resolution.width  = pIMX334Ctx->SensorMode.size.width;
    pCaps->resolution.height = pIMX334Ctx->SensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(IMX334_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX334_IsiGetRegCfgIss(const char *registerFileName, struct vvsensor_sccb_array_s *array)
{
    if (NULL == registerFileName) {
        TRACE(IMX334_ERROR, "%s:registerFileName is NULL\n", __func__);
        return (RET_NULL_POINTER);
    }
#ifdef SUBDEV_CHAR
    FILE *fp = NULL;
    fp = fopen(registerFileName, "rb");
    if (!fp) {
        TRACE(IMX334_ERROR, "%s:load register file  %s error!\n", __func__, registerFileName);
        return (RET_FAILURE);
    }

    char LineBuf[512];
    char *str;
    uint32_t FileTotalLine = 0;
    while (!feof(fp)) {
        str = fgets(LineBuf, 512, fp);
        if(str == NULL) {
            printf("fgets null");
        }
        FileTotalLine++;
    }

    array->sccb_data =
        malloc(FileTotalLine * sizeof(struct vvsensor_sccb_data_s));
    if (array->sccb_data == NULL) {
        TRACE(IMX334_ERROR, "%s:malloc failed NULL Point!\n", __func__, registerFileName);
        return (RET_FAILURE);
    }
    rewind(fp);

    array->count = 0;
    while (!feof(fp)) {
        memset(LineBuf, 0, sizeof(LineBuf));
        str = fgets(LineBuf, 512, fp);
        if(str == NULL) {
            printf("fgets null");
        }

        int result = sscanf(LineBuf, "0x%x 0x%x", &(array->sccb_data[array->count].addr), &(array->sccb_data[array->count].data));
        if (result != 2)
            continue;
        array->count++;

    }
#endif

    return 0;
}

static RESULT IMX334_AecSetModeParameters (IsiSensorHandle_t handle, IMX334_Context_t * pIMX334Ctx) 
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX334_INFO, "%s%s: (enter)\n", __func__, pIMX334Ctx->isAfpsRun ? "(AFPS)" : "");

    pIMX334Ctx->AecIntegrationTimeIncrement = pIMX334Ctx->one_line_exp_time;
    pIMX334Ctx->AecMinIntegrationTime = pIMX334Ctx->one_line_exp_time * pIMX334Ctx->MinIntegrationLine;
    pIMX334Ctx->AecMaxIntegrationTime = pIMX334Ctx->one_line_exp_time * pIMX334Ctx->MaxIntegrationLine;

    TRACE(IMX334_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pIMX334Ctx->isAfpsRun ? "(AFPS)" : "", pIMX334Ctx->AecMaxIntegrationTime);

    pIMX334Ctx->AecGainIncrement = IMX334_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pIMX334Ctx->AecCurGain = pIMX334Ctx->AecMinGain;
    pIMX334Ctx->AecCurIntegrationTime = 0.0f;
    pIMX334Ctx->OldGain = 0;
    pIMX334Ctx->OldIntegrationTime = 0;

    TRACE(IMX334_INFO, "%s%s: (exit)\n", __func__, pIMX334Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT IMX334_IsiSetupIss(IsiSensorHandle_t handle) 
{
    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    if (!pIMX334Ctx) {
        TRACE(IMX334_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;
    int ret = 0;

    TRACE(IMX334_INFO, "%s (enter)\n", __func__);

    if (pIMX334Ctx->Streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    if (pIMX334Ctx->KernelDriverFlag) {
#ifdef SUBDEV_CHAR
        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_INIT, &(pIMX334Ctx->SensorMode));
        if (ret != 0) {
            TRACE(IMX334_ERROR, "%s:sensor init error!\n", __func__);
            return (RET_FAILURE);
        }

        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_SENSOR_MODE, &(pIMX334Ctx->SensorMode));
        if (ret != 0) {
            TRACE(IMX334_ERROR, "%s:sensor get mode info error!\n", __func__);
            return (RET_FAILURE);
        }

        /*struct vvcam_ae_info_s ae_info;
        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_AE_INFO, &ae_info);
        if (ret != 0) {
            TRACE(IMX334_ERROR, "%s:sensor get ae info error!\n",
                  __func__);
            return (RET_FAILURE);
        }
        pIMX334Ctx->one_line_exp_time  = (double)ae_info.one_line_exp_time_ns / 1000000000;
        pIMX334Ctx->MaxIntegrationLine = ae_info.max_integration_time;
        pIMX334Ctx->MinIntegrationLine = ae_info.min_integration_time;
        pIMX334Ctx->gain_accuracy = ae_info.gain_accuracy;
        pIMX334Ctx->AecMinGain =  (float)(ae_info.min_gain) / ae_info.gain_accuracy;
        pIMX334Ctx->AecMaxGain = (float)(ae_info.max_gain) / ae_info.gain_accuracy;

        pIMX334Ctx->CurrFps = pIMX334Ctx->MaxFps;*/
#endif

#ifdef SUBDEV_V4L2
        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_SENSOR_MODE, &(pIMX334Ctx->SensorMode));
        if (ret != 0) {
            TRACE(IMX334_ERROR, "%s:sensor get mode info error!\n", __func__);
            return (RET_FAILURE);
        }
        if (pIMX334Ctx->SensorMode.hdr_mode != SENSOR_MODE_LINEAR) {
            pIMX334Ctx->enableHdr = true;
        } else {
            pIMX334Ctx->enableHdr = false;
        }

        pIMX334Ctx->one_line_exp_time  = (float) (pIMX334Ctx->SensorMode.ae_info.one_line_exp_time_ns) / 1000000000;
        pIMX334Ctx->MaxIntegrationLine = pIMX334Ctx->SensorMode.ae_info.max_integration_time;
        pIMX334Ctx->MinIntegrationLine = pIMX334Ctx->SensorMode.ae_info.min_integration_time;
        pIMX334Ctx->gain_accuracy      = pIMX334Ctx->SensorMode.ae_info.gain_accuracy;
        pIMX334Ctx->AecMaxGain         = (float)(pIMX334Ctx->SensorMode.ae_info.max_gain) /pIMX334Ctx->gain_accuracy ;
        pIMX334Ctx->AecMinGain         = (float)(pIMX334Ctx->SensorMode.ae_info.min_gain) / pIMX334Ctx->gain_accuracy ;
        pIMX334Ctx->MaxFps             = pIMX334Ctx->SensorMode.fps;
        pIMX334Ctx->CurrFps            = pIMX334Ctx->MaxFps;

#endif

    } else {
        struct vvsensor_sccb_array_s array;
        result = IMX334_IsiGetRegCfgIss(pIMX334Ctx->SensorRegCfgFile, &array);
        if (result != 0) {
            TRACE(IMX334_ERROR, "%s:IMX334_IsiGetRegCfgIss error!\n", __func__);
            return (RET_FAILURE);
        }

        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_WRITE_ARRAY, &array);
        if (ret != 0) {
            TRACE(IMX334_ERROR, "%s:Sensor Write Reg array error!\n", __func__);
            return (RET_FAILURE);
        }

        switch(pIMX334Ctx->SensorMode.index) {
            case 0:
                pIMX334Ctx->one_line_exp_time = 0.00014389;
                pIMX334Ctx->FrameLengthLines = 0xac4;
                pIMX334Ctx->CurFrameLengthLines = pIMX334Ctx->FrameLengthLines;
                pIMX334Ctx->MaxIntegrationLine = pIMX334Ctx->CurFrameLengthLines - 3;
                pIMX334Ctx->MinIntegrationLine = 1;
                pIMX334Ctx->AecMaxGain = 24;
                pIMX334Ctx->AecMinGain = 1;
                break;
            case 1:
                pIMX334Ctx->one_line_exp_time = 0.00014389;
                pIMX334Ctx->FrameLengthLines =  0xac4;
                pIMX334Ctx->CurFrameLengthLines = pIMX334Ctx->FrameLengthLines;
                pIMX334Ctx->MaxIntegrationLine = pIMX334Ctx->CurFrameLengthLines - 3;
                pIMX334Ctx->MinIntegrationLine = 1;
                pIMX334Ctx->AecMaxGain = 24;
                pIMX334Ctx->AecMinGain = 1;
                break;
            default:
                TRACE(IMX334_INFO, "%s:not support sensor mode %d\n", __func__, pIMX334Ctx->SensorMode.index);
                return RET_NOTSUPP;
                break;
        }

        if (pIMX334Ctx->SensorMode.hdr_mode != SENSOR_MODE_LINEAR) {
            pIMX334Ctx->enableHdr = true;
        } else {
            pIMX334Ctx->enableHdr = false;
        }

        pIMX334Ctx->MaxFps  = pIMX334Ctx->SensorMode.fps;
        pIMX334Ctx->MinFps  = 1 * ISI_FPS_QUANTIZE;
        pIMX334Ctx->CurrFps = pIMX334Ctx->MaxFps;
    }

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(IMX334_DEBUG, "%s: IMX334 System-Reset executed\n", __func__);
    osSleep(100);

    result = IMX334_AecSetModeParameters(handle, pIMX334Ctx);
    if (result != RET_SUCCESS) {
        TRACE(IMX334_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pIMX334Ctx->Configured = BOOL_TRUE;
    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT IMX334_IsiCheckConnectionIss(IsiSensorHandle_t handle) {
    RESULT result = RET_SUCCESS;
    uint32_t correct_id = 0x9012;
    uint32_t sensor_id = 0;

    TRACE(IMX334_INFO, "%s (enter)\n", __func__);

    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL || pIMX334Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;

    if (pIMX334Ctx->KernelDriverFlag) {

        int ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_CHIP_ID,
              &sensor_id);
        if (ret != 0) {
            TRACE(IMX334_ERROR, "%s: Read Sensor chip ID Error! \n", __func__);
            return (RET_FAILURE);
        }
    } else {

        result = IMX334_IsiGetRevisionIss(handle, &sensor_id);
        if (result != RET_SUCCESS) {
            TRACE(IMX334_ERROR, "%s: Read Sensor ID Error! \n", __func__);
            return (RET_FAILURE);
        }
    }

    if (correct_id != sensor_id) {
        TRACE(IMX334_ERROR, "%s:ChipID =0x%x sensor_id=%x error! \n", __func__, correct_id, sensor_id);
        return (RET_FAILURE);
    }

    TRACE(IMX334_INFO, "%s ChipID = 0x%08x, sensor_id = 0x%08x, success! \n", __func__, correct_id, sensor_id);
    TRACE(IMX334_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX334_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue) 
{
    RESULT result = RET_SUCCESS;
    uint32_t reg_val;
    uint32_t sensor_id;

    TRACE(IMX334_INFO, "%s (enter)\n", __func__);

    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL || pIMX334Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }
    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;

    if (!pValue)
        return (RET_NULL_POINTER);

    if (pIMX334Ctx->KernelDriverFlag) {
        int ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_CHIP_ID, &sensor_id);
        if (ret != 0) {
            TRACE(IMX334_ERROR, "%s: Read Sensor ID Error! \n", __func__);
            return (RET_FAILURE);
        }
    } else {
        reg_val = 0;
        result = IMX334_IsiReadRegIss(handle, 0x3a04, &reg_val);
        sensor_id = (reg_val & 0xff) << 8;

        reg_val = 0;
        result |= IMX334_IsiReadRegIss(handle, 0x3a05, &reg_val);
        sensor_id |= (reg_val & 0xff);

    }

    *pValue = sensor_id;
    TRACE(IMX334_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX334_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on) 
{
    RESULT result = RET_SUCCESS;
    int ret = 0;
    TRACE(IMX334_INFO, "%s (enter)\n", __func__);

    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL || pIMX334Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }
    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;

    if ((pIMX334Ctx->Configured != BOOL_TRUE) || (pIMX334Ctx->Streaming == on))
        return RET_WRONG_STATE;

    int32_t enable = (uint32_t) on;
    if (pIMX334Ctx->KernelDriverFlag) {
        ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_STREAM, &enable);

    } else {
        ret = IMX334_IsiWriteRegIss(handle, 0x3002, 0x00);
        ret |= IMX334_IsiWriteRegIss(handle, 0x3000, 0x00);
    }

    if (ret != 0) {
        return (RET_FAILURE);
    }

    pIMX334Ctx->Streaming = on;

    TRACE(IMX334_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX334_IsiGetGainLimitsIss(IsiSensorHandle_t handle, float *pMinGain, float *pMaxGain) 
{
    const IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    if (pIMX334Ctx == NULL) {
        TRACE(IMX334_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if ((pMinGain == NULL) || (pMaxGain == NULL)) {
        TRACE(IMX334_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    *pMinGain = pIMX334Ctx->AecMinGain;
    *pMaxGain = pIMX334Ctx->AecMaxGain;

    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);
    return (result);
}

static RESULT IMX334_IsiGetIntegrationTimeLimitsIss(IsiSensorHandle_t handle, float *pMinIntegrationTime, float *pMaxIntegrationTime) 
{
    const IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);
    if (pIMX334Ctx == NULL) {
        TRACE(IMX334_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if ((pMinIntegrationTime == NULL) || (pMaxIntegrationTime == NULL)) {
        TRACE(IMX334_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    *pMinIntegrationTime = pIMX334Ctx->AecMinIntegrationTime;
    *pMaxIntegrationTime = pIMX334Ctx->AecMaxIntegrationTime;

    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT IMX334_IsiGetGainIss(IsiSensorHandle_t handle, float *pSetGain) 
{
    const IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    if (pIMX334Ctx == NULL) {
        TRACE(IMX334_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSetGain == NULL) {
        return (RET_NULL_POINTER);
    }
    *pSetGain = pIMX334Ctx->AecCurGain;

    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX334_IsiGetSEF1GainIss(IsiSensorHandle_t handle, float *pSetGain) 
{
    const IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    if (pIMX334Ctx == NULL) {
        TRACE(IMX334_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSetGain == NULL) {
        return (RET_NULL_POINTER);
    }

    *pSetGain = pIMX334Ctx->AecCurGainSEF1;

    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);

    return (result);
}

RESULT IMX334_IsiGetGainIncrementIss(IsiSensorHandle_t handle, float *pIncr) 
{
    const IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    if (pIMX334Ctx == NULL) {
        TRACE(IMX334_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pIncr == NULL)
        return (RET_NULL_POINTER);

    *pIncr = pIMX334Ctx->AecGainIncrement;

    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);

    return (result);
}

RESULT IMX334_IsiSetGainIss(IsiSensorHandle_t handle, float NewGain, float *pSetGain, float *hdr_ratio) 
{

    RESULT result = RET_SUCCESS;

    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL || pIMX334Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;

    if (pIMX334Ctx->KernelDriverFlag) {
        uint32_t SensorGain = 0;
        SensorGain = NewGain * pIMX334Ctx->gain_accuracy;

    /*#ifdef SUBDEV_CHAR
        if (pIMX334Ctx->enableHdr == true) {
            uint32_t SensorHdrRatio = (uint32_t)*hdr_ratio;
            ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_HDR_RADIO, &SensorHdrRatio);
        }
    #endif*/
        int ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_GAIN, &SensorGain);
        if (ret != 0) {
            TRACE(IMX334_ERROR, "%s: set sensor gain error\n", __func__);
            return RET_FAILURE;
        }
    } else {
        uint32_t Gain = 0;
        Gain = (uint32_t)(20*log10(NewGain)*(10/3)) ;
        result = IMX334_IsiWriteRegIss(handle, 0x3001, 0x01);
        result |= IMX334_IsiWriteRegIss(handle, 0x30e8,(Gain & 0x00ff));
        result |= IMX334_IsiWriteRegIss(handle, 0x30e9,(Gain & 0x0700)>>8);
        result |= IMX334_IsiWriteRegIss(handle, 0x3001, 0x00);
        pIMX334Ctx->OldGain = NewGain;
    }


    pIMX334Ctx->AecCurGain = ((float)(NewGain));

    *pSetGain = pIMX334Ctx->AecCurGain;
    TRACE(IMX334_DEBUG, "%s: g=%f\n", __func__, *pSetGain);
    return (result);
}
RESULT IMX334_IsiSetSEF1GainIss(IsiSensorHandle_t handle, float NewIntegrationTime,float NewGain, float *pSetGain, const float *hdr_ratio) 
{

    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    if (!pIMX334Ctx) {
        TRACE(IMX334_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;

    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    if (!pSetGain || !hdr_ratio)
        return (RET_NULL_POINTER);

    if (pIMX334Ctx->KernelDriverFlag) {
        uint32_t SensorGain = 0;
        SensorGain = NewGain * pIMX334Ctx->gain_accuracy;
        ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_SGAIN, &SensorGain);
    } else {
            uint32_t Gain = 0;
            Gain = (uint32_t)(20*log10(NewGain)*(10/3)) ;
            result = IMX334_IsiWriteRegIss(handle, 0x3001, 0x01);
            result |= IMX334_IsiWriteRegIss(handle, 0x30EA, (Gain & 0x00FF));
            result |= IMX334_IsiWriteRegIss(handle, 0x30EB, (Gain & 0x0700)>>8);
            result |= IMX334_IsiWriteRegIss(handle, 0x3001, 0x00);
            pIMX334Ctx->OldGainSEF1 = NewGain;
    }

    pIMX334Ctx->AecCurGainSEF1 = NewGain;
    *pSetGain = pIMX334Ctx->AecCurGainSEF1;

    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX334_IsiGetIntegrationTimeIss(IsiSensorHandle_t handle, float *pSetIntegrationTime)
{
    const IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    if (!pIMX334Ctx) {
        TRACE(IMX334_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSetIntegrationTime)
        return (RET_NULL_POINTER);
    *pSetIntegrationTime = pIMX334Ctx->AecCurIntegrationTime;
    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX334_IsiGetSEF1IntegrationTimeIss(IsiSensorHandle_t handle, float *pSetIntegrationTime)
{
    const IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    if (!pIMX334Ctx) {
        TRACE(IMX334_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    if (!pSetIntegrationTime)
        return (RET_NULL_POINTER);

    *pSetIntegrationTime = pIMX334Ctx->AecCurIntegrationTimeSEF1;
    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX334_IsiGetIntegrationTimeIncrementIss(IsiSensorHandle_t handle, float *pIncr)
{
    const IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    if (!pIMX334Ctx) {
        TRACE(IMX334_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pIncr)
        return (RET_NULL_POINTER);

    //_smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application)
    *pIncr = pIMX334Ctx->AecIntegrationTimeIncrement;
    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX334_IsiSetIntegrationTimeIss(IsiSensorHandle_t handle, float NewIntegrationTime, float *pSetIntegrationTime, uint8_t * pNumberOfFramesToSkip, float *hdr_ratio)
{
    RESULT result = RET_SUCCESS;

    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (!pIMX334Ctx) {
        TRACE(IMX334_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;

    uint32_t exp = 0;

    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    if (!pSetIntegrationTime || !pNumberOfFramesToSkip) {
        TRACE(IMX334_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __func__);
        return (RET_NULL_POINTER);
    }

    exp = NewIntegrationTime / pIMX334Ctx->one_line_exp_time;

    TRACE(IMX334_DEBUG, "%s: set AEC_PK_EXPO=0x%05x\n", __func__, exp);

    if (NewIntegrationTime != pIMX334Ctx->OldIntegrationTime) {
        if (pIMX334Ctx->KernelDriverFlag) {
            ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_EXP, &exp);
        } else {

            exp = 2200 - exp +1;
            exp = exp > 5?exp:5;
            exp = exp <IMX334_VMAX - 1 ? exp:IMX334_VMAX - 1;
            result = IMX334_IsiWriteRegIss(handle, 0x3001, 0x01);
            result |= IMX334_IsiWriteRegIss(handle, 0x3058, (exp & 0x0000FF));
            result |= IMX334_IsiWriteRegIss(handle, 0x3059, (exp & 0x00FF00)>>8);
            result |= IMX334_IsiWriteRegIss(handle, 0x305a, (exp & 0x070000)>>16);
            result |= IMX334_IsiWriteRegIss(handle, 0x3001, 0x00);
        }

        pIMX334Ctx->OldIntegrationTime = NewIntegrationTime ;
        pIMX334Ctx->AecCurIntegrationTime = NewIntegrationTime;

        *pNumberOfFramesToSkip = 1U;
    } else {
        *pNumberOfFramesToSkip = 0U;    //no frame skip
    }

    *pSetIntegrationTime = pIMX334Ctx->AecCurIntegrationTime;
    TRACE(IMX334_DEBUG, "%s: Ti=%f\n", __func__, *pSetIntegrationTime);
    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX334_IsiSetSEF1IntegrationTimeIss(IsiSensorHandle_t handle, float NewIntegrationTime,
                                float *pSetIntegrationTimeSEF1, uint8_t * pNumberOfFramesToSkip, float *hdr_ratio)
{
    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (!pIMX334Ctx) {
        TRACE(IMX334_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;
    RESULT result = RET_SUCCESS;
    uint32_t exp = 0;

    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    if (!pSetIntegrationTimeSEF1 || !pNumberOfFramesToSkip) {
        TRACE(IMX334_ERROR,"%s: Invalid parameter (NULL pointer detected)\n", __func__);
        return (RET_NULL_POINTER);
    }
    TRACE(IMX334_INFO, "%s:  maxIntegrationTime-=%f minIntegrationTime = %f\n", __func__,
          pIMX334Ctx->AecMaxIntegrationTime, pIMX334Ctx->AecMinIntegrationTime);


    exp = NewIntegrationTime / pIMX334Ctx->one_line_exp_time;

    if (NewIntegrationTime != pIMX334Ctx->OldIntegrationTimeSEF1) {
        if (pIMX334Ctx->KernelDriverFlag) {
            ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_SEXP, &exp);
        } else {
            exp = 2200- exp +1;
            exp = exp > 5 ? exp : 5;
            exp = exp < IMX334_VMAX - 1 ? exp : IMX334_VMAX - 1;
            result = IMX334_IsiWriteRegIss(handle, 0x3001, 0x01);
            result |= IMX334_IsiWriteRegIss(handle, 0x305c,(exp & 0x0000ff));
            result |= IMX334_IsiWriteRegIss(handle, 0x305D,(exp & 0x00ff00)>>8);
            result |= IMX334_IsiWriteRegIss(handle, 0x305e,(exp & 0x070000)>>16);
            result |= IMX334_IsiWriteRegIss(handle, 0x3001, 0x00);
        }

        pIMX334Ctx->OldIntegrationTimeSEF1 = NewIntegrationTime;
        pIMX334Ctx->AecCurIntegrationTimeSEF1 = NewIntegrationTime;
        *pNumberOfFramesToSkip = 1U;
    } else {
        *pNumberOfFramesToSkip = 0U;
    }

    *pSetIntegrationTimeSEF1 = pIMX334Ctx->AecCurIntegrationTimeSEF1;

    TRACE(IMX334_DEBUG, "%s: NewIntegrationTime=%f\n", __func__, NewIntegrationTime);
    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX334_IsiExposureControlIss(IsiSensorHandle_t handle, float NewGain, float NewIntegrationTime,
                     uint8_t * pNumberOfFramesToSkip,float *pSetGain, float *pSetIntegrationTime, float *hdr_ratio)
{
    const IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;

    RESULT result = RET_SUCCESS;
    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    if (pIMX334Ctx == NULL) {
        TRACE(IMX334_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if ((pNumberOfFramesToSkip == NULL) || (pSetGain == NULL)
        || (pSetIntegrationTime == NULL)) {
        TRACE(IMX334_ERROR, "%s: Invalid parameter (NULL pointer detected)\n", __func__);
        return (RET_NULL_POINTER);
    }


    if (pIMX334Ctx->enableHdr) {
        result = IMX334_IsiSetSEF1IntegrationTimeIss(handle, NewIntegrationTime,pSetIntegrationTime,pNumberOfFramesToSkip,hdr_ratio);
        result |= IMX334_IsiSetSEF1GainIss(handle, NewIntegrationTime, NewGain,pSetGain, hdr_ratio);
    }
    result |= IMX334_IsiSetIntegrationTimeIss(handle, NewIntegrationTime,pSetIntegrationTime,pNumberOfFramesToSkip, hdr_ratio);
    result |= IMX334_IsiSetGainIss(handle, NewGain,  pSetGain,  hdr_ratio);
    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT IMX334_IsiGetCurrentExposureIss(IsiSensorHandle_t handle, float *pSetGain, float *pSetIntegrationTime) 
{
    const IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    if (pIMX334Ctx == NULL) {
        TRACE(IMX334_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if ((pSetGain == NULL) || (pSetIntegrationTime == NULL))
        return (RET_NULL_POINTER);

    *pSetGain = pIMX334Ctx->AecCurGain;
    *pSetIntegrationTime = pIMX334Ctx->AecCurIntegrationTime;

    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX334_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t *pFps)
{
    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    if (pIMX334Ctx == NULL) {
        TRACE(IMX334_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;

    if (pIMX334Ctx->KernelDriverFlag) {
        ioctl(pHalCtx->sensor_fd, VVSENSORIOC_G_FPS, pFps);
        pIMX334Ctx->CurrFps = *pFps;
    }

    *pFps = pIMX334Ctx->CurrFps;

    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX334_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t Fps)
{
    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);
    if (pIMX334Ctx == NULL) {
        TRACE(IMX334_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    HalContext_t *pHalCtx = (HalContext_t *) pIMX334Ctx->IsiCtx.HalHandle;

    if (Fps > pIMX334Ctx->MaxFps) {
        TRACE(IMX334_ERROR, "%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, Fps, pIMX334Ctx->MaxFps, 
        pIMX334Ctx->MinFps, pIMX334Ctx->MaxFps);
        Fps = pIMX334Ctx->MaxFps;
    }
    if (Fps < pIMX334Ctx->MinFps) {
        TRACE(IMX334_ERROR, "%s: set fps(%d) out of range, correct to %d (%d, %d)\n",
              __func__, Fps, pIMX334Ctx->MinFps, pIMX334Ctx->MinFps, pIMX334Ctx->MaxFps);
        Fps = pIMX334Ctx->MinFps;
    }
    if (pIMX334Ctx->KernelDriverFlag) {
        int ret = ioctl(pHalCtx->sensor_fd, VVSENSORIOC_S_FPS, &Fps);
        if (ret != 0) {
            TRACE(IMX334_ERROR, "%s: set sensor fps=%d error\n", __func__);
            return (RET_FAILURE);
        }
    } else {
        uint16_t FrameLengthLines;
        FrameLengthLines = pIMX334Ctx->FrameLengthLines * pIMX334Ctx->MaxFps / Fps;
        result = IMX334_IsiWriteRegIss(handle, 0x30b2,(FrameLengthLines >> 8) & 0xff);
        result |= IMX334_IsiWriteRegIss(handle, 0x30b3, FrameLengthLines & 0xff);
        if (result != RET_SUCCESS) {
            TRACE(IMX334_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
            return (RET_FAILURE);
        }
        pIMX334Ctx->CurrFps = Fps;
        pIMX334Ctx->CurFrameLengthLines = FrameLengthLines;
        pIMX334Ctx->MaxIntegrationLine  = pIMX334Ctx->CurFrameLengthLines - 3;
        pIMX334Ctx->AecMaxIntegrationTime =
        pIMX334Ctx->MaxIntegrationLine * pIMX334Ctx->one_line_exp_time;
    }

    TRACE(IMX334_INFO, "%s: set sensor fps = %d\n", __func__, pIMX334Ctx->CurrFps);

    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX334_IsiGetAutoFpsInfoIss(IsiSensorHandle_t handle, IsiAutoFps_t *pAutoFpsInfo)
{
    RESULT result = RET_SUCCESS;
    return (result);
}

RESULT IMX334_IsiGetStartEvIss(IsiSensorHandle_t handle, uint64_t *pStartEv)
{
    RESULT result = RET_SUCCESS;
    return (result);
}

RESULT IMX334_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL || pIMX334Ctx->IsiCtx.HalHandle == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    pIspStatus->useSensorAWB = false;

    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT IMX334_IsiSetTpgIss(IsiSensorHandle_t handle, IsiTpg_t Tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL || pIMX334Ctx->IsiCtx.HalHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pIMX334Ctx->Configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (Tpg.enable == 0) {
        result = IMX334_IsiWriteRegIss(handle, 0x3253, 0x00);
    } else {
        result = IMX334_IsiWriteRegIss(handle, 0x3253, 0x80);
    }

    pIMX334Ctx->TestPattern = Tpg.enable;

    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX334_IsiGetTpgIss(IsiSensorHandle_t handle, IsiTpg_t *Tpg)
{
    RESULT result = RET_SUCCESS;
    uint32_t value = 0;
    TRACE(IMX334_INFO, "%s: (enter)\n", __func__);

    IMX334_Context_t *pIMX334Ctx = (IMX334_Context_t *) handle;
    if (pIMX334Ctx == NULL || pIMX334Ctx->IsiCtx.HalHandle == NULL || Tpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pIMX334Ctx->Configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!IMX334_IsiReadRegIss(handle, 0x5081,&value)) {
        Tpg->enable = ((value & 0x80) != 0) ? 1 : 0;
        if(Tpg->enable) {
           Tpg->pattern = (0xff & value);
        }
      pIMX334Ctx->TestPattern = Tpg->enable;
    }

    TRACE(IMX334_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX334_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "IMX334";
    TRACE(IMX334_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = IMX334_IsiCreateIss;
        pIsiSensor->pIsiReleaseIss                      = IMX334_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = IMX334_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = IMX334_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = IMX334_IsiGetModeIss;
        pIsiSensor->pIsiSetModeIss                      = IMX334_IsiSetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = IMX334_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = IMX334_IsiGetCapsIss;
        pIsiSensor->pIsiSetupIss                        = IMX334_IsiSetupIss;
        pIsiSensor->pIsiCheckConnectionIss              = IMX334_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = IMX334_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = IMX334_IsiSetStreamingIss;

        /* AEC functions */
        pIsiSensor->pIsiExposureControlIss              = IMX334_IsiExposureControlIss;
        pIsiSensor->pIsiGetGainLimitsIss                = IMX334_IsiGetGainLimitsIss;
        pIsiSensor->pIsiGetIntegrationTimeLimitsIss     = IMX334_IsiGetIntegrationTimeLimitsIss;
        pIsiSensor->pIsiGetVSGainIss                    = IMX334_IsiGetSEF1GainIss;
        pIsiSensor->pIsiGetGainIss                      = IMX334_IsiGetGainIss;
        pIsiSensor->pIsiGetGainIncrementIss             = IMX334_IsiGetGainIncrementIss;
        pIsiSensor->pIsiSetGainIss                      = IMX334_IsiSetGainIss;
        pIsiSensor->pIsiGetIntegrationTimeIss           = IMX334_IsiGetIntegrationTimeIss;
        pIsiSensor->pIsiGetVSIntegrationTimeIss         = IMX334_IsiGetSEF1IntegrationTimeIss;
        pIsiSensor->pIsiGetIntegrationTimeIncrementIss  = IMX334_IsiGetIntegrationTimeIncrementIss;
        pIsiSensor->pIsiSetIntegrationTimeIss           = IMX334_IsiSetIntegrationTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = IMX334_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = IMX334_IsiSetFpsIss;
        pIsiSensor->pIsiGetAutoFpsInfoIss               = IMX334_IsiGetAutoFpsInfoIss;
        pIsiSensor->pIsiGetStartEvIss                   = IMX334_IsiGetStartEvIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = IMX334_IsiGetIspStatusIss;
        //pIsiSensor->pIsiSetBlcIss                       = IMX334_IsiSetBlcIss;
        //pIsiSensor->pIsiSetWBIss                        = IMX334_IsiSetWBIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetPowerIss                     = IMX334_IsiSetPowerIss;
        pIsiSensor->pIsiSetTpgIss                       = IMX334_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = IMX334_IsiGetTpgIss;
        //pIsiSensor->pIsiGetExpandCurveIss               = IMX334_IsiGetExpandCurveIss;
        //pIsiSensor->pIsiGetCompressCurveIss             = IMX334_IsiGetCompressCurveIss;
        //pIsiSensor->pIsiExtendFuncIss                   = IMX334_IsiExtendFuncIss;
        //pIsiSensor->pIsiGetOtpDataIss                   = IMX334_IsiGetOtpDataIss;

        /* AF */
        pIsiSensor->pIsiFocusCreateIss                  = NULL;
        pIsiSensor->pIsiFocusReleaseIss                 = NULL;
        pIsiSensor->pIsiFocusGetCalibrateIss            = NULL;
        pIsiSensor->pIsiFocusSetIss                     = NULL;
        pIsiSensor->pIsiFocusGetIss                     = NULL;

    } else {
        result = RET_NULL_POINTER;
    }

    TRACE(IMX334_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t IMX334_IsiCamDrvConfig = {
    .cameraDriverID        = 0x9012,
    .pIsiGetSensorIss      = IMX334_IsiGetSensorIss,
};
