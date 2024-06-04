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
#include "isi_vvsensor.h"

#include "OV2775_priv.h"

CREATE_TRACER(OV2775_INFO , "OV2775: ", INFO,    1);
CREATE_TRACER(OV2775_WARN , "OV2775: ", WARNING, 1);
CREATE_TRACER(OV2775_ERROR, "OV2775: ", ERROR,   1);
CREATE_TRACER(OV2775_DEBUG,     "OV2775: ", INFO, 1);
CREATE_TRACER(OV2775_REG_INFO , "OV2775: ", INFO, 1);
CREATE_TRACER(OV2775_REG_DEBUG, "OV2775: ", INFO, 1);

#ifdef SUBDEV_V4L2
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#undef TRACE
#define TRACE(x, ...)
#endif

#define OV2775_MIN_GAIN_STEP    (1.0f/256.0f)  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain) */

/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pov2775_mode_info[] = {
    {
        .index     = 0,
        .size      = {
            .boundsWidth  = 1920,
            .boundsHeight = 1080,
            .top           = 0,
            .left          = 0,
            .width         = 1920,
            .height        = 1080,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps       = 30 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 12,
        .bayerPattern = ISI_BPAT_GBRG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 1,
        .size      = {
            .boundsWidth  = 1920,
            .boundsHeight = 1080,
            .top           = 0,
            .left          = 0,
            .width         = 1920,
            .height        = 1080,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps      = 30 * ISI_FPS_QUANTIZE,
        .hdrMode = ISI_SENSOR_MODE_HDR_STITCH,
        .stitchingMode = ISI_SENSOR_STITCHING_DUAL_DCG,
        .bitWidth = 12,
        .bayerPattern = ISI_BPAT_GBRG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 2,
        .size      = {
            .boundsWidth  = 1920,
            .boundsHeight = 1080,
            .top           = 0,
            .left          = 0,
            .width         = 1920,
            .height        = 1080,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps      = 30 * ISI_FPS_QUANTIZE,
        .hdrMode = ISI_SENSOR_MODE_HDR_NATIVE,
        .nativeMode = ISI_SENSOR_NATIVE_DCG,
        .bitWidth = 12,
        .compress.enable = 1,
        .compress.xBit  = 16,
        .compress.yBit  = 12,
        .bayerPattern = ISI_BPAT_GBRG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
};

static RESULT OV2775_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    int32_t ret = 0;
    TRACE(OV2775_INFO, "%s (enter)\n", __func__);

    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL || pOV2775Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    ret = HalReadI2CReg(pOV2775Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (ret != 0) {
        TRACE(OV2775_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV2775_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV2775_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    int ret = 0;
    TRACE(OV2775_INFO, "%s (enter)\n", __func__);

    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL || pOV2775Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    ret = HalWriteI2CReg(pOV2775Ctx->isiCtx.halI2cHandle, addr, value);
    if (ret != 0) {
        TRACE(OV2775_ERROR, "%s: hal write sensor register error!\n",__func__);
        return (RET_FAILURE);
    }

    TRACE(OV2775_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV2775_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(OV2775_INFO, "%s (enter)\n", __func__);
    const OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pOV2775Ctx->sensorMode), sizeof(pOV2775Ctx->sensorMode));

    TRACE(OV2775_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT OV2775_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    const OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pov2775_mode_info)/sizeof(pov2775_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pov2775_mode_info)/sizeof(pov2775_mode_info[0])); i++) {
        if (pov2775_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pov2775_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(OV2775_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT OV2775_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(OV2775_INFO, "%s (enter)\n", __func__);

    if (pOV2775Ctx == NULL)
        return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pOV2775Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pOV2775Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pOV2775Ctx->sensorMode.size.width;
    pCaps->resolution.height = pOV2775Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(OV2775_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV2775_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s (enter)\n", __func__);

    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) malloc(sizeof(OV2775_Context_t));
    if (!pOV2775Ctx) {
        TRACE(OV2775_ERROR, "%s: Can't allocate ov2775 context\n",__func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pOV2775Ctx, 0, sizeof(OV2775_Context_t));

    pOV2775Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pOV2775Ctx->groupHold          = BOOL_FALSE;
    pOV2775Ctx->oldGain            = 0;
    pOV2775Ctx->oldIntegrationTime = 0;
    pOV2775Ctx->configured         = BOOL_FALSE;
    pOV2775Ctx->streaming          = BOOL_FALSE;
    pOV2775Ctx->testPattern        = BOOL_FALSE;
    pOV2775Ctx->isAfpsRun          = BOOL_FALSE;
    pOV2775Ctx->sensorMode.index   = 0;

    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x6c;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pOV2775Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pOV2775Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(OV2775_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pOV2775Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pOV2775Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pOV2775Ctx;

    TRACE(OV2775_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV2775_AecSetModeParameters(IsiSensorHandle_t handle, OV2775_Context_t * pOV2775Ctx)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s%s: (enter)\n", __func__,pOV2775Ctx->isAfpsRun ? "(AFPS)" : "");
    uint16_t value = 0, regVal = 0, again = 0;
    float dgain = 0;

    pOV2775Ctx->aecIntegrationTimeIncrement = pOV2775Ctx->oneLineExpTime;
    pOV2775Ctx->aecMinIntegrationTime       = pOV2775Ctx->oneLineExpTime * pOV2775Ctx->minIntegrationLine;
    //pOV2775Ctx->aecMaxIntegrationTime       = pOV2775Ctx->oneLineExpTime * pOV2775Ctx->maxIntegrationLine;
    pOV2775Ctx->aecMaxIntegrationTime        = 0.04f;
    pOV2775Ctx->aecMinVSIntegrationTime       = pOV2775Ctx->oneLineExpTime * pOV2775Ctx->minVSIntegrationLine;
    pOV2775Ctx->aecMaxVSIntegrationTime       = pOV2775Ctx->oneLineExpTime * pOV2775Ctx->maxVSIntegrationLine;

    TRACE(OV2775_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pOV2775Ctx->isAfpsRun ? "(AFPS)" : "",pOV2775Ctx->aecMaxIntegrationTime);

    pOV2775Ctx->aecGainIncrement = OV2775_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings

    OV2775_IsiReadRegIss(handle, 0x30b6, &value);
    regVal = value << 8;
    OV2775_IsiReadRegIss(handle, 0x30b7, &value);
    regVal = regVal | (value & 0xFF);
    pOV2775Ctx->aecCurIntegrationTime = regVal * pOV2775Ctx->oneLineExpTime;
    pOV2775Ctx->aecCurLongIntegrationTime = regVal * pOV2775Ctx->oneLineExpTime;

    if(pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR){
        OV2775_IsiReadRegIss(handle, 0x315a, &value);
        regVal = (value & 0xff) << 8;
        OV2775_IsiReadRegIss(handle, 0x315b, &value);
        regVal = regVal | (value & 0xFF);
        dgain = regVal/256.0f;

        OV2775_IsiReadRegIss(handle, 0x30bb, &value);
        regVal = value & 0x03;
        again = 1 << regVal;
        pOV2775Ctx->aecCurGain = again * dgain;
    } else {
        /* HCG analog gain and digital gain*/
        OV2775_IsiReadRegIss(handle, 0x315a, &value);
        regVal = (value & 0xff) << 8;
        OV2775_IsiReadRegIss(handle, 0x315b, &value);
        regVal = regVal | (value & 0xFF);
        dgain = regVal/256.0f;

        OV2775_IsiReadRegIss(handle, 0x30bb, &value);
        regVal = value & 0x03;
        again = 1 << regVal;
        pOV2775Ctx->aecCurGain = again * dgain;//dpf limit
        pOV2775Ctx->aecCurLongGain = again * dgain;
    }
    pOV2775Ctx->oldGain               = 0;
    pOV2775Ctx->oldIntegrationTime    = 0;

    TRACE(OV2775_INFO, "%s%s: (exit)\n", __func__,pOV2775Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT OV2775_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OV2775_INFO, "%s (enter)\n", __func__);

    if (!pOV2775Ctx) {
        TRACE(OV2775_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOV2775Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pOV2775Ctx->sensorMode.index   = mode;

    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pov2775_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (pov2775_mode_info[i].index == pOV2775Ctx->sensorMode.index) { 
            SensorDefaultMode = &(pov2775_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(OV2775_mipi4lane_1080p_init) / sizeof(OV2775_mipi4lane_1080p_init[0]); i++) {
                    OV2775_IsiWriteRegIss(handle, OV2775_mipi4lane_1080p_init[i][0], OV2775_mipi4lane_1080p_init[i][1]);
                }
                break;
            case 1: //stitch 3dol mode
                for (int i = 0; i<sizeof(OV2775_mipi4lane_1080p_3dol_init) / sizeof(OV2775_mipi4lane_1080p_3dol_init[0]); i++) {
                    OV2775_IsiWriteRegIss(handle, OV2775_mipi4lane_1080p_3dol_init[i][0], OV2775_mipi4lane_1080p_3dol_init[i][1]);
                }
                break;
            case 2: //native 2dol mode
                for (int i = 0; i<sizeof(OV2775_mipi4lane_1080p_native2dol_init) / sizeof(OV2775_mipi4lane_1080p_native2dol_init[0]); i++) {
                    OV2775_IsiWriteRegIss(handle, OV2775_mipi4lane_1080p_native2dol_init[i][0], OV2775_mipi4lane_1080p_native2dol_init[i][1]);
                }
                break;
            default:
                TRACE(OV2775_INFO, "%s:not support sensor mode %d\n", __func__,pOV2775Ctx->sensorMode.index);
                return RET_NOTSUPP;
                break;
    }

        memcpy(&(pOV2775Ctx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));
    } else {
        TRACE(OV2775_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pOV2775Ctx->sensorMode.index) {
        case 0:
            pOV2775Ctx->oneLineExpTime      = 0.0000295833;
            pOV2775Ctx->frameLengthLines    = 0x466;
            pOV2775Ctx->curFrameLengthLines = pOV2775Ctx->frameLengthLines;
            pOV2775Ctx->maxIntegrationLine  = pOV2775Ctx->curFrameLengthLines - 2;
            pOV2775Ctx->minIntegrationLine  = 1;
            pOV2775Ctx->aecMaxGain          = 320;
            pOV2775Ctx->aecMinGain          = 3.0;
            pOV2775Ctx->aGain.min           = 3.0;
            pOV2775Ctx->aGain.max           = 320;
            pOV2775Ctx->aGain.step          = (1.0f/256.0f);
            pOV2775Ctx->dGain.min           = 1.0;
            pOV2775Ctx->dGain.max           = 1.0;
            pOV2775Ctx->dGain.step          = (1.0f/256.0f);

            break;
        case 1:
            pOV2775Ctx->oneLineExpTime      = 0.000069607;
            pOV2775Ctx->frameLengthLines    = 0x466;
            pOV2775Ctx->curFrameLengthLines = pOV2775Ctx->frameLengthLines;
            pOV2775Ctx->maxIntegrationLine  = pOV2775Ctx->curFrameLengthLines - 2;
            pOV2775Ctx->minIntegrationLine  = 1;
            pOV2775Ctx->aecMaxGain          = 320;
            pOV2775Ctx->aecMinGain          = 3.0;
            pOV2775Ctx->maxVSIntegrationLine  = 40;
            pOV2775Ctx->minVSIntegrationLine  = 1;
            pOV2775Ctx->aGain.min           = 3.0;
            pOV2775Ctx->aGain.max           = 320;
            pOV2775Ctx->aGain.step          = (1.0f/256.0f);
            pOV2775Ctx->dGain.min           = 1.0;
            pOV2775Ctx->dGain.max           = 1.0;
            pOV2775Ctx->dGain.step          = (1.0f/256.0f);
            pOV2775Ctx->aVSGain.min         = 3.0;
            pOV2775Ctx->aVSGain.max         = 320;
            pOV2775Ctx->aVSGain.step        = (1.0f/256.0f);
            pOV2775Ctx->dVSGain.min         = 1.0;
            pOV2775Ctx->dVSGain.max         = 1.0;
            pOV2775Ctx->dVSGain.step        = (1.0f/256.0f);
            break;
        case 2:
            pOV2775Ctx->oneLineExpTime      = 0.000069607;
            pOV2775Ctx->frameLengthLines    = 0x466;
            pOV2775Ctx->curFrameLengthLines = pOV2775Ctx->frameLengthLines;
            pOV2775Ctx->maxIntegrationLine  = pOV2775Ctx->curFrameLengthLines - 4;
            pOV2775Ctx->minIntegrationLine  = 1;
            pOV2775Ctx->aecMaxGain          = 24;
            pOV2775Ctx->aecMinGain          = 3;
            pOV2775Ctx->aGain.min           = 3.0;
            pOV2775Ctx->aGain.max           = 24;
            pOV2775Ctx->aGain.step          = (1.0f/256.0f);
            pOV2775Ctx->dGain.min           = 1.0;
            pOV2775Ctx->dGain.max           = 1.0;
            pOV2775Ctx->dGain.step          = (1.0f/256.0f);
            break;
        default:
            return (RET_NOTAVAILABLE);
            break;
    }

    pOV2775Ctx->maxFps  = pOV2775Ctx->sensorMode.fps;
    pOV2775Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pOV2775Ctx->currFps = pOV2775Ctx->maxFps;

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(OV2775_DEBUG, "%s: OV2775 System-Reset executed\n", __func__);
    osSleep(100);

    result = OV2775_AecSetModeParameters(handle, pOV2775Ctx);
    if (result != RET_SUCCESS) {
        TRACE(OV2775_ERROR, "%s: SetupOutputWindow failed.\n",__func__);
        return (result);
    }

    pOV2775Ctx->configured = BOOL_TRUE;
    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT OV2775_IsiCloseIss(IsiSensorHandle_t handle)
{
    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s (enter)\n", __func__);

    if (pOV2775Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)OV2775_IsiSetStreamingIss(pOV2775Ctx, BOOL_FALSE);

    TRACE(OV2775_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV2775_IsiReleaseIss(IsiSensorHandle_t handle)
{
    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s (enter)\n", __func__);

    if (pOV2775Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pOV2775Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pOV2775Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pOV2775Ctx, 0, sizeof(OV2775_Context_t));
    free(pOV2775Ctx);
    TRACE(OV2775_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV2775_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;
    uint32_t correctId = 0x2770;
    uint32_t sensorId = 0;
    TRACE(OV2775_INFO, "%s (enter)\n", __func__);

    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL || pOV2775Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = OV2775_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(OV2775_ERROR, "%s: Read Sensor ID Error! \n",__func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(OV2775_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n", __func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(OV2775_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__,correctId, sensorId);
    TRACE(OV2775_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV2775_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t * pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(OV2775_INFO, "%s (enter)\n", __func__);

    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL || pOV2775Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (!pValue) return (RET_NULL_POINTER);

    regVal = 0;
    result = OV2775_IsiReadRegIss(handle, 0x300a, &regVal);
    sensorId = (regVal & 0xff) << 8;

    regVal = 0;
    result |= OV2775_IsiReadRegIss(handle, 0x300b, &regVal);
    sensorId |= (regVal & 0xff);

    *pValue = sensorId;
    TRACE(OV2775_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV2775_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;
    int ret = 0;
    TRACE(OV2775_INFO, "%s (enter)\n", __func__);

    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL || pOV2775Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV2775Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    ret = OV2775_IsiWriteRegIss(handle, 0x3012, on);

    if (ret != 0) {
        return (RET_FAILURE);
    }

    pOV2775Ctx->streaming = on;

    TRACE(OV2775_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV2775_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);

    if (pOV2775Ctx == NULL) {
        TRACE(OV2775_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(OV2775_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    /* linear and native2dol*/
    pAeBaseInfo->gain.min        = pOV2775Ctx->aecMinGain;
    pAeBaseInfo->gain.max        = pOV2775Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pOV2775Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pOV2775Ctx->aecMaxIntegrationTime;

    pAeBaseInfo->aGain           = pOV2775Ctx->aGain;
    pAeBaseInfo->dGain           = pOV2775Ctx->dGain;

    pAeBaseInfo->aecCurGain      = pOV2775Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime   = pOV2775Ctx->aecCurIntegrationTime;

    /* stitching 3dol */
    pAeBaseInfo->longGain.min        = pOV2775Ctx->aecMinGain;
    pAeBaseInfo->longGain.max        = pOV2775Ctx->aecMaxGain;
    pAeBaseInfo->longIntTime.min     = pOV2775Ctx->aecMinIntegrationTime;
    pAeBaseInfo->longIntTime.max     = pOV2775Ctx->aecMaxIntegrationTime;

    pAeBaseInfo->shortGain.min       = pOV2775Ctx->aecMinGain;
    pAeBaseInfo->shortGain.max       = pOV2775Ctx->aecMaxGain;
    pAeBaseInfo->shortIntTime.min    = pOV2775Ctx->aecMinIntegrationTime;
    pAeBaseInfo->shortIntTime.max    = pOV2775Ctx->aecMaxIntegrationTime;

    pAeBaseInfo->vsGain.min          = pOV2775Ctx->aecMinGain;
    pAeBaseInfo->vsGain.max          = pOV2775Ctx->aecMaxGain;
    pAeBaseInfo->vsIntTime.min       = pOV2775Ctx->aecMinVSIntegrationTime;
    pAeBaseInfo->vsIntTime.max       = pOV2775Ctx->aecMaxVSIntegrationTime;

    pAeBaseInfo->aLongGain           = pOV2775Ctx->aGain;
    pAeBaseInfo->dLongGain           = pOV2775Ctx->dGain;
    pAeBaseInfo->aShortGain          = pOV2775Ctx->aGain;
    pAeBaseInfo->dShortGain          = pOV2775Ctx->dGain;
    pAeBaseInfo->aVSGain             = pOV2775Ctx->aVSGain;
    pAeBaseInfo->dVSGain             = pOV2775Ctx->dVSGain;

    pAeBaseInfo->curGain.gain[0] = pOV2775Ctx->aecCurLongGain;
    pAeBaseInfo->curGain.gain[1] = pOV2775Ctx->aecCurGain;
    pAeBaseInfo->curGain.gain[2] = pOV2775Ctx->aecCurVSGain;
    pAeBaseInfo->curGain.gain[3] = 0;
    pAeBaseInfo->curIntTime.intTime[0] = pOV2775Ctx->aecCurLongIntegrationTime;
    pAeBaseInfo->curIntTime.intTime[1] = pOV2775Ctx->aecCurIntegrationTime;
    pAeBaseInfo->curIntTime.intTime[2] = pOV2775Ctx->aecCurVSIntegrationTime;
    pAeBaseInfo->curIntTime.intTime[3] = 0;

    pAeBaseInfo->aecGainStep    = pOV2775Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep = pOV2775Ctx->aecIntegrationTimeIncrement;
    pAeBaseInfo->stitchingMode  = pOV2775Ctx->sensorMode.stitchingMode;
    pAeBaseInfo->nativeMode     = pOV2775Ctx->sensorMode.nativeMode;
    pAeBaseInfo->nativeHdrRatio[0] = 10.0;
    pAeBaseInfo->conversionGainDCG = 10.0;


    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT OV2775_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);
    float longGain = 0, newGain = 0, vsGain = 0, dGainHcg = 0.0f, dGainLcg = 0.0f, dGainVs = 0.0f, gain = 0.0f;
    uint32_t gainHcg = 0, gainLcg = 0, againHcg = 0, aGainLcg = 0, aGainVs = 0, gainVs = 0;
    uint16_t data = 0;

    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL || pOV2775Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        /* base LCG, set linear LCG channel total gain*/
        newGain = pSensorAGain->gain[ISI_LINEAR_PARAS];
        if (newGain < 3.0) {
            newGain = 3.0;
            dGainLcg = newGain / 2.0;
            aGainLcg = 0x01;
        } else if (newGain < 4.0) {
            dGainLcg = newGain / 2.0;
            aGainLcg = 0x01;
        } else if (newGain < 8.0) {
            dGainLcg = newGain / 4.0;
            aGainLcg = 0x02;
        } else {
            dGainLcg = newGain / 8.0;
            aGainLcg = 0x03;
        }
        gainLcg = (uint32_t)(dGainLcg * 256);

        OV2775_IsiReadRegIss(handle, 0x30bb, &data);
        //data = ((data & 0x00) | (aGainLcg));
        data = aGainLcg;

        result = OV2775_IsiWriteRegIss(handle, 0x3467, 0x00);
        result |= OV2775_IsiWriteRegIss(handle, 0x3464, 0x04);

        result |= OV2775_IsiWriteRegIss(handle, 0x315a, ((gainLcg >> 8) & 0xff));
        result |= OV2775_IsiWriteRegIss(handle, 0x315b, (gainLcg & 0xff));

        result |= OV2775_IsiWriteRegIss(handle, 0x30bb, data);

        result |= OV2775_IsiWriteRegIss(handle, 0x3464, 0x14);
        result |= OV2775_IsiWriteRegIss(handle, 0x3467, 0x01);

        pOV2775Ctx->curAgain   = 1<<aGainLcg;
        pOV2775Ctx->curDgain   = gainLcg/256.0f;
        pOV2775Ctx->aecCurGain = (gainLcg/256.0f) * (1<<aGainLcg);

    } else if (pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        /* base LCG, set DUAL_DCG HCG and LCG channel total gain*/
        longGain = pSensorAGain->gain[ISI_TRI_EXP_L_PARAS];
        newGain = pSensorAGain->gain[ISI_TRI_EXP_S_PARAS];
        if(longGain > pOV2775Ctx->aecMaxGain)  longGain = pOV2775Ctx->aecMaxGain;
        if(newGain  > pOV2775Ctx->aecMaxGain)  newGain = pOV2775Ctx->aecMaxGain;
        if (longGain < 3.0) {
            dGainHcg = 3.0 / 2.0;
            againHcg = 0x01;
        } else if (longGain < 4.0) {
            dGainHcg = longGain / 2.0;
            againHcg = 0x01;
        } else if (longGain < 8.0) {
            dGainHcg = longGain / 4.0;
            againHcg = 0x02;
        } else {
            dGainHcg = longGain / 8.0;
            againHcg = 0x03;
        }

        if (newGain < 3.0) {
            dGainLcg = 3.0 / 2.0;
            aGainLcg = 0x01;
        } else if (newGain < 4.0) {
            dGainLcg = newGain / 2.0;
            aGainLcg = 0x01;
        } else if (newGain < 8.0) {
            dGainLcg = newGain / 4.0;
            aGainLcg = 0x02;
        } else {
            dGainLcg = newGain / 8.0;
            aGainLcg = 0x03;
        }

        gainLcg = (uint32_t)(dGainLcg  * 256);
        gainHcg = (uint32_t)(dGainHcg  * 256);
        OV2775_IsiReadRegIss(handle, 0x30bb, &data);
        data &= ~0x03;
        data |= againHcg & 0x03;//again_hcg, 30bb[1:0]
        data &= ~(0x03 << 2);
        data |= (aGainLcg & 0x03) << 2;//again_lcg, 30bb[3:2]

        result = OV2775_IsiWriteRegIss(handle, 0x3467, 0x00);
        result |= OV2775_IsiWriteRegIss(handle, 0x3464, 0x04);

        result |= OV2775_IsiWriteRegIss(handle, 0x315a, (gainHcg >> 8) & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x315b, gainHcg & 0xff);

        result |= OV2775_IsiWriteRegIss(handle, 0x315c, (gainLcg >> 8) & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x315d, gainLcg & 0xff);

        result |= OV2775_IsiWriteRegIss(handle, 0x30bb, data);

        result |= OV2775_IsiWriteRegIss(handle, 0x3464, 0x14);
        result |= OV2775_IsiWriteRegIss(handle, 0x3467, 0x01);

        pOV2775Ctx->curAgain       = 1 << aGainLcg;
        pOV2775Ctx->curLongAgain   = 1 << againHcg;
        pOV2775Ctx->curDgain       = gainLcg/256.0f;
        pOV2775Ctx->curLongDgain   = gainHcg/256.0f;
        pOV2775Ctx->aecCurGain     = (gainLcg/256.0f) * (1<<aGainLcg);
        pOV2775Ctx->aecCurLongGain = (gainHcg/256.0f) * (1<<againHcg);

        /* base LCG, set DUAL_DCG VS channel total gain*/
        vsGain = pSensorAGain->gain[ISI_TRI_EXP_VS_PARAS];
        if (vsGain < 3.0) {
            dGainVs = 3.0 / 2.0;
            aGainVs = 0x01;
        } else if (vsGain < 4.0) {
            dGainVs = vsGain / 2.0;
            aGainVs = 0x01;
        } else if (vsGain < 8.0) {
            dGainVs = vsGain / 4.0;
            aGainVs = 0x02;
        } else {
            dGainVs = vsGain / 8.0;
            aGainVs = 0x03;
        }
        gainVs = (uint32_t) (dGainVs * 256);
        OV2775_IsiReadRegIss(handle, 0x30bb, &data);
        data &= ~0x30;
        data |= (aGainVs & 0x03) << 4;

        result |= OV2775_IsiWriteRegIss(handle, 0x3467, 0x00);
        result |= OV2775_IsiWriteRegIss(handle, 0x3464, 0x04);

        result |= OV2775_IsiWriteRegIss(handle, 0x315e, (gainVs >> 8) & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x315f, gainVs & 0xff);

        result |= OV2775_IsiWriteRegIss(handle, 0x30bb, data);

        result |= OV2775_IsiWriteRegIss(handle, 0x3464, 0x14);
        result |= OV2775_IsiWriteRegIss(handle, 0x3467, 0x01);

        pOV2775Ctx->curVSAgain       = 1 << aGainVs;
        pOV2775Ctx->curVSDgain       = gainVs/256.0f;
        pOV2775Ctx->aecCurVSGain     = (1 << aGainVs) * (gainVs/256.0f);

    } else if(pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE){
        newGain = pSensorAGain->gain[ISI_LINEAR_PARAS];
        gain = newGain * 1; //default HCG/LCG conversion ratio 10.
        gain = MAX(pOV2775Ctx->aecMinGain, MIN(gain, pOV2775Ctx->aecMaxGain));
        if (gain < 3.0) {
            dGainHcg = 3.0 / 2.0;
            againHcg = 0x01;
        } else if (gain>=3.0 && gain < 4.0) {
            dGainHcg = gain / 2.0;
            againHcg = 0x01;
        } else if (gain>=4.0 && gain < 8.0) {
            dGainHcg = gain / 4.0;
            againHcg = 0x02;
        } else {
            dGainHcg = gain / 8.0;
            againHcg = 0x03;
        }

        if (newGain < 3.0) {
            dGainLcg = 3.0 / 2.0;
            aGainLcg = 0x01;
        } else if (newGain>=3.0 && newGain < 4.0) {
            dGainLcg = newGain / 2.0;
            aGainLcg = 0x01;
        } else if (newGain>=4.0 && newGain < 8.0 ) {
            dGainLcg = newGain / 4.0;
            aGainLcg = 0x02;
        } else {
            dGainLcg = newGain / 8.0;
            aGainLcg = 0x03;
        }

        gainLcg = (uint32_t)(dGainLcg  * 256);
        gainHcg = (uint32_t)(dGainHcg  * 256);
        OV2775_IsiReadRegIss(handle, 0x30bb, &data);
        data &= ~0x03;
        data |= againHcg & 0x03;//again_hcg, 30bb[1:0]
        data &= ~(0x03 << 2);
        data |= (aGainLcg & 0x03) << 2;//again_lcg, 30bb[3:2]

        result = OV2775_IsiWriteRegIss(handle, 0x3467, 0x00);
        result |= OV2775_IsiWriteRegIss(handle, 0x3464, 0x04);

        result |= OV2775_IsiWriteRegIss(handle, 0x315a, (gainHcg >> 8) & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x315b, gainHcg & 0xff);

        result |= OV2775_IsiWriteRegIss(handle, 0x315c, (gainLcg >> 8) & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x315d, gainLcg & 0xff);

        result |= OV2775_IsiWriteRegIss(handle, 0x30bb, data);

        result |= OV2775_IsiWriteRegIss(handle, 0x3464, 0x14);
        result |= OV2775_IsiWriteRegIss(handle, 0x3467, 0x01);

        pOV2775Ctx->curAgain = 1<<againHcg;
        pOV2775Ctx->curDgain = gainHcg / 256.0f;
        pOV2775Ctx->aecCurLongGain = (1<<againHcg) * (gainHcg / 256.0f);
    } else {
        TRACE(OV2775_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2775_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2775_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);

    if (pOV2775Ctx == NULL) {
        TRACE(OV2775_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    if(pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR || pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pSensorAGain->gain[ISI_LINEAR_PARAS]       = pOV2775Ctx->curAgain;

    } else if (pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        pSensorAGain->gain[ISI_TRI_EXP_L_PARAS]   = pOV2775Ctx->curLongAgain;
        pSensorAGain->gain[ISI_TRI_EXP_S_PARAS]   = pOV2775Ctx->curAgain;
        pSensorAGain->gain[ISI_TRI_EXP_VS_PARAS]  = pOV2775Ctx->curVSAgain;

    } else {
        TRACE(OV2775_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2775_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);

    if (pOV2775Ctx == NULL) {
        TRACE(OV2775_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    if (pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR || pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pSensorDGain->gain[ISI_LINEAR_PARAS] = pOV2775Ctx->curDgain;

    } else if (pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        pSensorDGain->gain[ISI_TRI_EXP_L_PARAS]   = pOV2775Ctx->curLongDgain;
        pSensorDGain->gain[ISI_TRI_EXP_S_PARAS]   = pOV2775Ctx->curDgain;
        pSensorDGain->gain[ISI_TRI_EXP_VS_PARAS]  = pOV2775Ctx->curVSDgain;

    } else {
        TRACE(OV2775_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2775_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);
    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;

    if (!pOV2775Ctx) {
        TRACE(OV2775_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR || pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        result = OV2775_SetIntTime(handle, pSensorIntTime->intTime[ISI_LINEAR_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(OV2775_INFO, "%s: set sensor IntTime[ISI_LINEAR_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

    } else if (pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        result = OV2775_SetIntTime(handle, pSensorIntTime->intTime[ISI_TRI_EXP_L_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(OV2775_INFO, "%s: set sensor IntTime[ISI_TRI_EXP_L_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

        result = OV2775_SetVSIntTime(handle, pSensorIntTime->intTime[ISI_TRI_EXP_VS_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(OV2775_INFO, "%s: set sensor IntTime[ISI_TRI_EXP_VS_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

    }  else {
        TRACE(OV2775_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV2775_SetIntTime(IsiSensorHandle_t handle, float newIntegrationTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);
    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pOV2775Ctx) {
        TRACE(OV2775_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = newIntegrationTime / pOV2775Ctx->oneLineExpTime;
    expLine = MIN(pOV2775Ctx->maxIntegrationLine, MAX(pOV2775Ctx->minIntegrationLine, expLine));
    TRACE(OV2775_DEBUG, "%s: set expline=0x%04x\n", __func__, expLine);

    result = OV2775_IsiWriteRegIss(handle, 0x3467, 0x00);
    result |= OV2775_IsiWriteRegIss(handle, 0x3464, 0x04);

    result |= OV2775_IsiWriteRegIss(handle, 0x30b6, (expLine >> 8) & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x30b7, expLine & 0xff);

    result |= OV2775_IsiWriteRegIss(handle, 0x3464, 0x14);
    result |= OV2775_IsiWriteRegIss(handle, 0x3467, 0x01);

    pOV2775Ctx->aecCurIntegrationTime     = expLine * pOV2775Ctx->oneLineExpTime;
    pOV2775Ctx->aecCurLongIntegrationTime = expLine * pOV2775Ctx->oneLineExpTime;

    TRACE(OV2775_DEBUG, "%s: set IntTime = %f\n", __func__, pOV2775Ctx->aecCurIntegrationTime);
    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV2775_SetVSIntTime(IsiSensorHandle_t handle, float newIntegrationTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);
    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pOV2775Ctx) {
        TRACE(OV2775_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = newIntegrationTime / pOV2775Ctx->oneLineExpTime;
    expLine = MIN(pOV2775Ctx->maxVSIntegrationLine, MAX(pOV2775Ctx->minVSIntegrationLine, expLine));

    result = OV2775_IsiWriteRegIss(handle, 0x3467, 0x00);
    result |= OV2775_IsiWriteRegIss(handle, 0x3464, 0x04);

    result |= OV2775_IsiWriteRegIss(handle, 0x30b8, (expLine >> 8) & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x30b9, expLine & 0xff);

    result |= OV2775_IsiWriteRegIss(handle, 0x3467, 0x14);
    result |= OV2775_IsiWriteRegIss(handle, 0x3464, 0x01);

    pOV2775Ctx->aecCurVSIntegrationTime = expLine * pOV2775Ctx->oneLineExpTime; 

    TRACE(OV2775_DEBUG, "%s: set VSIntTime = %f\n", __func__, pOV2775Ctx->aecCurVSIntegrationTime);
    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2775_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);

    if (!pOV2775Ctx) {
        TRACE(OV2775_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    if (pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR || pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pSensorIntTime->intTime[ISI_TRI_EXP_S_PARAS] = pOV2775Ctx->aecCurIntegrationTime;

    } else if (pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        pSensorIntTime->intTime[ISI_TRI_EXP_L_PARAS]   = pOV2775Ctx->aecCurLongIntegrationTime;
        pSensorIntTime->intTime[ISI_TRI_EXP_S_PARAS]   = pOV2775Ctx->aecCurIntegrationTime;
        pSensorIntTime->intTime[ISI_TRI_EXP_VS_PARAS]  = pOV2775Ctx->aecCurVSIntegrationTime;

    } else {
        TRACE(OV2775_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2775_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t * pFps)
{
    const OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);

    if (pOV2775Ctx == NULL) {
        TRACE(OV2775_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pOV2775Ctx->currFps;

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2775_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);

    if (pOV2775Ctx == NULL) {
        TRACE(OV2775_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (fps > pOV2775Ctx->maxFps) {
        TRACE(OV2775_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, fps, pOV2775Ctx->maxFps, pOV2775Ctx->minFps,pOV2775Ctx->maxFps);
        fps = pOV2775Ctx->maxFps;
    }
    if (fps < pOV2775Ctx->minFps) {
        TRACE(OV2775_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, fps, pOV2775Ctx->minFps, pOV2775Ctx->minFps,pOV2775Ctx->maxFps);
        fps = pOV2775Ctx->minFps;
    }

    uint16_t FrameLengthLines;
    FrameLengthLines = pOV2775Ctx->frameLengthLines * pOV2775Ctx->maxFps / fps;
    result = OV2775_IsiWriteRegIss(handle, 0x30b2,(FrameLengthLines >> 8) & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x30b3, FrameLengthLines & 0xff);
    if (result != RET_SUCCESS) {
        TRACE(OV2775_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_FAILURE);
    }
    pOV2775Ctx->currFps               = fps;
    pOV2775Ctx->curFrameLengthLines   = FrameLengthLines;
    pOV2775Ctx->maxIntegrationLine    = pOV2775Ctx->curFrameLengthLines - 3;
    pOV2775Ctx->aecMaxIntegrationTime = pOV2775Ctx->maxIntegrationLine * pOV2775Ctx->oneLineExpTime;

    TRACE(OV2775_INFO, "%s: set sensor fps = %d\n", __func__,pOV2775Ctx->currFps);

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2775_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    if (pOV2775Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pIspStatus->useSensorAWB = true;
        pIspStatus->useSensorBLC = true;
    } else {
        pIspStatus->useSensorAWB = false;
        pIspStatus->useSensorBLC = false;
    }

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

static RESULT OV2775_IsiSetBlcIss(IsiSensorHandle_t handle, const IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);

    uint32_t rOffset=0, grOffset=0, gbOffset=0, bOffset=0;
    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL || pOV2775Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)
        return RET_NULL_POINTER;

    float rGain=0, grGain=0, gbGain=0, bGain=0;
    rGain  = pOV2775Ctx->sensorWb.rGain;
    grGain = pOV2775Ctx->sensorWb.grGain;
    gbGain = pOV2775Ctx->sensorWb.gbGain;
    bGain  = pOV2775Ctx->sensorWb.bGain;

    if (rGain  < 1)  rGain  = 1;
    if (grGain < 1)  grGain = 1;
    if (gbGain < 1)  gbGain = 1;
    if (bGain  < 1)  bGain  = 1;

    rOffset  = (uint32_t)((rGain  - 1) * 0x100 * pBlc->red);
    grOffset = (uint32_t)((grGain - 1) * 0x100 * pBlc->gr);
    gbOffset = (uint32_t)((gbGain - 1) * 0x100 * pBlc->gb);
    bOffset  = (uint32_t)((bGain  - 1) * 0x100 * pBlc->blue);

    /* R,Gr,Gb,B HCG Offset */
    result |= OV2775_IsiWriteRegIss(handle, 0x3378, (rOffset >> 16) & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x3379, (rOffset >> 8)  & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x337a,  rOffset        & 0xff);

    result |= OV2775_IsiWriteRegIss(handle, 0x337b, (grOffset >> 16) & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x337c, (grOffset >> 8)  & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x337d,  grOffset        & 0xff);

    result |= OV2775_IsiWriteRegIss(handle, 0x337e, (gbOffset >> 16) & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x337f, (gbOffset >> 8)  & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x3380,  gbOffset        & 0xff);

    result |= OV2775_IsiWriteRegIss(handle, 0x3381, (bOffset >> 16) & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x3382, (bOffset >> 8)  & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x3383,  bOffset        & 0xff);

    /* R,Gr,Gb,B LCG Offset */
    result |= OV2775_IsiWriteRegIss(handle, 0x3384, (rOffset >> 16) & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x3385, (rOffset >> 8)  & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x3386,  rOffset        & 0xff);

    result |= OV2775_IsiWriteRegIss(handle, 0x3387, (grOffset >> 16) & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x3388, (grOffset >> 8)  & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x3389,  grOffset        & 0xff);

    result |= OV2775_IsiWriteRegIss(handle, 0x338a, (gbOffset >> 16) & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x338b, (gbOffset >> 8)  & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x338c,  gbOffset        & 0xff);

    result |= OV2775_IsiWriteRegIss(handle, 0x338d, (bOffset >> 16) & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x338e, (bOffset >> 8)  & 0xff);
    result |= OV2775_IsiWriteRegIss(handle, 0x338f,  bOffset        & 0xff);

    memcpy(&pOV2775Ctx->sensorBlc, pBlc, sizeof(IsiSensorBlc_t));

    if (result != RET_SUCCESS) {
        TRACE(OV2775_ERROR, "%s: set sensor blc error\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV2775_IsiGetBlcIss(IsiSensorHandle_t handle, IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);

    const OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)  return RET_NULL_POINTER;

    memcpy(pBlc, &pOV2775Ctx->sensorBlc, sizeof(IsiSensorBlc_t));

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV2775_IsiSetWBIss(IsiSensorHandle_t handle, const IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);

    bool update_flag = false;
    uint32_t rGain, grGain, gbGain, bGain;
    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL || pOV2775Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)
        return RET_NULL_POINTER;

    rGain  = (uint32_t)(pWb->rGain * 0x100);
    grGain = (uint32_t)(pWb->grGain * 0x100);
    gbGain = (uint32_t)(pWb->gbGain * 0x100);
    bGain  = (uint32_t)(pWb->bGain * 0x100);

    result |= OV2775_IsiWriteRegIss(handle, 0x3467, 0x00);
    result |= OV2775_IsiWriteRegIss(handle, 0x3464, 0x04);
    
    if (rGain != pOV2775Ctx->sensorWb.rGain) {
        result |= OV2775_IsiWriteRegIss(handle, 0x3360, (rGain >> 8) & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x3361,  rGain & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x3368, (rGain >> 8) & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x3369,  rGain & 0xff);
        update_flag = true;
        pOV2775Ctx->sensorWb.rGain = pWb->rGain;
    }
    if (grGain != pOV2775Ctx->sensorWb.grGain) {
        result |= OV2775_IsiWriteRegIss(handle, 0x3362, (grGain >> 8) & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x3363,  grGain & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x336a, (grGain >> 8) & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x336b,  grGain & 0xff);
        update_flag = true;
        pOV2775Ctx->sensorWb.grGain = pWb->grGain;
    }
    if (gbGain != pOV2775Ctx->sensorWb.gbGain) {
        result |= OV2775_IsiWriteRegIss(handle, 0x3364, (gbGain >> 8) & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x3365,  gbGain & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x336c, (gbGain >> 8) & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x336d,  gbGain & 0xff);
        update_flag = true;
        pOV2775Ctx->sensorWb.gbGain = pWb->gbGain;
    }
    if (bGain != pOV2775Ctx->sensorWb.bGain) {
        result |= OV2775_IsiWriteRegIss(handle, 0x3366, (bGain >> 8) & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x3367,  bGain & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x336e, (bGain >> 8) & 0xff);
        result |= OV2775_IsiWriteRegIss(handle, 0x336f,  bGain & 0xff);
        update_flag = true;
        pOV2775Ctx->sensorWb.bGain = pWb->bGain;
    }

    if (update_flag) {
        result = OV2775_IsiSetBlcIss(handle, &pOV2775Ctx->sensorBlc);
    }

    result |= OV2775_IsiWriteRegIss(handle, 0x3464, 0x14);
    result |= OV2775_IsiWriteRegIss(handle, 0x3467, 0x01);

    if (result != RET_SUCCESS) {
        TRACE(OV2775_ERROR, "%s: set sensor wb error\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV2775_IsiGetWBIss(IsiSensorHandle_t handle, IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);

    const OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL ) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)  return RET_NULL_POINTER;

    memcpy(pWb, &pOV2775Ctx->sensorWb, sizeof(IsiSensorWb_t));

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2775_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);

    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL || pOV2775Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV2775Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = OV2775_IsiWriteRegIss(handle, 0x3253, 0x00);
    } else {
        result = OV2775_IsiWriteRegIss(handle, 0x3253, 0x80);
    }

    pOV2775Ctx->testPattern = tpg.enable;

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2775_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);

    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL || pOV2775Ctx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV2775Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!OV2775_IsiReadRegIss(handle, 0x3253,&value)) {
        pTpg->enable = ((value & 0x80) != 0) ? 1 : 0;
        if(pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pOV2775Ctx->testPattern = pTpg->enable;
    }

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV2775_IsiGetExpandCurveIss(IsiSensorHandle_t handle, IsiSensorCompandCurve_t *pCurve)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2775_INFO, "%s: (enter)\n", __func__);

    OV2775_Context_t *pOV2775Ctx = (OV2775_Context_t *) handle;
    if (pOV2775Ctx == NULL || pOV2775Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    uint8_t expandPx[64] = {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6};
    memcpy(pCurve->compandPx, expandPx, sizeof(expandPx));

    pCurve->compandXData[0] = 0;
    pCurve->compandYData[0] = 0;
    for(int i=1; i<65; i++){
        pCurve->compandXData[i] = (1 << pCurve->compandPx[i-1]) + pCurve->compandXData[i-1];

        if(pCurve->compandXData[i] < 512){
            pCurve->compandYData[i] = pCurve->compandXData[i] << 1;//*2
        } else if (pCurve->compandXData[i] < 768){
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 256) << 2;//*4
        } else if (pCurve->compandXData[i] < 2560){
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 512) << 3;//*8
        } else {
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 2048) << 5;//*32
        }
    }

    TRACE(OV2775_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2775_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "OV2775";
    TRACE(OV2775_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = OV2775_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = OV2775_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = OV2775_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = OV2775_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = OV2775_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = OV2775_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = OV2775_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = OV2775_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = OV2775_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = OV2775_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = OV2775_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = OV2775_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = OV2775_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = OV2775_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = OV2775_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = OV2775_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = OV2775_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = OV2775_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = OV2775_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = OV2775_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = OV2775_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = OV2775_IsiGetIspStatusIss;
        pIsiSensor->pIsiSetBlcIss                       = OV2775_IsiSetBlcIss;
        pIsiSensor->pIsiGetBlcIss                       = OV2775_IsiGetBlcIss;
        pIsiSensor->pIsiSetWBIss                        = OV2775_IsiSetWBIss;
        pIsiSensor->pIsiGetWBIss                        = OV2775_IsiGetWBIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = OV2775_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = OV2775_IsiGetTpgIss;
        pIsiSensor->pIsiGetExpandCurveIss               = OV2775_IsiGetExpandCurveIss;

        /* AF */
        pIsiSensor->pIsiFocusCreateIss                  = NULL;
        pIsiSensor->pIsiFocusReleaseIss                 = NULL;
        pIsiSensor->pIsiFocusGetCalibrateIss            = NULL;
        pIsiSensor->pIsiFocusSetIss                     = NULL;
        pIsiSensor->pIsiFocusGetIss                     = NULL;
        /* metadata*/
        pIsiSensor->pIsiQueryMetadataAttrIss            = NULL;
        pIsiSensor->pIsiSetMetadataAttrEnableIss        = NULL;
        pIsiSensor->pIsiGetMetadataAttrEnableIss        = NULL;
        pIsiSensor->pIsiGetMetadataWinIss               = NULL;
        pIsiSensor->pIsiParserMetadataIss               = NULL;

    } else {
        result = RET_NULL_POINTER;
    }

    TRACE(OV2775_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t OV2775_IsiCamDrvConfig = {
    .cameraDriverID      = 0x2770,
    .pIsiGetSensorIss    = OV2775_IsiGetSensorIss,
};
