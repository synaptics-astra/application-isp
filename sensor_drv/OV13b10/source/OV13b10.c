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

#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <common/misc.h>
#include "isi.h"
#include "isi_iss.h"
#include "OV13b10_priv.h"

CREATE_TRACER(OV13b10_INFO , "OV13b10: ", INFO,    1);
CREATE_TRACER(OV13b10_WARN , "OV13b10: ", WARNING, 1);
CREATE_TRACER(OV13b10_ERROR, "OV13b10: ", ERROR,   1);
CREATE_TRACER(OV13b10_DEBUG,     "OV13b10: ", INFO, 1);
CREATE_TRACER(OV13b10_REG_INFO , "OV13b10: ", INFO, 1);
CREATE_TRACER(OV13b10_REG_DEBUG, "OV13b10: ", INFO, 1);

#define OV13b10_MIN_GAIN_STEP    (1.0f/128.0f)  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain ) */
#define VCM_ADDR                 0x0c
#define MIN_VCM_POS              0x00
#define MAX_VCM_POS              0x3FF


/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pov13b10_mode_info[] = {
    {
        .index     = 0,
        .size      = {
            .boundsWidth  = 4208,
            .boundsHeight = 3120,
            .top           = 0,
            .left          = 0,
            .width         = 4208,
            .height        = 3120,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps       = 1 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_PDAF,
    },
    {
        .index     = 1,
        .size      = {
            .boundsWidth  = 4208,
            .boundsHeight = 2368,
            .top           = 0,
            .left          = 0,
            .width         = 4208,
            .height        = 2368,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps       = 0.3 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_PDAF,
    },
    {
        .index     = 2,
        .size      = {
            .boundsWidth  = 1280,
            .boundsHeight = 720,
            .top           = 0,
            .left          = 0,
            .width         = 1280,
            .height        = 720,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps       = 3 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_PDAF,
    },
    {
        .index     = 3,
        .size      = {
            .boundsWidth  = 1280,
            .boundsHeight = 720,
            .top           = 0,
            .left          = 0,
            .width         = 1280,
            .height        = 720,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps       = 0.5 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_PDAF,
    }
};

static RESULT OV13b10_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV13b10_INFO, "%s (enter)\n", __func__);

    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    if (pOV13b10Ctx == NULL || pOV13b10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pOV13b10Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(OV13b10_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV13b10_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV13b10_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV13b10_INFO, "%s (enter)\n", __func__);

    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    if (pOV13b10Ctx == NULL || pOV13b10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pOV13b10Ctx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(OV13b10_ERROR, "%s: hal write sensor register error!\n",__func__);
        return (RET_FAILURE);
    }

    TRACE(OV13b10_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV13b10_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(OV13b10_INFO, "%s (enter)\n", __func__);
    const OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    if (pOV13b10Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pOV13b10Ctx->sensorMode), sizeof(pOV13b10Ctx->sensorMode));

    TRACE(OV13b10_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT OV13b10_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(OV13b10_INFO, "%s (enter)\n", __func__);
    const OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    if (pOV13b10Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pov13b10_mode_info)/sizeof(pov13b10_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pov13b10_mode_info)/sizeof(pov13b10_mode_info[0])); i++) {
        if (pov13b10_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pov13b10_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(OV13b10_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT OV13b10_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(OV13b10_INFO, "%s (enter)\n", __func__);

    if (pOV13b10Ctx == NULL)
        return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pOV13b10Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pOV13b10Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pOV13b10Ctx->sensorMode.size.width;
    pCaps->resolution.height = pOV13b10Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(OV13b10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV13b10_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV13b10_INFO, "%s (enter)\n", __func__);

    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) osMalloc(sizeof(OV13b10_Context_t));
    if (!pOV13b10Ctx) {
        TRACE(OV13b10_ERROR, "%s: Can't allocate ov13b10 context\n", __func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pOV13b10Ctx, 0, sizeof(OV13b10_Context_t));

    pOV13b10Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pOV13b10Ctx->groupHold          = BOOL_FALSE;
    pOV13b10Ctx->oldGain            = 0;
    pOV13b10Ctx->oldIntegrationTime = 0;
    pOV13b10Ctx->configured         = BOOL_FALSE;
    pOV13b10Ctx->streaming          = BOOL_FALSE;
    pOV13b10Ctx->testPattern        = BOOL_FALSE;
    pOV13b10Ctx->isAfpsRun          = BOOL_FALSE;
    pOV13b10Ctx->sensorMode.index   = 0;
    
    pOV13b10Ctx->i2cBusID = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x6c;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pOV13b10Ctx->isiCtx.halI2cHandle = HalI2cOpen(pOV13b10Ctx->i2cBusID, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pOV13b10Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(OV13b10_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pOV13b10Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        osFree(pOV13b10Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pOV13b10Ctx;

    TRACE(OV13b10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV13b10_AecSetModeParameters(IsiSensorHandle_t handle, OV13b10_Context_t * pOV13b10Ctx)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    uint32_t regVal = 0;
    float again = 0, dgain = 0;
    TRACE(OV13b10_INFO, "%s%s: (enter)\n", __func__,pOV13b10Ctx->isAfpsRun ? "(AFPS)" : "");

    pOV13b10Ctx->aecIntegrationTimeIncrement = pOV13b10Ctx->oneLineExpTime;
    pOV13b10Ctx->aecMinIntegrationTime = pOV13b10Ctx->oneLineExpTime * pOV13b10Ctx->minIntegrationLine;
    pOV13b10Ctx->aecMaxIntegrationTime = pOV13b10Ctx->oneLineExpTime * pOV13b10Ctx->maxIntegrationLine;

    TRACE(OV13b10_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__,pOV13b10Ctx->isAfpsRun ? "(AFPS)" : "",pOV13b10Ctx->aecMaxIntegrationTime);

    pOV13b10Ctx->aecGainIncrement = OV13b10_MIN_GAIN_STEP;
    pOV13b10Ctx->oldGain = 0;
    pOV13b10Ctx->oldIntegrationTime = 0;

    OV13b10_IsiReadRegIss(handle, 0x3500, &value);
    regVal = value << 16;
    OV13b10_IsiReadRegIss(handle, 0x3501, &value);
    regVal |= value << 8;
    OV13b10_IsiReadRegIss(handle, 0x3502, &value);
    regVal |= value & 0xFF;
    pOV13b10Ctx->aecCurIntegrationTime = regVal * pOV13b10Ctx->oneLineExpTime;

    OV13b10_IsiReadRegIss(handle, 0x3508, &value);
    regVal = (value & 0x0f) << 7;
    OV13b10_IsiReadRegIss(handle, 0x3509, &value);
    regVal = regVal | ((value & 0xfe) >> 1);
    again = (float)regVal/128.0f;

    OV13b10_IsiReadRegIss(handle, 0x350a, &value);
    regVal = (value & 0x03) << 10;
    OV13b10_IsiReadRegIss(handle, 0x350b, &value);
    regVal = regVal | ((value & 0xFF) << 2);
    OV13b10_IsiReadRegIss(handle, 0x350c, &value);
    regVal = regVal | ((value & 0xc0) >> 6);
    dgain = (float)regVal/1024.0f;
    pOV13b10Ctx->aecCurGain = again * dgain;
    TRACE(OV13b10_INFO, "%s%s: (exit)\n", __func__,pOV13b10Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT OV13b10_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OV13b10_INFO, "%s (enter)\n", __func__);

    if (!pOV13b10Ctx) {
        TRACE(OV13b10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOV13b10Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pOV13b10Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pov13b10_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (pov13b10_mode_info[i].index == pOV13b10Ctx->sensorMode.index) {
            SensorDefaultMode = &(pov13b10_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(OV13b10_mipi4lane_4208_3120_init) / sizeof(OV13b10_mipi4lane_4208_3120_init[0]); i++) {
                    OV13b10_IsiWriteRegIss(handle, OV13b10_mipi4lane_4208_3120_init[i][0], OV13b10_mipi4lane_4208_3120_init[i][1]);
                }
                break;
            case 1:
                for (int i = 0; i<sizeof(OV13b10_mipi4lane_4208_2368_init) / sizeof(OV13b10_mipi4lane_4208_2368_init[0]); i++) {
                    OV13b10_IsiWriteRegIss(handle, OV13b10_mipi4lane_4208_2368_init[i][0], OV13b10_mipi4lane_4208_2368_init[i][1]);
                }
                break;
            case 2:
                for (int i = 0; i<sizeof(OV13b10_mipi4lane_720p_3fps_init) / sizeof(OV13b10_mipi4lane_720p_3fps_init[0]); i++) {
                    OV13b10_IsiWriteRegIss(handle, OV13b10_mipi4lane_720p_3fps_init[i][0], OV13b10_mipi4lane_720p_3fps_init[i][1]);
                }
                break;
            case 3:
                for (int i = 0; i<sizeof(OV13b10_mipi4lane_720p_2s1f_init) / sizeof(OV13b10_mipi4lane_720p_2s1f_init[0]); i++) {
                    OV13b10_IsiWriteRegIss(handle, OV13b10_mipi4lane_720p_2s1f_init[i][0], OV13b10_mipi4lane_720p_2s1f_init[i][1]);
                }
                break;
            default:
                TRACE(OV13b10_INFO, "%s:not support sensor mode %d\n", __func__, pOV13b10Ctx->sensorMode.index);
                return RET_NOTSUPP;
                break;
        }

        memcpy(&(pOV13b10Ctx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));
    } else {
        TRACE(OV13b10_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pOV13b10Ctx->sensorMode.index) {
        case 0:
            pOV13b10Ctx->oneLineExpTime      = 0.0000705;
            pOV13b10Ctx->frameLengthLines    = 0x3074;
            pOV13b10Ctx->curFrameLengthLines = pOV13b10Ctx->frameLengthLines;
            pOV13b10Ctx->maxIntegrationLine  = pOV13b10Ctx->frameLengthLines - 8;
            pOV13b10Ctx->minIntegrationLine  = 4;
            pOV13b10Ctx->aecMaxGain          = 62;
            pOV13b10Ctx->aecMinGain          = 1.0;
            pOV13b10Ctx->aGain.min           = 1.0;
            pOV13b10Ctx->aGain.max           = 15.5;
            pOV13b10Ctx->aGain.step          = (1.0f/128.0f);
            pOV13b10Ctx->dGain.min           = 1.0;
            pOV13b10Ctx->dGain.max           = 4;
            pOV13b10Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 1:
            pOV13b10Ctx->oneLineExpTime      = 0.000079;
            pOV13b10Ctx->frameLengthLines    = 0x20f8;
            pOV13b10Ctx->curFrameLengthLines = pOV13b10Ctx->frameLengthLines;
            pOV13b10Ctx->maxIntegrationLine  = pOV13b10Ctx->frameLengthLines - 8;
            pOV13b10Ctx->minIntegrationLine  = 4;
            pOV13b10Ctx->aecMaxGain          = 62;
            pOV13b10Ctx->aecMinGain          = 1.0;
            pOV13b10Ctx->aGain.min           = 1.0;
            pOV13b10Ctx->aGain.max           = 15.5;
            pOV13b10Ctx->aGain.step          = (1.0f/128.0f);
            pOV13b10Ctx->dGain.min           = 1.0;
            pOV13b10Ctx->dGain.max           = 4;
            pOV13b10Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 2:
            pOV13b10Ctx->oneLineExpTime      = 0.000052;
            pOV13b10Ctx->frameLengthLines    = 0x18f8;
            pOV13b10Ctx->curFrameLengthLines = pOV13b10Ctx->frameLengthLines;
            pOV13b10Ctx->maxIntegrationLine  = pOV13b10Ctx->curFrameLengthLines - 8;
            pOV13b10Ctx->minIntegrationLine  = 4;
            pOV13b10Ctx->aecMaxGain          = 62;
            pOV13b10Ctx->aecMinGain          = 1.0;
            pOV13b10Ctx->aGain.min           = 1.0;
            pOV13b10Ctx->aGain.max           = 15.5;
            pOV13b10Ctx->aGain.step          = (1.0f/128.0f);
            pOV13b10Ctx->dGain.min           = 1.0;
            pOV13b10Ctx->dGain.max           = 4;
            pOV13b10Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 3:
            pOV13b10Ctx->oneLineExpTime      = 0.000104;
            pOV13b10Ctx->frameLengthLines    = 0x4ff8;
            pOV13b10Ctx->curFrameLengthLines = pOV13b10Ctx->frameLengthLines;
            pOV13b10Ctx->maxIntegrationLine  = pOV13b10Ctx->curFrameLengthLines - 8;
            pOV13b10Ctx->minIntegrationLine  = 4;
            pOV13b10Ctx->aecMaxGain          = 62;
            pOV13b10Ctx->aecMinGain          = 1.0;
            pOV13b10Ctx->aGain.min           = 1.0;
            pOV13b10Ctx->aGain.max           = 15.5;
            pOV13b10Ctx->aGain.step          = (1.0f/128.0f);
            pOV13b10Ctx->dGain.min           = 1.0;
            pOV13b10Ctx->dGain.max           = 4;
            pOV13b10Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        default:
            TRACE(OV13b10_INFO, "%s:not support sensor mode %d\n", __func__,pOV13b10Ctx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    pOV13b10Ctx->maxFps  = pOV13b10Ctx->sensorMode.fps;
    pOV13b10Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pOV13b10Ctx->currFps = pOV13b10Ctx->maxFps;

    TRACE(OV13b10_DEBUG, "%s: OV13b10 System-Reset executed\n", __func__);
    osSleep(100);

    result = OV13b10_AecSetModeParameters(handle, pOV13b10Ctx);
    if (result != RET_SUCCESS) {
        TRACE(OV13b10_ERROR, "%s: SetupOutputWindow failed.\n",__func__);
        return (result);
    }

    pOV13b10Ctx->configured = BOOL_TRUE;
    TRACE(OV13b10_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT OV13b10_IsiCloseIss(IsiSensorHandle_t handle)
{
    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV13b10_INFO, "%s (enter)\n", __func__);

    if (pOV13b10Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)OV13b10_IsiSetStreamingIss(pOV13b10Ctx, BOOL_FALSE);

    TRACE(OV13b10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV13b10_IsiReleaseIss(IsiSensorHandle_t handle)
{
    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV13b10_INFO, "%s (enter)\n", __func__);

    if (pOV13b10Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pOV13b10Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pOV13b10Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    osFree(pOV13b10Ctx->pLscData);
    pOV13b10Ctx->pLscData = NULL;

    osFree(pOV13b10Ctx->pAwbData);
    pOV13b10Ctx->pAwbData = NULL;

    osFree(pOV13b10Ctx->pGoldenAwbData);
    pOV13b10Ctx->pGoldenAwbData = NULL;

    osFree(pOV13b10Ctx->pLightSourceData);
    pOV13b10Ctx->pLightSourceData = NULL;

    osFree(pOV13b10Ctx);
    pOV13b10Ctx = NULL;
    TRACE(OV13b10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV13b10_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;

    uint32_t sensorId = 0;
    uint32_t correctId = 0x560d;

    TRACE(OV13b10_INFO, "%s (enter)\n", __func__);

    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    if (pOV13b10Ctx == NULL || pOV13b10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = OV13b10_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(OV13b10_ERROR, "%s: Read Sensor ID Error! \n", __func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(OV13b10_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n", __func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(OV13b10_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__,correctId, sensorId);
    TRACE(OV13b10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV13b10_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(OV13b10_INFO, "%s (enter)\n", __func__);

    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    if (pOV13b10Ctx == NULL || pOV13b10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    regVal = 0;
    result = OV13b10_IsiReadRegIss(handle, 0x300a, &regVal);
    sensorId = (regVal & 0xff) << 8;

    regVal = 0;
    result |= OV13b10_IsiReadRegIss(handle, 0x300b, &regVal);
    sensorId |= (regVal & 0xff);
    TRACE(OV13b10_INFO, "%s sensorId = %d \n", __func__,sensorId);

    *pValue = sensorId;
    TRACE(OV13b10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV13b10_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV13b10_INFO, "%s (enter)\n", __func__);

    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    if (pOV13b10Ctx == NULL || pOV13b10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV13b10Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    result = OV13b10_IsiWriteRegIss(handle, 0x0100, on);
    if (result != RET_SUCCESS) {
        TRACE(OV13b10_ERROR, "%s: set sensor streaming error! \n",__func__);
        return (RET_FAILURE);
    }

    pOV13b10Ctx->streaming = on;

    TRACE(OV13b10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV13b10_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    const OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OV13b10_INFO, "%s: (enter)\n", __func__);

    if (pOV13b10Ctx == NULL) {
        TRACE(OV13b10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(OV13b10_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    //get time limit and total gain limit
    pAeBaseInfo->gain.min        = pOV13b10Ctx->aecMinGain;
    pAeBaseInfo->gain.max        = pOV13b10Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pOV13b10Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pOV13b10Ctx->aecMaxIntegrationTime;

    //get again/dgain info
    pAeBaseInfo->aGain           = pOV13b10Ctx->aGain;
    pAeBaseInfo->dGain           = pOV13b10Ctx->dGain;
    
    pAeBaseInfo->aecCurGain      = pOV13b10Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime   = pOV13b10Ctx->aecCurIntegrationTime;
    pAeBaseInfo->aecGainStep     = pOV13b10Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep  = pOV13b10Ctx->aecIntegrationTimeIncrement;

    TRACE(OV13b10_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT OV13b10_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV13b10_INFO, "%s: (enter)\n", __func__);
    uint32_t again = 0; 

    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    if (pOV13b10Ctx == NULL || pOV13b10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    again = (uint32_t)(pSensorAGain->gain[ISI_LINEAR_PARAS] * 128);
    result = OV13b10_IsiWriteRegIss(handle, 0x3508,  (again >> 7) & 0x0f);
    result |= OV13b10_IsiWriteRegIss(handle, 0x3509, (again & 0x7f) << 1);
    pOV13b10Ctx->curAgain = (float)again/128.0f;

    TRACE(OV13b10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV13b10_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV13b10_INFO, "%s: (enter)\n", __func__);
    uint32_t dgain = 0; 

    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    if (pOV13b10Ctx == NULL || pOV13b10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    dgain = (uint32_t)((pSensorDGain->gain[ISI_LINEAR_PARAS]) * 1024);
    result = OV13b10_IsiWriteRegIss(handle,  0x350a, (dgain >> 10) & 0x03);
    result |= OV13b10_IsiWriteRegIss(handle, 0x350b, (dgain >> 2) & 0xff);
    result |= OV13b10_IsiWriteRegIss(handle, 0x350c, (dgain & 0x3)<<6);
    pOV13b10Ctx->curDgain = (float)dgain/1024.0f;
    pOV13b10Ctx->aecCurGain = pOV13b10Ctx->curAgain * pOV13b10Ctx->curDgain;

    TRACE(OV13b10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV13b10_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    const OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV13b10_INFO, "%s: (enter)\n", __func__);

    if (pOV13b10Ctx == NULL) {
        TRACE(OV13b10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorAGain->gain[ISI_LINEAR_PARAS] = pOV13b10Ctx->curAgain;

    TRACE(OV13b10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV13b10_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    const OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV13b10_INFO, "%s: (enter)\n", __func__);

    if (pOV13b10Ctx == NULL) {
        TRACE(OV13b10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorDGain->gain[ISI_LINEAR_PARAS] = pOV13b10Ctx->curDgain;

    TRACE(OV13b10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV13b10_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV13b10_INFO, "%s: (enter)\n", __func__);
    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pOV13b10Ctx) {
        TRACE(OV13b10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }
    
    expLine = pSensorIntTime->intTime[ISI_LINEAR_PARAS] / pOV13b10Ctx->oneLineExpTime;
    expLine = MIN(pOV13b10Ctx->maxIntegrationLine, MAX(pOV13b10Ctx->minIntegrationLine, expLine));
    TRACE(OV13b10_DEBUG, "%s: set expLine=0x%04x\n", __func__, expLine);

    result |=  OV13b10_IsiWriteRegIss(handle, 0x3500,(expLine >> 16) & 0xff);
    result |=  OV13b10_IsiWriteRegIss(handle, 0x3501,(expLine >> 8) & 0xff);
    result |= OV13b10_IsiWriteRegIss(handle, 0x3502,(expLine & 0xff));

    pOV13b10Ctx->aecCurIntegrationTime = expLine * pOV13b10Ctx->oneLineExpTime;

    TRACE(OV13b10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV13b10_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    const OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV13b10_INFO, "%s: (enter)\n", __func__);

    if (!pOV13b10Ctx) {
        TRACE(OV13b10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    pSensorIntTime->intTime[ISI_LINEAR_PARAS] = pOV13b10Ctx->aecCurIntegrationTime;

    TRACE(OV13b10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV13b10_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t *pFps)
{
    const OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV13b10_INFO, "%s: (enter)\n", __func__);

    if (pOV13b10Ctx == NULL) {
        TRACE(OV13b10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pOV13b10Ctx->currFps;

    TRACE(OV13b10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV13b10_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    RESULT result = RET_SUCCESS;
    int32_t NewVts = 0;
    TRACE(OV13b10_INFO, "%s: (enter)\n", __func__);

    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    if (pOV13b10Ctx == NULL) {
        TRACE(OV13b10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (fps > pOV13b10Ctx->maxFps) {
        TRACE(OV13b10_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, fps, pOV13b10Ctx->maxFps, pOV13b10Ctx->minFps,pOV13b10Ctx->maxFps);
        fps = pOV13b10Ctx->maxFps;
    }
    if (fps < pOV13b10Ctx->minFps) {
        TRACE(OV13b10_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",
              __func__, fps, pOV13b10Ctx->minFps, pOV13b10Ctx->minFps,pOV13b10Ctx->maxFps);
        fps = pOV13b10Ctx->minFps;
    }

    NewVts = pOV13b10Ctx->frameLengthLines*pOV13b10Ctx->sensorMode.fps / fps;
    result =  OV13b10_IsiWriteRegIss(handle, 0x380e, NewVts >> 8);
    result |=  OV13b10_IsiWriteRegIss(handle, 0x380f, NewVts & 0xff);
    pOV13b10Ctx->currFps              = fps;
    pOV13b10Ctx->curFrameLengthLines  = NewVts;
    pOV13b10Ctx->maxIntegrationLine   = pOV13b10Ctx->curFrameLengthLines - 8;
    //pOV13b10Ctx->aecMaxIntegrationTime = pOV13b10Ctx->maxIntegrationLine * pOV13b10Ctx->oneLineExpTime;

    TRACE(OV13b10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV13b10_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    const OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    if (pOV13b10Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(OV13b10_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    pIspStatus->useSensorAWB = false;

    TRACE(OV13b10_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT OV13b10_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV13b10_INFO, "%s: (enter)\n", __func__);

    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    if (pOV13b10Ctx == NULL || pOV13b10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV13b10Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = OV13b10_IsiWriteRegIss(handle, 0x5080, 0x00);
    } else {
        result = OV13b10_IsiWriteRegIss(handle, 0x5080, 0x80);
    }

    pOV13b10Ctx->testPattern = tpg.enable;

    TRACE(OV13b10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV13b10_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(OV13b10_INFO, "%s: (enter)\n", __func__);

    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    if (pOV13b10Ctx == NULL || pOV13b10Ctx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV13b10Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!OV13b10_IsiReadRegIss(handle, 0x5080,&value)) {
        pTpg->enable = ((value & 0x80) != 0) ? 1 : 0;
        if(pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pOV13b10Ctx->testPattern = pTpg->enable;
    }

    TRACE(OV13b10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV13b10_IsiFocusGetCalibrateIss(IsiSensorHandle_t handle, IsiFocusCalibAttr_t *pFocusCalib)
{
    TRACE(OV13b10_INFO, "%s: (enter)\n", __func__);

    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    if (pOV13b10Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    switch (pOV13b10Ctx->sensorMode.index)
    {
    case 0:
        pFocusCalib->pdInfo.sensorType     = ISI_PDAF_SENSOR_OCL2X1;
        pFocusCalib->pdInfo.ocl2x1Shield   = 1;
        pFocusCalib->pdInfo.bayerPattern   = ISI_BPAT_BGGR;
        pFocusCalib->pdInfo.bitWidth       = 16;//10bit
        pFocusCalib->pdInfo.imageWidth     = 4208;
        pFocusCalib->pdInfo.imageHeight    = 3120;
        pFocusCalib->pdInfo.pdArea[0]      = 8;  //relate to sensor configure
        pFocusCalib->pdInfo.pdArea[1]      = 8;
        // if filp or mirror, pd position will change at the same time
        pFocusCalib->pdInfo.pdShiftL2R[0]  = 0;
        pFocusCalib->pdInfo.pdShiftL2R[1]  = 4;
        MEMSET(&pFocusCalib->pdInfo.pdShiftMark, 0, sizeof(pFocusCalib->pdInfo.pdShiftMark));
        pFocusCalib->pdInfo.pdShiftMark[0] = 14;
        pFocusCalib->pdInfo.pdShiftMark[1] = 2;
        pFocusCalib->pdInfo.pdShiftMark[2] = 6;
        pFocusCalib->pdInfo.pdShiftMark[3] = 10;
        pFocusCalib->pdInfo.pdShiftMark[4] = 0;
        pFocusCalib->pdInfo.pdShiftMark[5] = 0;
        pFocusCalib->pdInfo.pdShiftMark[6] = 0;
        pFocusCalib->pdInfo.pdShiftMark[7] = 0;

        pFocusCalib->pdInfo.correctRect[0] = 0;
        pFocusCalib->pdInfo.correctRect[1] = 0;
        pFocusCalib->pdInfo.correctRect[2] = 4208;
        pFocusCalib->pdInfo.correctRect[3] = 3120;
        break;
    default:
        // ISI_PDAF_ISI_SENSOR_TYPE_MAX means not support PDAF
        pFocusCalib->pdInfo.sensorType = ISI_PDAF_SENSOR_TYPE_MAX;
        TRACE(OV13b10_WARN, "%s: mode index %d is not support PDAF\n",__func__, pOV13b10Ctx->sensorMode.index);
        break;
    }

    pFocusCalib->pdInfo.pdArea[2]       = 16;
    pFocusCalib->pdInfo.pdArea[3]       = 16;
    pFocusCalib->pdInfo.pdNumPerArea[0] = 1;
    pFocusCalib->pdInfo.pdNumPerArea[1] = 2;
    pFocusCalib->pdInfo.pdFocalHeigh    = 3;
    pFocusCalib->pdInfo.pdFocalWidth    = 3;
    int pdFocalInit[48] ={-38,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    MEMCPY(&pFocusCalib->pdInfo.pdFocal, pdFocalInit, sizeof(pdFocalInit));

    pFocusCalib->posInfo.minPos  = MIN_VCM_POS;
    pFocusCalib->posInfo.maxPos  = MAX_VCM_POS;
    pFocusCalib->posInfo.minStep = 2;

    return RET_SUCCESS;
}

RESULT OV13b10_IsiGetOtpDataIss(IsiSensorHandle_t handle, IsiOTP_t *pOtpData)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV13b10_INFO, "%s: (enter)\n", __func__);

    OV13b10_Context_t *pOV13b10Ctx = (OV13b10_Context_t *) handle;
    if (pOV13b10Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    pOtpData->otpInformation.hwVersion = 2;
    pOtpData->otpInformation.eepromRevision = 2;
    pOtpData->otpInformation.sensorRevision = 0;
    pOtpData->otpInformation.tlensRevision = 255;
    pOtpData->otpInformation.ircfRevision = 1;
    pOtpData->otpInformation.lensRevision = 1;
    pOtpData->otpInformation.caRevision = 255;
    pOtpData->otpInformation.moduleInteID = 20557;
    pOtpData->otpInformation.factoryID = 17479;
    pOtpData->otpInformation.mirrorFlip = 0;
    pOtpData->otpInformation.tlensSlaveID = 24;
    pOtpData->otpInformation.eepromSlaveID = 160;
    pOtpData->otpInformation.sensorSlaveID = 32;
    for (int i=0; i<11; i++){
        pOtpData->otpInformation.sensorID[i] = 15 + i;
    }
    pOtpData->otpInformation.manuDateYear = 2024;
    pOtpData->otpInformation.manuDateMonth = 1;
    pOtpData->otpInformation.manuDateDay = 8;
    uint8_t barcodeModuleSN[12] = {80, 51, 56, 55, 48, 50, 52, 69, 48, 49, 55, 53};
    for (int i=0; i<12; i++){
        pOtpData->otpInformation.barcodeModuleSN[i] = barcodeModuleSN[i];
    }
    pOtpData->otpInformation.mapTotalSize = 2501;

    pOtpData->otpLSCEnable         = true;
    pOtpData->otpAwbEnable         = true;
    pOtpData->otpLightSourceEnable = true;
    pOtpData->otpFocusEnable       = true;

    pOtpData->lscNum = 3;
    pOtpData->awbNum = 3;
    pOtpData->goldenAwbNum = 3;
    pOtpData->lightSourceNum = 3;

    //lsc data
    if (pOV13b10Ctx->pLscData == NULL) {
        pOV13b10Ctx->pLscData = (IsiOTPLSC_t *) osMalloc(pOtpData->lscNum * sizeof(IsiOTPLSC_t));
        if (!(pOV13b10Ctx->pLscData)) {
            TRACE(OV13b10_ERROR, "%s: Can't allocate pOV13b10Ctx->pLscData!\n", __func__);
            return (RET_OUTOFMEM);
        }
        MEMSET(pOV13b10Ctx->pLscData, 0, pOtpData->lscNum * sizeof(IsiOTPLSC_t));
    }
    pOtpData->pLscData = pOV13b10Ctx->pLscData;
    pOtpData->pLscData[0].colorTemperature = ISI_COLOR_TEMPERATURE_3100K;
    for (int i=0; i<ISI_OTP_LSC_TABLE_NUM; i++){
        for (int j=0; j<ISI_OTP_LSC_TABLE_NUM; j++){
            pOtpData->pLscData[0].r[i][j] = 0;
            pOtpData->pLscData[0].gr[i][j] = 0;
            pOtpData->pLscData[0].gb[i][j] = 0;
            pOtpData->pLscData[0].b[i][j] = 0;
        }
    }

    pOtpData->pLscData[1].colorTemperature = ISI_COLOR_TEMPERATURE_4000K;
    for (int i=0; i<ISI_OTP_LSC_TABLE_NUM; i++){
        for (int j=0; j<ISI_OTP_LSC_TABLE_NUM; j++){
            pOtpData->pLscData[1].r[i][j] = 1;
            pOtpData->pLscData[1].gr[i][j] = 1;
            pOtpData->pLscData[1].gb[i][j] = 1;
            pOtpData->pLscData[1].b[i][j] = 1;
        }
    }

    pOtpData->pLscData[2].colorTemperature = ISI_COLOR_TEMPERATURE_5800K;
    for (int i=0; i<ISI_OTP_LSC_TABLE_NUM; i++){
        for (int j=0; j<ISI_OTP_LSC_TABLE_NUM; j++){
            pOtpData->pLscData[2].r[i][j] = 2;
            pOtpData->pLscData[2].gr[i][j] = 2;
            pOtpData->pLscData[2].gb[i][j] = 2;
            pOtpData->pLscData[2].b[i][j] = 2;
        }
    }

    //awb data
    if (pOV13b10Ctx->pAwbData == NULL) {
        pOV13b10Ctx->pAwbData = (IsiOTPAWB_t *) osMalloc(pOtpData->awbNum * sizeof(IsiOTPAWB_t));
        if (!(pOV13b10Ctx->pAwbData)) {
            TRACE(OV13b10_ERROR, "%s: Can't allocate pOV13b10Ctx->pAwbData!\n", __func__);
            return (RET_OUTOFMEM);
        }
        MEMSET(pOV13b10Ctx->pAwbData, 0, pOtpData->awbNum * sizeof(IsiOTPAWB_t));
    }
    pOtpData->pAwbData = pOV13b10Ctx->pAwbData;
    pOtpData->pAwbData[0].colorTemperature = ISI_COLOR_TEMPERATURE_3100K;
    pOtpData->pAwbData[0].r = 3;
    pOtpData->pAwbData[0].gr = 3;
    pOtpData->pAwbData[0].gb = 3;
    pOtpData->pAwbData[0].b = 3;
    pOtpData->pAwbData[0].rgRatio = 3;
    pOtpData->pAwbData[0].bgRatio = 3;

    pOtpData->pAwbData[1].colorTemperature = ISI_COLOR_TEMPERATURE_4000K;
    pOtpData->pAwbData[1].r = 4;
    pOtpData->pAwbData[1].gr = 4;
    pOtpData->pAwbData[1].gb = 4;
    pOtpData->pAwbData[1].b = 4;
    pOtpData->pAwbData[1].rgRatio = 4;
    pOtpData->pAwbData[1].bgRatio = 4;

    pOtpData->pAwbData[2].colorTemperature = ISI_COLOR_TEMPERATURE_5800K;
    pOtpData->pAwbData[2].r = 5;
    pOtpData->pAwbData[2].gr = 5;
    pOtpData->pAwbData[2].gb = 5;
    pOtpData->pAwbData[2].b = 5;
    pOtpData->pAwbData[2].rgRatio = 5;
    pOtpData->pAwbData[2].bgRatio = 5;

    //GlodenAwb data
    if (pOV13b10Ctx->pGoldenAwbData == NULL) {
        pOV13b10Ctx->pGoldenAwbData = (IsiOTPAWB_t *) osMalloc(pOtpData->goldenAwbNum * sizeof(IsiOTPAWB_t));
        if (!(pOV13b10Ctx->pGoldenAwbData)) {
            TRACE(OV13b10_ERROR, "%s: Can't allocate pOV13b10Ctx->pGoldenAwbData!\n", __func__);
            return (RET_OUTOFMEM);
        }
        MEMSET(pOV13b10Ctx->pGoldenAwbData, 0, pOtpData->goldenAwbNum * sizeof(IsiOTPAWB_t));
    }
    pOtpData->pGoldenAwbData = pOV13b10Ctx->pGoldenAwbData;
    pOtpData->pGoldenAwbData[0].colorTemperature = ISI_COLOR_TEMPERATURE_3100K;
    pOtpData->pGoldenAwbData[0].r = 6;
    pOtpData->pGoldenAwbData[0].gr = 6;
    pOtpData->pGoldenAwbData[0].gb = 6;
    pOtpData->pGoldenAwbData[0].b = 6;
    pOtpData->pGoldenAwbData[0].rgRatio = 6;
    pOtpData->pGoldenAwbData[0].bgRatio = 6;

    pOtpData->pGoldenAwbData[1].colorTemperature = ISI_COLOR_TEMPERATURE_4000K;
    pOtpData->pGoldenAwbData[1].r = 7;
    pOtpData->pGoldenAwbData[1].gr = 7;
    pOtpData->pGoldenAwbData[1].gb = 7;
    pOtpData->pGoldenAwbData[1].b = 7;
    pOtpData->pGoldenAwbData[1].rgRatio = 7;
    pOtpData->pGoldenAwbData[1].bgRatio = 7;

    pOtpData->pGoldenAwbData[2].colorTemperature = ISI_COLOR_TEMPERATURE_5800K;
    pOtpData->pGoldenAwbData[2].r = 8;
    pOtpData->pGoldenAwbData[2].gr = 8;
    pOtpData->pGoldenAwbData[2].gb = 8;
    pOtpData->pGoldenAwbData[2].b = 8;
    pOtpData->pGoldenAwbData[2].rgRatio = 8;
    pOtpData->pGoldenAwbData[2].bgRatio = 8;

    //LightSource data
    if (pOV13b10Ctx->pLightSourceData == NULL) {
        pOV13b10Ctx->pLightSourceData = (IsiOTPLightSource_t *) osMalloc(pOtpData->lightSourceNum * sizeof(IsiOTPLightSource_t));
        if (!(pOV13b10Ctx->pLightSourceData)) {
            TRACE(OV13b10_ERROR, "%s: Can't allocate pOV13b10Ctx->pLightSourceData!\n", __func__);
            return (RET_OUTOFMEM);
        }
        MEMSET(pOV13b10Ctx->pLightSourceData, 0, pOtpData->lightSourceNum * sizeof(IsiOTPLightSource_t));
    }
    pOtpData->pLightSourceData = pOV13b10Ctx->pLightSourceData;
    pOtpData->pLightSourceData[0].colorTemperature = ISI_COLOR_TEMPERATURE_3100K;
    pOtpData->pLightSourceData[0].xCIE = 9;
    pOtpData->pLightSourceData[0].yCIE = 9;
    pOtpData->pLightSourceData[0].intensity = 9;

    pOtpData->pLightSourceData[1].colorTemperature = ISI_COLOR_TEMPERATURE_4000K;
    pOtpData->pLightSourceData[1].xCIE = 10;
    pOtpData->pLightSourceData[1].yCIE = 10;
    pOtpData->pLightSourceData[1].intensity = 10;

    pOtpData->pLightSourceData[2].colorTemperature = ISI_COLOR_TEMPERATURE_5800K;
    pOtpData->pLightSourceData[2].xCIE = 11;
    pOtpData->pLightSourceData[2].yCIE = 11;
    pOtpData->pLightSourceData[2].intensity = 11;

    //AF data
    pOtpData->focus.otpVersion = 1;
    pOtpData->focus.otpFocusEnable = true;
    pOtpData->focus.cdafOtp.minFocal = 1;
    pOtpData->focus.cdafOtp.maxFocal = 1024;
    for (int i=0; i<PDAF_OTP_FOCAL_SIZE; i++){
        pOtpData->focus.pdafOtp.pdFocal[i] = i;
    }

    TRACE(OV13b10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV13b10_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "OV13b10";
    TRACE(OV13b10_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = OV13b10_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = OV13b10_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = OV13b10_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = OV13b10_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = OV13b10_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = OV13b10_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = OV13b10_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = OV13b10_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = OV13b10_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = OV13b10_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = OV13b10_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = OV13b10_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = OV13b10_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = OV13b10_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = OV13b10_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = OV13b10_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = OV13b10_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = OV13b10_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = OV13b10_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = OV13b10_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = OV13b10_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = OV13b10_IsiGetIspStatusIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = OV13b10_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = OV13b10_IsiGetTpgIss;

        /* AF */
        pIsiSensor->pIsiFocusCreateIss                  = NULL;
        pIsiSensor->pIsiFocusReleaseIss                 = NULL;
        pIsiSensor->pIsiFocusGetCalibrateIss            = OV13b10_IsiFocusGetCalibrateIss;
        pIsiSensor->pIsiFocusSetIss                     = NULL;
        pIsiSensor->pIsiFocusGetIss                     = NULL;

        /* OTP */
        pIsiSensor->pIsiGetOtpDataIss                   = OV13b10_IsiGetOtpDataIss;
        /* metadata*/
        pIsiSensor->pIsiQueryMetadataAttrIss            = NULL;
        pIsiSensor->pIsiSetMetadataAttrEnableIss        = NULL;
        pIsiSensor->pIsiGetMetadataAttrEnableIss        = NULL;
        pIsiSensor->pIsiGetMetadataWinIss               = NULL;
        pIsiSensor->pIsiParserMetadataIss               = NULL;

    } else {
        result = RET_NULL_POINTER;
    }

    TRACE(OV13b10_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t OV13b10_IsiCamDrvConfig = {
    .cameraDriverID      = 0x560d,
    .pIsiGetSensorIss    = OV13b10_IsiGetSensorIss,
};
