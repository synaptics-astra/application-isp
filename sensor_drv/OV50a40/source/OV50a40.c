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
#include "OV50a40_priv.h"

CREATE_TRACER(OV50a40_INFO , "OV50a40: ", INFO,    1);
CREATE_TRACER(OV50a40_WARN , "OV50a40: ", WARNING, 1);
CREATE_TRACER(OV50a40_ERROR, "OV50a40: ", ERROR,   1);
CREATE_TRACER(OV50a40_DEBUG,     "OV50a40: ", INFO, 0);
CREATE_TRACER(OV50a40_REG_INFO , "OV50a40: ", INFO, 1);
CREATE_TRACER(OV50a40_REG_DEBUG, "OV50a40: ", INFO, 1);

#define OV50a40_MIN_GAIN_STEP    (1.0f/128.0f)  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain) */

/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pov50a40_mode_info[] = {
    {
        .index     = 0,
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
        .fps       = 7 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    }
};

static RESULT OV50a40_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pOV50a40Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(OV50a40_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV50a40_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV50a40_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(OV50a40_ERROR, "%s: NULL POINTER!\n", __func__);
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pOV50a40Ctx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(OV50a40_ERROR, "%s: hal write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV50a40_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV50a40_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);
    const OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pOV50a40Ctx->sensorMode), sizeof(pOV50a40Ctx->sensorMode));

    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT OV50a40_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);
    const OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pov50a40_mode_info)/sizeof(pov50a40_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pov50a40_mode_info)/sizeof(pov50a40_mode_info[0])); i++) {
        if (pov50a40_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pov50a40_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT OV50a40_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    if (pOV50a40Ctx == NULL) return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pOV50a40Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pOV50a40Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pOV50a40Ctx->sensorMode.size.width;
    pCaps->resolution.height = pOV50a40Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) malloc(sizeof(OV50a40_Context_t));
    if (!pOV50a40Ctx) {
        TRACE(OV50a40_ERROR, "%s: Can't allocate ov50a40 context\n",__func__);
        return (RET_OUTOFMEM);
    }
    MEMSET(pOV50a40Ctx, 0, sizeof(OV50a40_Context_t));

    pOV50a40Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pOV50a40Ctx->groupHold          = BOOL_FALSE;
    pOV50a40Ctx->oldGain            = 0;
    pOV50a40Ctx->oldIntegrationTime = 0;
    pOV50a40Ctx->configured         = BOOL_FALSE;
    pOV50a40Ctx->streaming          = BOOL_FALSE;
    pOV50a40Ctx->testPattern        = BOOL_FALSE;
    pOV50a40Ctx->isAfpsRun          = BOOL_FALSE;
    pOV50a40Ctx->sensorMode.index   = 0;

    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x6c;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pOV50a40Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(OV50a40_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pOV50a40Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pOV50a40Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pOV50a40Ctx;

    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_AecSetModeParameters(IsiSensorHandle_t handle, OV50a40_Context_t * pOV50a40Ctx)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0, regVal = 0;
    float again = 0, dgain = 0;
    TRACE(OV50a40_INFO, "%s%s: (enter)\n", __func__, pOV50a40Ctx->isAfpsRun ? "(AFPS)" : "");

    pOV50a40Ctx->aecIntegrationTimeIncrement = pOV50a40Ctx->oneLineExpTime;
    pOV50a40Ctx->aecMinIntegrationTime       = pOV50a40Ctx->oneLineExpTime * pOV50a40Ctx->minIntegrationLine;
    pOV50a40Ctx->aecMaxIntegrationTime       = pOV50a40Ctx->oneLineExpTime * pOV50a40Ctx->maxIntegrationLine;
    //pOV50a40Ctx->aecMaxIntegrationTime        = 0.04f;
    pOV50a40Ctx->aecMinVSIntegrationTime     = pOV50a40Ctx->oneLineExpTime * pOV50a40Ctx->minVSIntegrationLine;
    pOV50a40Ctx->aecMaxVSIntegrationTime     = pOV50a40Ctx->oneLineExpTime * pOV50a40Ctx->maxVSIntegrationLine;

    TRACE(OV50a40_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pOV50a40Ctx->isAfpsRun ? "(AFPS)" : "",pOV50a40Ctx->aecMaxIntegrationTime);

    pOV50a40Ctx->aecGainIncrement = OV50a40_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pOV50a40Ctx->oldGain               = 0;
    pOV50a40Ctx->oldIntegrationTime    = 0;

    OV50a40_IsiReadRegIss(handle, 0x3508, &value);
    regVal = (value & 0x3f) << 8;
    OV50a40_IsiReadRegIss(handle, 0x3509, &value);
    regVal = regVal | (value & 0xFF);
    again = regVal/128.0f;
    
    OV50a40_IsiReadRegIss(handle, 0x350a, &value);
    regVal = (value & 0x3f) << 8;
    OV50a40_IsiReadRegIss(handle, 0x350b, &value);
    regVal = regVal | (value & 0xFF);
    dgain = regVal/1024.0f;
    pOV50a40Ctx->aecCurGain            = again * dgain;

    OV50a40_IsiReadRegIss(handle, 0x3501, &value);
    regVal = value << 8;
    OV50a40_IsiReadRegIss(handle, 0x3502, &value);
    regVal = regVal | (value & 0xFF);
    pOV50a40Ctx->aecCurIntegrationTime = regVal * pOV50a40Ctx->oneLineExpTime;

    TRACE(OV50a40_INFO, "%s%s: (exit)\n", __func__,pOV50a40Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT OV50a40_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    if (!pOV50a40Ctx) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOV50a40Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pOV50a40Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pov50a40_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (pov50a40_mode_info[i].index == pOV50a40Ctx->sensorMode.index) {
            SensorDefaultMode = &(pov50a40_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(OV50a40_mipi4lane_720p_init) / sizeof(OV50a40_mipi4lane_720p_init[0]); i++) {
                    OV50a40_IsiWriteRegIss(handle, OV50a40_mipi4lane_720p_init[i][0], OV50a40_mipi4lane_720p_init[i][1]);
                }
                break;
            default:
                TRACE(OV50a40_INFO, "%s:not support sensor mode %d\n", __func__,pOV50a40Ctx->sensorMode.index);
                return RET_NOTSUPP;
                break;
    }

        memcpy(&(pOV50a40Ctx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));

    } else {
        TRACE(OV50a40_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pOV50a40Ctx->sensorMode.index) {
        case 0:
            pOV50a40Ctx->oneLineExpTime   = 0.00014389;
            pOV50a40Ctx->frameLengthLines    = 0xa64;
            pOV50a40Ctx->curFrameLengthLines = pOV50a40Ctx->frameLengthLines;
            pOV50a40Ctx->maxIntegrationLine  = pOV50a40Ctx->frameLengthLines -32 ;
            pOV50a40Ctx->minIntegrationLine  = 1;
            pOV50a40Ctx->aecMaxGain          = 230;
            pOV50a40Ctx->aecMinGain          = 1.0;
            pOV50a40Ctx->aGain.min           = 1.0;
            pOV50a40Ctx->aGain.max           = 15.5;
            pOV50a40Ctx->aGain.step          = (1.0f/128.0f);
            pOV50a40Ctx->dGain.min           = 1.0;
            pOV50a40Ctx->dGain.max           = 15;
            pOV50a40Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        default:
            TRACE(OV50a40_INFO, "%s:not support sensor mode %d\n", __func__,pOV50a40Ctx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    pOV50a40Ctx->maxFps  = pOV50a40Ctx->sensorMode.fps;
    pOV50a40Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pOV50a40Ctx->currFps = pOV50a40Ctx->maxFps;

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(OV50a40_DEBUG, "%s: OV50a40 System-Reset executed\n", __func__);
    osSleep(100);

    result = OV50a40_AecSetModeParameters(handle, pOV50a40Ctx);
    if (result != RET_SUCCESS) {
        TRACE(OV50a40_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pOV50a40Ctx->configured = BOOL_TRUE;
    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT OV50a40_IsiCloseIss(IsiSensorHandle_t handle)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    if (pOV50a40Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)OV50a40_IsiSetStreamingIss(pOV50a40Ctx, BOOL_FALSE);

    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_IsiReleaseIss(IsiSensorHandle_t handle)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    if (pOV50a40Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pOV50a40Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pOV50a40Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pOV50a40Ctx, 0, sizeof(OV50a40_Context_t));
    free(pOV50a40Ctx);
    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;

    uint32_t sensorId = 0;
    uint32_t correctId = 0x5650;

    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = OV50a40_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(OV50a40_ERROR, "%s: Read Sensor ID Error! \n",__func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(OV50a40_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n", __func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(OV50a40_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__, correctId, sensorId);
    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (!pValue) return (RET_NULL_POINTER);

    regVal   = 0;
    result    = OV50a40_IsiReadRegIss(handle, 0x300a, &regVal);
    sensorId = (regVal & 0xff) << 8;

    regVal   = 0;
    result   |= OV50a40_IsiReadRegIss(handle, 0x300b, &regVal);
    sensorId |= (regVal & 0xff);

    *pValue = sensorId;
    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV50a40Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    result = OV50a40_IsiWriteRegIss(handle, 0x0100, on);
    if (result != RET_SUCCESS) {
        TRACE(OV50a40_ERROR, "%s: set sensor streaming error! \n",__func__);
        return (RET_FAILURE);
    }

    pOV50a40Ctx->streaming = on;

    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    if (pOV50a40Ctx == NULL) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(OV50a40_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    //get time limit and total gain limit
    pAeBaseInfo->gain.min        = pOV50a40Ctx->aecMinGain;
    pAeBaseInfo->gain.max        = pOV50a40Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pOV50a40Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pOV50a40Ctx->aecMaxIntegrationTime;

    pAeBaseInfo->vsGain.min      = pOV50a40Ctx->aecMinGain;
    pAeBaseInfo->vsGain.max      = pOV50a40Ctx->aecMaxGain;
    pAeBaseInfo->vsIntTime.min   = pOV50a40Ctx->aecMinVSIntegrationTime;
    pAeBaseInfo->vsIntTime.max   = pOV50a40Ctx->aecMaxVSIntegrationTime;

    //get again/dgain info
    pAeBaseInfo->aGain           = pOV50a40Ctx->aGain;
    pAeBaseInfo->dGain           = pOV50a40Ctx->dGain;
    pAeBaseInfo->aVSGain         = pOV50a40Ctx->aVSGain;
    pAeBaseInfo->dVSGain         = pOV50a40Ctx->dVSGain;
    
    pAeBaseInfo->aecCurGain      = pOV50a40Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime   = pOV50a40Ctx->aecCurIntegrationTime;
    //pAeBaseInfo->aecCurVSGain    = (pOV50a40Ctx->curVSAgain) * (pOV50a40Ctx->curVSDgain);
    //pAeBaseInfo->aecCurVSIntTime = pOV50a40Ctx->aecCurVSIntegrationTime;
    //pAeBaseInfo->aecCurLongGain  = 0;
    pAeBaseInfo->aecGainStep     = pOV50a40Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep  = pOV50a40Ctx->aecIntegrationTimeIncrement;
    pAeBaseInfo->stitchingMode   = pOV50a40Ctx->sensorMode.stitchingMode;

    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);
    uint32_t again = 0, vsagain = 0;

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV50a40Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        again = (uint32_t)(pSensorAGain->gain[ISI_LINEAR_PARAS] * 128);
        result = OV50a40_IsiWriteRegIss(handle, 0x3508,(again & 0x3f00)>>8);
        result |= OV50a40_IsiWriteRegIss(handle, 0x3509,(again & 0xff));
        pOV50a40Ctx->curAgain = again/128.0f;   

    } else if (pOV50a40Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        again = (uint32_t)(pSensorAGain->gain[ISI_DUAL_EXP_L_PARAS] * 128);
        result = OV50a40_IsiWriteRegIss(handle, 0x3508,(again & 0x3f00)>>8);
        result |= OV50a40_IsiWriteRegIss(handle, 0x3509,(again & 0xff));
        pOV50a40Ctx->curAgain = again/128.0f;

        vsagain = (uint32_t)(pSensorAGain->gain[ISI_DUAL_EXP_S_PARAS] * 128);
        result |=  OV50a40_IsiWriteRegIss(handle, 0x350c,(vsagain & 0x3f00)>>8);
        result |= OV50a40_IsiWriteRegIss(handle, 0x350d,(vsagain & 0x00ff));
        pOV50a40Ctx->curVSAgain = vsagain/128.0f;

    }  else {
        TRACE(OV50a40_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);
    uint32_t dgain = 0, vsdgain = 0;

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV50a40Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        dgain = (uint32_t)(pSensorDGain->gain[ISI_LINEAR_PARAS] * 1024);
        result = OV50a40_IsiWriteRegIss(handle, 0x350a, (dgain >> 8) & 0x3f);
        result |= OV50a40_IsiWriteRegIss(handle,0x350b, (dgain & 0xff));
        pOV50a40Ctx->curDgain = dgain/1024.0f;
        pOV50a40Ctx->aecCurGain = pOV50a40Ctx->curAgain * pOV50a40Ctx->curDgain;

    } else if (pOV50a40Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        dgain = (uint32_t)(pSensorDGain->gain[ISI_DUAL_EXP_L_PARAS] * 1024);
        result = OV50a40_IsiWriteRegIss(handle, 0x350a, (dgain >> 8) & 0x3f);
        result |= OV50a40_IsiWriteRegIss(handle,0x350b, (dgain & 0xff));
        pOV50a40Ctx->curDgain = dgain/1024.0f;
        pOV50a40Ctx->aecCurGain = pOV50a40Ctx->curAgain * pOV50a40Ctx->curDgain;

        vsdgain = (uint32_t)(pSensorDGain->gain[ISI_DUAL_EXP_S_PARAS] * 1024);
        result |=  OV50a40_IsiWriteRegIss(handle, 0x350e, (vsdgain >> 8) & 0x3f);
        result |=  OV50a40_IsiWriteRegIss(handle, 0x350f, (vsdgain & 0xff));
        pOV50a40Ctx->curVSDgain = vsdgain/1024.0f;

    }  else {
        TRACE(OV50a40_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    if (pOV50a40Ctx == NULL) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    if(pOV50a40Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        pSensorAGain->gain[ISI_LINEAR_PARAS]       = pOV50a40Ctx->curAgain;

    } else if (pOV50a40Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        pSensorAGain->gain[ISI_DUAL_EXP_L_PARAS]   = pOV50a40Ctx->curAgain;
        pSensorAGain->gain[ISI_DUAL_EXP_S_PARAS]   = pOV50a40Ctx->curVSAgain;

    } else {
        TRACE(OV50a40_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    if (pOV50a40Ctx == NULL) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    if (pOV50a40Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        pSensorDGain->gain[ISI_LINEAR_PARAS] = pOV50a40Ctx->curDgain;

    } else if (pOV50a40Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        pSensorDGain->gain[ISI_DUAL_EXP_L_PARAS]  = pOV50a40Ctx->curDgain;
        pSensorDGain->gain[ISI_DUAL_EXP_S_PARAS]  = pOV50a40Ctx->curVSDgain;

    } else {
        TRACE(OV50a40_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;

    if (!pOV50a40Ctx) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOV50a40Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        result = OV50a40_SetIntTime(handle, pSensorIntTime->intTime[ISI_LINEAR_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(OV50a40_INFO, "%s: set sensor IntTime[ISI_LINEAR_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

    } else if (pOV50a40Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        result = OV50a40_SetIntTime(handle, pSensorIntTime->intTime[ISI_DUAL_EXP_L_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(OV50a40_INFO, "%s: set sensor IntTime[ISI_DUAL_EXP_L_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

        result = OV50a40_SetVSIntTime(handle, pSensorIntTime->intTime[ISI_DUAL_EXP_S_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(OV50a40_INFO, "%s: set sensor IntTime[ISI_DUAL_EXP_S_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

    }  else {
        TRACE(OV50a40_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_SetIntTime(IsiSensorHandle_t handle, float newIntegrationTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pOV50a40Ctx) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = newIntegrationTime / pOV50a40Ctx->oneLineExpTime;
    expLine = MIN(pOV50a40Ctx->maxIntegrationLine,MAX(4, expLine));
    TRACE(OV50a40_DEBUG, "%s: set expLine = 0x%04x\n", __func__, expLine);

    result =  OV50a40_IsiWriteRegIss(handle, 0x3501,(expLine >>8) & 0xff);
    result |= OV50a40_IsiWriteRegIss(handle, 0x3502,(expLine & 0xff));

    pOV50a40Ctx->aecCurIntegrationTime = expLine * pOV50a40Ctx->oneLineExpTime;

    TRACE(OV50a40_DEBUG, "%s: set IntTime = %f\n", __func__, pOV50a40Ctx->aecCurIntegrationTime);
    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_SetVSIntTime(IsiSensorHandle_t handle, float newIntegrationTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pOV50a40Ctx) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = newIntegrationTime / pOV50a40Ctx->oneLineExpTime;
#ifdef ISP_MI_HDR
    expLine = MIN(550/2, MAX(4, expLine)); // 40ms
#else
    expLine = MIN(51, MAX(1, expLine));   //61
#endif

    result =  OV50a40_IsiWriteRegIss(handle, 0x3511,(expLine >> 8) & 0xff);
    result |= OV50a40_IsiWriteRegIss(handle, 0x3512,(expLine & 0xff));

    pOV50a40Ctx->aecCurVSIntegrationTime = expLine * pOV50a40Ctx->oneLineExpTime;

    TRACE(OV50a40_DEBUG, "%s: set VSIntTime = %f\n", __func__, pOV50a40Ctx->aecCurVSIntegrationTime);
    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    if (!pOV50a40Ctx) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    if (pOV50a40Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        pSensorIntTime->intTime[ISI_LINEAR_PARAS] = pOV50a40Ctx->aecCurIntegrationTime;

    } else if (pOV50a40Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        pSensorIntTime->intTime[ISI_DUAL_EXP_L_PARAS]   = pOV50a40Ctx->aecCurIntegrationTime;
        pSensorIntTime->intTime[ISI_DUAL_EXP_S_PARAS]   = pOV50a40Ctx->aecCurVSIntegrationTime;

    } else {
        TRACE(OV50a40_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t * pFps)
{
    const OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    if (pOV50a40Ctx == NULL) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pOV50a40Ctx->currFps;

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    RESULT result = RET_SUCCESS;
    int32_t NewVts = 0;

    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

   if (fps > pOV50a40Ctx->maxFps) {
        TRACE(OV50a40_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, fps, pOV50a40Ctx->maxFps, pOV50a40Ctx->minFps,pOV50a40Ctx->maxFps);
        fps = pOV50a40Ctx->maxFps;
    }
    if (fps < pOV50a40Ctx->minFps) {
        TRACE(OV50a40_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, fps, pOV50a40Ctx->minFps, pOV50a40Ctx->minFps,pOV50a40Ctx->maxFps);
        fps = pOV50a40Ctx->minFps;
    }
     
    NewVts = pOV50a40Ctx->frameLengthLines*pOV50a40Ctx->sensorMode.fps / fps;
    result =  OV50a40_IsiWriteRegIss(handle, 0x380e, NewVts >> 8);
    result |=  OV50a40_IsiWriteRegIss(handle, 0x380f, NewVts & 0xff);
    pOV50a40Ctx->currFps              = fps;
    pOV50a40Ctx->curFrameLengthLines  = NewVts;
    pOV50a40Ctx->maxIntegrationLine   = pOV50a40Ctx->curFrameLengthLines - 8;
    //pOV50a40Ctx->aecMaxIntegrationTime = pOV50a40Ctx->maxIntegrationLine * pOV50a40Ctx->oneLineExpTime;

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    const OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    pIspStatus->useSensorAWB = false;

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT OV50a40_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV50a40Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = OV50a40_IsiWriteRegIss(handle, 0x50c1, 0x00);
    } else {
        result = OV50a40_IsiWriteRegIss(handle, 0x50c1, 0x01);
    }

    pOV50a40Ctx->testPattern = tpg.enable;

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV50a40Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!OV50a40_IsiReadRegIss(handle, 0x50c1,&value)) {
        pTpg->enable = ((value & 0x01) != 0) ? 1 : 0;
        if(pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pOV50a40Ctx->testPattern = pTpg->enable;
    }

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "OV50a40";
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = OV50a40_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = OV50a40_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = OV50a40_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = OV50a40_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = OV50a40_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = OV50a40_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = OV50a40_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = OV50a40_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = OV50a40_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = OV50a40_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = OV50a40_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = OV50a40_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = OV50a40_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = OV50a40_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = OV50a40_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = OV50a40_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = OV50a40_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = OV50a40_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = OV50a40_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = OV50a40_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = OV50a40_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = OV50a40_IsiGetIspStatusIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = OV50a40_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = OV50a40_IsiGetTpgIss;

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

    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t OV50a40_IsiCamDrvConfig = {
    .cameraDriverID      = 0x5650,
    .pIsiGetSensorIss    = OV50a40_IsiGetSensorIss,
};
