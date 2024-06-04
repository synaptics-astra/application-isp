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
#include "OV2778_priv.h"

CREATE_TRACER( OV2778_INFO , "OV2778: ", INFO,    1);
CREATE_TRACER( OV2778_WARN , "OV2778: ", WARNING, 1);
CREATE_TRACER( OV2778_ERROR, "OV2778: ", ERROR,   1);
CREATE_TRACER( OV2778_DEBUG,     "OV2778: ", INFO, 1);
CREATE_TRACER( OV2778_REG_INFO , "OV2778: ", INFO, 1);
CREATE_TRACER( OV2778_REG_DEBUG, "OV2778: ", INFO, 1);

#define OV2778_MIN_GAIN_STEP    ( 1.0f/256.0f )  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain ) */

/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pov2778_mode_info[] = {
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
        .bayerPattern = ISI_BPAT_GRIRG,
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
        .hdrMode = ISI_SENSOR_MODE_HDR_NATIVE,
        .nativeMode = ISI_SENSOR_NATIVE_DCG,
        .bitWidth = 12,
        .compress.enable = 1,
        .compress.xBit  = 16,
        .compress.yBit  = 12,
        .bayerPattern = ISI_BPAT_GRIRG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
};

static RESULT OV2778_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue) 
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s (enter)\n", __func__);

    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL || pOV2778Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pOV2778Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(OV2778_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV2778_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV2778_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value) 
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s (enter)\n", __func__);

    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL || pOV2778Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pOV2778Ctx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(OV2778_ERROR, "%s: hal write sensor register error!\n",__func__);
        return (RET_FAILURE);
    }

    TRACE(OV2778_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV2778_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(OV2778_INFO, "%s (enter)\n", __func__);
    const OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pOV2778Ctx->sensorMode), sizeof(pOV2778Ctx->sensorMode));

    TRACE(OV2778_INFO, "%s (exit)\n", __func__);
    return ( RET_SUCCESS );
}

static  RESULT OV2778_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(OV2778_INFO, "%s (enter)\n", __func__);
    const OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pov2778_mode_info)/sizeof(pov2778_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pov2778_mode_info)/sizeof(pov2778_mode_info[0])); i++) {
        if (pov2778_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pov2778_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(OV2778_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT OV2778_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps) 
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s (enter)\n", __func__);
    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;

    if (pOV2778Ctx == NULL)
        return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pOV2778Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pOV2778Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pOV2778Ctx->sensorMode.size.width;
    pCaps->resolution.height = pOV2778Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(OV2778_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV2778_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle) 
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s (enter)\n", __func__);

    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) malloc(sizeof(OV2778_Context_t));
    if (!pOV2778Ctx) {
        TRACE(OV2778_ERROR, "%s: Can't allocate ov2778 context\n", __func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pOV2778Ctx, 0, sizeof(OV2778_Context_t));

    pOV2778Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pOV2778Ctx->groupHold          = BOOL_FALSE;
    pOV2778Ctx->oldGain            = 0;
    pOV2778Ctx->oldIntegrationTime = 0;
    pOV2778Ctx->configured         = BOOL_FALSE;
    pOV2778Ctx->streaming          = BOOL_FALSE;
    pOV2778Ctx->testPattern        = BOOL_FALSE;
    pOV2778Ctx->isAfpsRun          = BOOL_FALSE;
    pOV2778Ctx->sensorMode.index   = 0;
    
    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x6c;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pOV2778Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pOV2778Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(OV2778_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pOV2778Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pOV2778Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pOV2778Ctx;

    TRACE(OV2778_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV2778_AecSetModeParameters(IsiSensorHandle_t handle, OV2778_Context_t *pOV2778Ctx) 
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0, regVal = 0, again = 0;
    float dgain = 0;
    TRACE(OV2778_INFO, "%s%s: (enter)\n", __func__, pOV2778Ctx->isAfpsRun ? "(AFPS)" : "");

    pOV2778Ctx->aecIntegrationTimeIncrement = pOV2778Ctx->oneLineExpTime;
    pOV2778Ctx->aecMinIntegrationTime = pOV2778Ctx->oneLineExpTime * pOV2778Ctx->minIntegrationLine;
    //pOV2778Ctx->aecMaxIntegrationTime = pOV2778Ctx->oneLineExpTime * pOV2778Ctx->maxIntegrationLine;
    pOV2778Ctx->aecMaxIntegrationTime = 0.04;

    TRACE(OV2778_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pOV2778Ctx->isAfpsRun ? "(AFPS)" : "", pOV2778Ctx->aecMaxIntegrationTime);

    pOV2778Ctx->aecGainIncrement = OV2778_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pOV2778Ctx->oldGain = 0;
    pOV2778Ctx->oldIntegrationTime = 0;

    OV2778_IsiReadRegIss(handle, 0x30b6, &value);
    regVal = value << 8;
    OV2778_IsiReadRegIss(handle, 0x30b7, &value);
    regVal = regVal | (value & 0xFF);
    pOV2778Ctx->aecCurIntegrationTime = regVal * pOV2778Ctx->oneLineExpTime;

    if(pOV2778Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR){
        OV2778_IsiReadRegIss(handle, 0x315a, &value);
        regVal = (value & 0xff) << 8;
        OV2778_IsiReadRegIss(handle, 0x315b, &value);
        regVal = regVal | (value & 0xFF);
        dgain = regVal/256.0f;

        OV2778_IsiReadRegIss(handle, 0x30bb, &value);
        regVal = value & 0x03;
        again = 1 << regVal;
        pOV2778Ctx->aecCurGain = again * dgain;
    } else {
        OV2778_IsiReadRegIss(handle, 0x315c, &value);
        regVal = (value & 0xff) << 8;
        OV2778_IsiReadRegIss(handle, 0x315d, &value);
        regVal = regVal | (value & 0xFF);
        dgain = regVal/256.0f;

        OV2778_IsiReadRegIss(handle, 0x30bb, &value);
        regVal = value & 0x0c;
        again = 1 << regVal;
        pOV2778Ctx->aecCurGain = again * dgain;
    }

    TRACE(OV2778_INFO, "%s%s: (exit)\n", __func__, pOV2778Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT OV2778_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode) 
{
    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OV2778_INFO, "%s (enter)\n", __func__);

    if (!pOV2778Ctx) {
        TRACE(OV2778_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOV2778Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pOV2778Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pov2778_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (pov2778_mode_info[i].index == pOV2778Ctx->sensorMode.index) {
            SensorDefaultMode = &(pov2778_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(OV2778_mipi4lane_1080p_init) / sizeof(OV2778_mipi4lane_1080p_init[0]); i++) {
                    OV2778_IsiWriteRegIss(handle, OV2778_mipi4lane_1080p_init[i][0], OV2778_mipi4lane_1080p_init[i][1]);
                }
                break;
            case 1: 
                for (int i = 0; i<sizeof(OV2778_mipi4lane_1080p_native2dol_init) / sizeof(OV2778_mipi4lane_1080p_native2dol_init[0]); i++) {
                    OV2778_IsiWriteRegIss(handle, OV2778_mipi4lane_1080p_native2dol_init[i][0], OV2778_mipi4lane_1080p_native2dol_init[i][1]);
                }
                break;
            default:
                break;
        }

        memcpy(&(pOV2778Ctx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));
    } else {
        TRACE(OV2778_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pOV2778Ctx->sensorMode.index) {
        case 0:
            pOV2778Ctx->oneLineExpTime      = 0.000069607;
            pOV2778Ctx->frameLengthLines    = 0x466;
            pOV2778Ctx->curFrameLengthLines = pOV2778Ctx->frameLengthLines;
            pOV2778Ctx->maxIntegrationLine  = pOV2778Ctx->curFrameLengthLines - 2;
            pOV2778Ctx->minIntegrationLine  = 1;
            pOV2778Ctx->aecMaxGain          = 24;
            pOV2778Ctx->aecMinGain          = 3;
            pOV2778Ctx->aGain.min           = 3.0;
            pOV2778Ctx->aGain.max           = 24;
            pOV2778Ctx->aGain.step          = (1.0f/256.0f);
            pOV2778Ctx->dGain.min           = 1.0;
            pOV2778Ctx->dGain.max           = 1.0;
            pOV2778Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 1:
            pOV2778Ctx->oneLineExpTime      = 0.000069607;
            pOV2778Ctx->frameLengthLines    = 0x466;
            pOV2778Ctx->curFrameLengthLines = pOV2778Ctx->frameLengthLines;
            pOV2778Ctx->maxIntegrationLine  = pOV2778Ctx->curFrameLengthLines - 4;
            pOV2778Ctx->minIntegrationLine  = 1;
            pOV2778Ctx->aecMaxGain          = 24;
            pOV2778Ctx->aecMinGain          = 3;
            pOV2778Ctx->aGain.min           = 3.0;
            pOV2778Ctx->aGain.max           = 24;
            pOV2778Ctx->aGain.step          = (1.0f/256.0f);
            pOV2778Ctx->dGain.min           = 1.0;
            pOV2778Ctx->dGain.max           = 1.0;
            pOV2778Ctx->dGain.step          = (1.0f/256.0f);
            break;
        default:
            return ( RET_NOTAVAILABLE );
            break;
    }

    pOV2778Ctx->maxFps  = pOV2778Ctx->sensorMode.fps;
    pOV2778Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pOV2778Ctx->currFps = pOV2778Ctx->maxFps;

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(OV2778_DEBUG, "%s: OV2778 System-Reset executed\n", __func__);
    osSleep(100);

    result = OV2778_AecSetModeParameters(handle, pOV2778Ctx);
    if (result != RET_SUCCESS) {
        TRACE(OV2778_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pOV2778Ctx->configured = BOOL_TRUE;
    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT OV2778_IsiCloseIss(IsiSensorHandle_t handle)
{
    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s (enter)\n", __func__);

    if (pOV2778Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)OV2778_IsiSetStreamingIss(pOV2778Ctx, BOOL_FALSE);

    TRACE(OV2778_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV2778_IsiReleaseIss(IsiSensorHandle_t handle)
{
    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s (enter)\n", __func__);

    if (pOV2778Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pOV2778Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pOV2778Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pOV2778Ctx, 0, sizeof(OV2778_Context_t));
    free(pOV2778Ctx);
    TRACE(OV2778_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV2778_IsiCheckConnectionIss(IsiSensorHandle_t handle) 
{
    RESULT result = RET_SUCCESS;
    uint32_t correctId = 0x2770;
    uint32_t sensorId = 0;

    TRACE(OV2778_INFO, "%s (enter)\n", __func__);

    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL || pOV2778Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = OV2778_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(OV2778_ERROR, "%s: Read Sensor ID Error! \n", __func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(OV2778_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n", __func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(OV2778_INFO, "%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__, correctId, sensorId);
    TRACE(OV2778_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV2778_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue) 
{
    RESULT result = RET_SUCCESS;
    uint16_t reg_val;
    uint32_t sensorId;

    TRACE(OV2778_INFO, "%s (enter)\n", __func__);

    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL || pOV2778Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (!pValue)
        return (RET_NULL_POINTER);

    reg_val = 0;
    result = OV2778_IsiReadRegIss(handle, 0x300a, &reg_val);
    sensorId = (reg_val & 0xff) << 8;

    reg_val = 0;
    result |= OV2778_IsiReadRegIss(handle, 0x300b, &reg_val);
    sensorId |= (reg_val & 0xff);

    *pValue = sensorId;
    TRACE(OV2778_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV2778_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on) 
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s (enter)\n", __func__);

    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL || pOV2778Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV2778Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    result = OV2778_IsiWriteRegIss(handle, 0x3012, on);
    if (result != RET_SUCCESS) {
        TRACE(OV2778_ERROR, "%s: set sensor streaming error! \n",__func__);
        return (RET_FAILURE);
    }

    pOV2778Ctx->streaming = on;

    TRACE(OV2778_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV2778_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    const OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    if (pOV2778Ctx == NULL) {
        TRACE(OV2778_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(OV2778_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    //get time limit and total gain limit
    pAeBaseInfo->gain.min        = pOV2778Ctx->aecMinGain;
    pAeBaseInfo->gain.max        = pOV2778Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pOV2778Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pOV2778Ctx->aecMaxIntegrationTime;

    //get again/dgain info
    pAeBaseInfo->aGain           = pOV2778Ctx->aGain;
    pAeBaseInfo->dGain           = pOV2778Ctx->dGain;
    
    pAeBaseInfo->aecCurGain      = pOV2778Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime   = pOV2778Ctx->aecCurIntegrationTime;
    pAeBaseInfo->aecGainStep     = pOV2778Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep  = pOV2778Ctx->aecIntegrationTimeIncrement;

    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT OV2778_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);
    float newGain = 0.0f, gain = 0.0f, dGainHcg = 0.0f, dGainLcg = 0.0f;
    uint32_t gainHcg = 0, gainLcg = 0, againHcg = 0, againLcg = 0;
    uint16_t data = 0;

    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL || pOV2778Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }
    
    newGain = pSensorAGain->gain[ISI_LINEAR_PARAS];
    newGain = MAX(pOV2778Ctx->aecMinGain, MIN(newGain, pOV2778Ctx->aecMaxGain));

    if(pOV2778Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        if (newGain < 3.0) {
            newGain = 3.0;
            dGainHcg = newGain;
            againHcg = 0x00;
        } else if (newGain < 6.0 && newGain >= 3.0) {
            dGainHcg = newGain;
            againHcg = 0x00;
        } else if (newGain < 12.0 && newGain >= 6.0) {
            dGainHcg = newGain / 2.0;
            againHcg = 0x01;
        } else if (newGain < 24.0 && newGain >= 12.0) {
            dGainHcg = newGain / 4.0;
            againHcg = 0x02;
        } else if (newGain >= 24.0) {
            dGainHcg = newGain / 8.0;
            againHcg = 0x03;
        }
        gainHcg = (uint32_t)(dGainHcg * 256);

        OV2778_IsiReadRegIss(handle, 0x30bb, &data);
        //data = ((data&0x00) | (againHcg));
        data = againHcg;

        result = OV2778_IsiWriteRegIss(handle, 0x3467, 0x00);
        result |= OV2778_IsiWriteRegIss(handle, 0x3464, 0x04);

        result |= OV2778_IsiWriteRegIss(handle, 0x315a, ((gainHcg >> 8) & 0x00ff));//dgain
        result |= OV2778_IsiWriteRegIss(handle, 0x315b, (gainHcg & 0x00ff));

        result |= OV2778_IsiWriteRegIss(handle, 0x30bb, data);//again

        result |= OV2778_IsiWriteRegIss(handle, 0x3464, 0x14);
        result |= OV2778_IsiWriteRegIss(handle, 0x3467, 0x01);

        pOV2778Ctx->curAgain = 1<<againHcg;
        pOV2778Ctx->curDgain = gainHcg/256.0f;
        pOV2778Ctx->aecCurGain = (1<<againHcg) * (gainHcg/256.0f);

    } else if (pOV2778Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        gain = newGain * 1;
        gain = MAX(pOV2778Ctx->aecMinGain, MIN(gain, pOV2778Ctx->aecMaxGain));
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
            againLcg = 0x01;
        } else if (newGain>=3.0 && newGain < 4.0) {
            dGainLcg = newGain / 2.0;
            againLcg = 0x01;
        } else if (newGain>=4.0 && newGain < 8.0 ) {
            dGainLcg = newGain / 4.0;
            againLcg = 0x02;
        } else {
            dGainLcg = newGain / 8.0;
            againLcg = 0x03;
        }

        gainLcg = (uint32_t)(dGainLcg  * 256);
        gainHcg = (uint32_t)(dGainHcg  * 256);
        OV2778_IsiReadRegIss(handle, 0x30bb, &data);
        data &= ~0x03;
        data |= againHcg & 0x03;//again_hcg, 30bb[1:0]
        data &= ~(0x03 << 2);
        data |= (againLcg & 0x03) << 2;//again_lcg, 30bb[3:2]

        result = OV2778_IsiWriteRegIss(handle, 0x3467, 0x00);
        result |= OV2778_IsiWriteRegIss(handle, 0x3464, 0x04);

        result |= OV2778_IsiWriteRegIss(handle, 0x315a, (gainHcg >> 8) & 0xff);
        result |= OV2778_IsiWriteRegIss(handle, 0x315b, gainHcg & 0xff);

        result |= OV2778_IsiWriteRegIss(handle, 0x315c, (gainLcg >> 8) & 0xff);
        result |= OV2778_IsiWriteRegIss(handle, 0x315d, gainLcg & 0xff);

        result |= OV2778_IsiWriteRegIss(handle, 0x30bb, data);

        result |= OV2778_IsiWriteRegIss(handle, 0x3464, 0x14);
        result |= OV2778_IsiWriteRegIss(handle, 0x3467, 0x01);

        pOV2778Ctx->curAgain = 1<<againLcg;
        pOV2778Ctx->curDgain = gainLcg/256.0f;
        pOV2778Ctx->aecCurGain = (1<<againLcg) * (gainLcg/256.0f);

    } else {
        TRACE(OV2778_INFO, "%s:not support this hdr_mode.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2778_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2778_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    const OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    if (pOV2778Ctx == NULL) {
        TRACE(OV2778_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorAGain->gain[ISI_LINEAR_PARAS] = pOV2778Ctx->curAgain;

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2778_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    const OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    if (pOV2778Ctx == NULL) {
        TRACE(OV2778_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorDGain->gain[ISI_LINEAR_PARAS] = pOV2778Ctx->curDgain;

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2778_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);
    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pOV2778Ctx) {
        TRACE(OV2778_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = pSensorIntTime->intTime[ISI_LINEAR_PARAS] / pOV2778Ctx->oneLineExpTime;
    expLine = MIN(pOV2778Ctx->maxIntegrationLine, MAX(pOV2778Ctx->minIntegrationLine, expLine));
    TRACE(OV2778_DEBUG, "%s: set expLine=0x%04x\n", __func__, expLine);

    result = OV2778_IsiWriteRegIss(handle, 0x3467, 0x00);
    result |= OV2778_IsiWriteRegIss(handle, 0x3464, 0x04);

    result |= OV2778_IsiWriteRegIss(handle, 0x30b6, (expLine >> 8) & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x30b7, expLine & 0xff);

    result |= OV2778_IsiWriteRegIss(handle, 0x3464, 0x14);
    result |= OV2778_IsiWriteRegIss(handle, 0x3467, 0x01);

    pOV2778Ctx->aecCurIntegrationTime = expLine * pOV2778Ctx->oneLineExpTime;

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2778_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    const OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    if (!pOV2778Ctx) {
        TRACE(OV2778_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    pSensorIntTime->intTime[ISI_LINEAR_PARAS] = pOV2778Ctx->aecCurIntegrationTime;

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2778_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t *pFps)
{
    const OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    if (pOV2778Ctx == NULL) {
        TRACE(OV2778_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pOV2778Ctx->currFps;

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2778_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    if (pOV2778Ctx == NULL) {
        TRACE(OV2778_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (fps > pOV2778Ctx->maxFps) {
        TRACE(OV2778_ERROR, "%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, fps, pOV2778Ctx->maxFps, pOV2778Ctx->minFps, pOV2778Ctx->maxFps);
        fps = pOV2778Ctx->maxFps;
    }
    if (fps < pOV2778Ctx->minFps) {
        TRACE(OV2778_ERROR, "%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, fps, pOV2778Ctx->minFps, pOV2778Ctx->minFps, pOV2778Ctx->maxFps);
        fps = pOV2778Ctx->minFps;
    }

    uint16_t FrameLengthLines;
    FrameLengthLines =
    pOV2778Ctx->frameLengthLines * pOV2778Ctx->maxFps / fps;
    result = OV2778_IsiWriteRegIss(handle, 0x30b2, (FrameLengthLines >> 8) & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x30b3, FrameLengthLines & 0xff);
    if (result != RET_SUCCESS) {
        TRACE(OV2778_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_FAILURE);
    }
    pOV2778Ctx->currFps = fps;
    pOV2778Ctx->curFrameLengthLines   = FrameLengthLines;
    pOV2778Ctx->maxIntegrationLine    = pOV2778Ctx->curFrameLengthLines - 3;
    pOV2778Ctx->aecMaxIntegrationTime = pOV2778Ctx->maxIntegrationLine * pOV2778Ctx->oneLineExpTime;

    TRACE(OV2778_INFO, "%s: set sensor fps = %d\n", __func__, pOV2778Ctx->currFps);

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2778_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL || pOV2778Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    if (pOV2778Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pIspStatus->useSensorAWB = true;
        pIspStatus->useSensorBLC = true;
    } else {
        pIspStatus->useSensorAWB = false;
        pIspStatus->useSensorBLC = false;
    }

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV2778_IsiSetBlcIss(IsiSensorHandle_t handle, const IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    uint32_t rOffset=0, grOffset=0, gbOffset=0, bOffset=0;
    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL || pOV2778Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)
        return RET_NULL_POINTER;

    float rGain=0, grGain=0, gbGain=0, bGain=0;
    rGain  = pOV2778Ctx->sensorWb.rGain;
    grGain = pOV2778Ctx->sensorWb.grGain;
    gbGain = pOV2778Ctx->sensorWb.gbGain;
    bGain  = pOV2778Ctx->sensorWb.bGain;

    if (rGain  < 1)  rGain  = 1;
    if (grGain < 1)  grGain = 1;
    if (gbGain < 1)  gbGain = 1;
    if (bGain  < 1)  bGain  = 1;

    rOffset  = (uint32_t)((rGain  - 1) * 0x100 * pBlc->red);
    grOffset = (uint32_t)((grGain - 1) * 0x100 * pBlc->gr);
    gbOffset = (uint32_t)((gbGain - 1) * 0x100 * pBlc->gb);
    bOffset  = (uint32_t)((bGain  - 1) * 0x100 * pBlc->blue);

    /* R,Gr,Gb,B HCG Offset */
    result |= OV2778_IsiWriteRegIss(handle, 0x3378, (rOffset >> 16) & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x3379, (rOffset >> 8)  & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x337a,  rOffset        & 0xff);

    result |= OV2778_IsiWriteRegIss(handle, 0x337b, (grOffset >> 16) & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x337c, (grOffset >> 8)  & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x337d,  grOffset        & 0xff);

    result |= OV2778_IsiWriteRegIss(handle, 0x337e, (gbOffset >> 16) & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x337f, (gbOffset >> 8)  & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x3380,  gbOffset        & 0xff);

    result |= OV2778_IsiWriteRegIss(handle, 0x3381, (bOffset >> 16) & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x3382, (bOffset >> 8)  & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x3383,  bOffset        & 0xff);

    /* R,Gr,Gb,B LCG Offset */
    result |= OV2778_IsiWriteRegIss(handle, 0x3384, (rOffset >> 16) & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x3385, (rOffset >> 8)  & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x3386,  rOffset        & 0xff);

    result |= OV2778_IsiWriteRegIss(handle, 0x3387, (grOffset >> 16) & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x3388, (grOffset >> 8)  & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x3389,  grOffset        & 0xff);

    result |= OV2778_IsiWriteRegIss(handle, 0x338a, (gbOffset >> 16) & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x338b, (gbOffset >> 8)  & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x338c,  gbOffset        & 0xff);

    result |= OV2778_IsiWriteRegIss(handle, 0x338d, (bOffset >> 16) & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x338e, (bOffset >> 8)  & 0xff);
    result |= OV2778_IsiWriteRegIss(handle, 0x338f,  bOffset        & 0xff);

    memcpy(&pOV2778Ctx->sensorBlc, pBlc, sizeof(IsiSensorBlc_t));

    if (result != RET_SUCCESS) {
        TRACE(OV2778_ERROR, "%s: set sensor blc error\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV2778_IsiGetBlcIss(IsiSensorHandle_t handle, IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    const OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)  return RET_NULL_POINTER;

    memcpy(pBlc, &pOV2778Ctx->sensorBlc, sizeof(IsiSensorBlc_t));

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV2778_IsiSetWBIss(IsiSensorHandle_t handle, const IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    bool update_flag = false;
    uint32_t rGain, grGain, gbGain, bGain;
    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL || pOV2778Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)
        return RET_NULL_POINTER;

    rGain  = (uint32_t)(pWb->rGain * 0x100);
    grGain = (uint32_t)(pWb->grGain * 0x100);
    gbGain = (uint32_t)(pWb->gbGain * 0x100);
    bGain  = (uint32_t)(pWb->bGain * 0x100);

    result |= OV2778_IsiWriteRegIss(handle, 0x3467, 0x00);
    result |= OV2778_IsiWriteRegIss(handle, 0x3464, 0x04);
    
    if (rGain != pOV2778Ctx->sensorWb.rGain) {
        result |= OV2778_IsiWriteRegIss(handle, 0x3360, (rGain >> 8) & 0xff);
        result |= OV2778_IsiWriteRegIss(handle, 0x3361,  rGain & 0xff);
        result |= OV2778_IsiWriteRegIss(handle, 0x3368, (rGain >> 8) & 0xff);
        result |= OV2778_IsiWriteRegIss(handle, 0x3369,  rGain & 0xff);
        update_flag = true;
        pOV2778Ctx->sensorWb.rGain = pWb->rGain;
    }
    if (grGain != pOV2778Ctx->sensorWb.grGain) {
        result |= OV2778_IsiWriteRegIss(handle, 0x3362, (grGain >> 8) & 0xff);
        result |= OV2778_IsiWriteRegIss(handle, 0x3363,  grGain & 0xff);
        result |= OV2778_IsiWriteRegIss(handle, 0x336a, (grGain >> 8) & 0xff);
        result |= OV2778_IsiWriteRegIss(handle, 0x336b,  grGain & 0xff);
        update_flag = true;
        pOV2778Ctx->sensorWb.grGain = pWb->grGain;
    }
    if (gbGain != pOV2778Ctx->sensorWb.gbGain) {
        result |= OV2778_IsiWriteRegIss(handle, 0x3364, (gbGain >> 8) & 0xff);
        result |= OV2778_IsiWriteRegIss(handle, 0x3365,  gbGain & 0xff);
        result |= OV2778_IsiWriteRegIss(handle, 0x336c, (gbGain >> 8) & 0xff);
        result |= OV2778_IsiWriteRegIss(handle, 0x336d,  gbGain & 0xff);
        update_flag = true;
        pOV2778Ctx->sensorWb.gbGain = pWb->gbGain;
    }
    if (bGain != pOV2778Ctx->sensorWb.bGain) {
        result |= OV2778_IsiWriteRegIss(handle, 0x3366, (bGain >> 8) & 0xff);
        result |= OV2778_IsiWriteRegIss(handle, 0x3367,  bGain & 0xff);
        result |= OV2778_IsiWriteRegIss(handle, 0x336e, (bGain >> 8) & 0xff);
        result |= OV2778_IsiWriteRegIss(handle, 0x336f,  bGain & 0xff);
        update_flag = true;
        pOV2778Ctx->sensorWb.bGain = pWb->bGain;
    }

    if (update_flag) {
        result = OV2778_IsiSetBlcIss(handle, &pOV2778Ctx->sensorBlc);
    }

    result |= OV2778_IsiWriteRegIss(handle, 0x3464, 0x14);
    result |= OV2778_IsiWriteRegIss(handle, 0x3467, 0x01);

    if (result != RET_SUCCESS) {
        TRACE(OV2778_ERROR, "%s: set sensor wb error\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV2778_IsiGetWBIss(IsiSensorHandle_t handle, IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    const OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL ) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)  return RET_NULL_POINTER;

    memcpy(pWb, &pOV2778Ctx->sensorWb, sizeof(IsiSensorWb_t));

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2778_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL || pOV2778Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV2778Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = OV2778_IsiWriteRegIss(handle, 0x3253, 0x00);
    } else {
        result = OV2778_IsiWriteRegIss(handle, 0x3253, 0x80);
    }

    pOV2778Ctx->testPattern = tpg.enable;

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2778_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL || pOV2778Ctx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV2778Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!OV2778_IsiReadRegIss(handle, 0x3253, &value)) {
        pTpg->enable = ((value & 0x80) != 0) ? 1 : 0;
        if(pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pOV2778Ctx->testPattern = pTpg->enable;
    }

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV2778_IsiGetExpandCurveIss(IsiSensorHandle_t handle, IsiSensorCompandCurve_t *pCurve)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV2778_INFO, "%s: (enter)\n", __func__);

    OV2778_Context_t *pOV2778Ctx = (OV2778_Context_t *) handle;
    if (pOV2778Ctx == NULL || pOV2778Ctx->isiCtx.halI2cHandle == NULL) {
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

    TRACE(OV2778_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV2778_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "OV2778";
    TRACE( OV2778_INFO, "%s (enter)\n", __func__);

    if ( pIsiSensor != NULL ) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = OV2778_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = OV2778_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = OV2778_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = OV2778_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = OV2778_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = OV2778_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = OV2778_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = OV2778_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = OV2778_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = OV2778_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = OV2778_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = OV2778_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = OV2778_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = OV2778_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = OV2778_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = OV2778_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = OV2778_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = OV2778_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = OV2778_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = OV2778_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = OV2778_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = OV2778_IsiGetIspStatusIss;
        pIsiSensor->pIsiSetBlcIss                       = OV2778_IsiSetBlcIss;
        pIsiSensor->pIsiGetBlcIss                       = OV2778_IsiGetBlcIss;
        pIsiSensor->pIsiSetWBIss                        = OV2778_IsiSetWBIss;
        pIsiSensor->pIsiGetWBIss                        = OV2778_IsiGetWBIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = OV2778_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = OV2778_IsiGetTpgIss;
        pIsiSensor->pIsiGetExpandCurveIss               = OV2778_IsiGetExpandCurveIss;

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

    TRACE( OV2778_INFO, "%s (exit)\n", __func__);
    return ( result );
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t OV2778_IsiCamDrvConfig = {
    .cameraDriverID      = 0x2770,
    .pIsiGetSensorIss    = OV2778_IsiGetSensorIss,
};
