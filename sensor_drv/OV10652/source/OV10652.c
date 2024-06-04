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
#include "OV10652_priv.h"

CREATE_TRACER( OV10652_INFO , "OV10652: ", INFO,    1 );
CREATE_TRACER( OV10652_WARN , "OV10652: ", WARNING, 1 );
CREATE_TRACER( OV10652_ERROR, "OV10652: ", ERROR,   1 );
CREATE_TRACER( OV10652_DEBUG,     "OV10652: ", INFO, 0 );
CREATE_TRACER( OV10652_REG_INFO , "OV10652: ", INFO, 1 );
CREATE_TRACER( OV10652_REG_DEBUG, "OV10652: ", INFO, 1 );


/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pov10652_mode_info[] = {
    {
        .index     = 0,
        .size      = {
            .boundsWidth  = 1824,
            .boundsHeight = 940,
            .top           = 0,
            .left          = 0,
            .width         = 1824,
            .height        = 940,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps       = 16 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 12,
        .bayerPattern = ISI_BPAT_RCCC,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 1,//native 3dol
        .size      = {
            .boundsWidth  = 1824,
            .boundsHeight = 940,
            .top           = 0,
            .left          = 0,
            .width         = 1824,
            .height        = 940,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps      = 16 * ISI_FPS_QUANTIZE,
        .hdrMode = ISI_SENSOR_MODE_HDR_NATIVE,
        .nativeMode = ISI_SENSOR_NATIVE_3DOL,
        .bitWidth = 12,
        .compress.enable = 1,
        .compress.xBit  = 20,
        .compress.yBit  = 12,
        .bayerPattern = ISI_BPAT_RCCC,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
};

static RESULT OV10652_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s (enter)\n", __func__);

    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL || pOV10652Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pOV10652Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(OV10652_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV10652_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV10652_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s (enter)\n", __func__);

    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL || pOV10652Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pOV10652Ctx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(OV10652_ERROR, "%s: hal write sensor register error!\n",__func__);
        return (RET_FAILURE);
    }

    TRACE(OV10652_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV10652_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(OV10652_INFO, "%s (enter)\n", __func__);
    const OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    if (pMode == NULL) {
        return (RET_WRONG_HANDLE);
    }

    memcpy(pMode, &(pOV10652Ctx->sensorMode), sizeof(pOV10652Ctx->sensorMode));

    TRACE(OV10652_INFO, "%s (exit)\n", __func__);
    return ( RET_SUCCESS );
}

static  RESULT OV10652_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    const OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    TRACE(OV10652_INFO, "%s (enter)\n", __func__);
    if (pOV10652Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pov10652_mode_info)/sizeof(pov10652_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pov10652_mode_info)/sizeof(pov10652_mode_info[0])); i++) {
        if (pov10652_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pov10652_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(OV10652_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT OV10652_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(OV10652_INFO, "%s (enter)\n", __func__);

    if (pOV10652Ctx == NULL) return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pOV10652Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pOV10652Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pOV10652Ctx->sensorMode.size.width;
    pCaps->resolution.height = pOV10652Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(OV10652_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV10652_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle) 
{
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s (enter)\n", __func__);

    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) osMalloc(sizeof(OV10652_Context_t));
    if (!pOV10652Ctx) {
        TRACE(OV10652_ERROR, "%s: Can't allocate ov10652 context\n", __func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pOV10652Ctx, 0, sizeof(OV10652_Context_t));

    pOV10652Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pOV10652Ctx->groupHold          = BOOL_FALSE;
    pOV10652Ctx->oldGain            = 0;
    pOV10652Ctx->oldIntegrationTime = 0;
    pOV10652Ctx->configured         = BOOL_FALSE;
    pOV10652Ctx->streaming          = BOOL_FALSE;
    pOV10652Ctx->testPattern        = BOOL_FALSE;
    pOV10652Ctx->isAfpsRun          = BOOL_FALSE;
    pOV10652Ctx->sensorMode.index   = 0;
    
    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x60;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pOV10652Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pOV10652Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(OV10652_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pOV10652Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        osFree(pOV10652Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pOV10652Ctx;

    TRACE(OV10652_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV10652_AecSetModeParameters(IsiSensorHandle_t handle, OV10652_Context_t *pOV10652Ctx)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s%s: (enter)\n", __func__, pOV10652Ctx->isAfpsRun ? "(AFPS)" : "");
    uint32_t expLine = 0, again = 0, dgain = 0;
    uint16_t value = 0;

    pOV10652Ctx->aecIntegrationTimeIncrement = pOV10652Ctx->oneLineExpTime;
    pOV10652Ctx->aecMinIntegrationTime       = pOV10652Ctx->oneLineExpTime * pOV10652Ctx->minIntegrationLine;
    //pOV10652Ctx->aecMaxIntegrationTime       = pOV10652Ctx->oneLineExpTime * pOV10652Ctx->maxIntegrationLine;
    pOV10652Ctx->aecMaxIntegrationTime       = 0.04f;
    pOV10652Ctx->aecMinVSIntegrationTime     = pOV10652Ctx->oneLineExpTime * pOV10652Ctx->minVSIntegrationLine;
    pOV10652Ctx->aecMaxVSIntegrationTime     = pOV10652Ctx->oneLineExpTime * pOV10652Ctx->maxVSIntegrationLine;

    TRACE(OV10652_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pOV10652Ctx->isAfpsRun ? "(AFPS)" : "",pOV10652Ctx->aecMaxIntegrationTime);

    pOV10652Ctx->aecGainIncrement = 1/256.0f;

    //reflects the state of the sensor registers, must equal default settings
    OV10652_IsiReadRegIss(handle, 0x3197, &value);
    value &= 0x03;
    if (value == 0) {
        again = 1;
    } else if (value == 1) {
        again = 2;
    } else if (value == 2) {
        again = 4;
    } else if (value == 3) {
        again = 8;
    }
    OV10652_IsiReadRegIss(handle, 0x3198, &value);
    dgain = (value & 0x3f) << 8;
    OV10652_IsiReadRegIss(handle, 0x3199, &value);
    dgain = dgain | (value & 0xff);
    pOV10652Ctx->aecCurGain            = again * (dgain/256.0);

    OV10652_IsiReadRegIss(handle, 0x3192, &value);
    expLine = value << 8;
    OV10652_IsiReadRegIss(handle, 0x3193, &value);
    expLine = expLine | (value & 0xff);
    pOV10652Ctx->aecCurIntegrationTime = expLine * pOV10652Ctx->oneLineExpTime;
    
    pOV10652Ctx->oldGain               = 0;
    pOV10652Ctx->oldIntegrationTime    = 0;

    TRACE(OV10652_INFO, "%s%s: (exit)\n", __func__,pOV10652Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT OV10652_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OV10652_INFO, "%s (enter)\n", __func__);

    if (!pOV10652Ctx) {
        TRACE(OV10652_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOV10652Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pOV10652Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pov10652_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (pov10652_mode_info[i].index == pOV10652Ctx->sensorMode.index) {
            SensorDefaultMode = &(pov10652_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(OV10652_mipi4lane_1824_940_init) / sizeof(OV10652_mipi4lane_1824_940_init[0]); i++) {
                    OV10652_IsiWriteRegIss(handle, OV10652_mipi4lane_1824_940_init[i][0], OV10652_mipi4lane_1824_940_init[i][1]);
                }
                break;
            case 1:
                for (int i = 0; i<sizeof(OV10652_mipi4lane_1824_940_native3dol_init) / sizeof(OV10652_mipi4lane_1824_940_native3dol_init[0]); i++) {
                    OV10652_IsiWriteRegIss(handle, OV10652_mipi4lane_1824_940_native3dol_init[i][0], OV10652_mipi4lane_1824_940_native3dol_init[i][1]);
                }
                break;
            default:
                TRACE(OV10652_INFO, "%s:not support sensor mode %d\n", __func__, pOV10652Ctx->sensorMode.index);
                osFree(pOV10652Ctx);
                return RET_NOTSUPP;
                break;
    }

        memcpy(&(pOV10652Ctx->sensorMode), SensorDefaultMode, sizeof(IsiSensorMode_t));
    } else {
        TRACE(OV10652_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pOV10652Ctx->sensorMode.index) {
        case 0:
            pOV10652Ctx->oneLineExpTime      = 0.0000645;
            pOV10652Ctx->frameLengthLines    = 0x3c9;
            pOV10652Ctx->curFrameLengthLines = pOV10652Ctx->frameLengthLines;
            pOV10652Ctx->maxIntegrationLine  = pOV10652Ctx->frameLengthLines -8 ;
            pOV10652Ctx->minIntegrationLine  = 1;
            pOV10652Ctx->aecMaxGain          = 230;
            pOV10652Ctx->aecMinGain          = 1.0;
            pOV10652Ctx->aGain.min           = 1.0;
            pOV10652Ctx->aGain.max           = 230;
            pOV10652Ctx->aGain.step          = (1.0f/256.0f);
            pOV10652Ctx->dGain.min           = 1.0;
            pOV10652Ctx->dGain.max           = 1.0;
            pOV10652Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 1:
            pOV10652Ctx->oneLineExpTime      = 0.0000645;
            pOV10652Ctx->frameLengthLines    = 0x3c9;
            pOV10652Ctx->curFrameLengthLines = pOV10652Ctx->frameLengthLines;
            pOV10652Ctx->maxIntegrationLine  = pOV10652Ctx->curFrameLengthLines - 8;
            pOV10652Ctx->minIntegrationLine  = 1;
            pOV10652Ctx->aecMaxGain          = 230;
            pOV10652Ctx->aecMinGain          = 1.0;
            pOV10652Ctx->aecMaxVSGain        = 128;
            pOV10652Ctx->aecMinVSGain        = 1.0;
            pOV10652Ctx->maxVSIntegrationLine= 5;
            pOV10652Ctx->minVSIntegrationLine= 1;
            pOV10652Ctx->aGain.min           = 1.0;
            pOV10652Ctx->aGain.max           = 230;
            pOV10652Ctx->aGain.step          = (1.0f/256.0f);
            pOV10652Ctx->dGain.min           = 1.0;
            pOV10652Ctx->dGain.max           = 1.0;
            pOV10652Ctx->dGain.step          = (1.0f/256.0f);
            pOV10652Ctx->aVSGain.min         = 1.0;
            pOV10652Ctx->aVSGain.max         = 128;
            pOV10652Ctx->aVSGain.step        = (1.0f/256.0f);
            pOV10652Ctx->dVSGain.min         = 1.0;
            pOV10652Ctx->dVSGain.max         = 1.0;
            pOV10652Ctx->dVSGain.step        = (1.0f/256.0f);
            break;
        default:
            TRACE(OV10652_INFO, "%s:not support sensor mode %d\n", __func__,pOV10652Ctx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    pOV10652Ctx->maxFps  = pOV10652Ctx->sensorMode.fps;
    pOV10652Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pOV10652Ctx->currFps = pOV10652Ctx->maxFps;

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(OV10652_DEBUG, "%s: OV10652 System-Reset executed\n", __func__);
    osSleep(100);

    result = OV10652_AecSetModeParameters(handle, pOV10652Ctx);
    if (result != RET_SUCCESS) {
        TRACE(OV10652_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pOV10652Ctx->configured = BOOL_TRUE;
    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT OV10652_IsiCloseIss(IsiSensorHandle_t handle)
{
    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s (enter)\n", __func__);

    if (pOV10652Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)OV10652_IsiSetStreamingIss(pOV10652Ctx, BOOL_FALSE);

    TRACE(OV10652_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV10652_IsiReleaseIss(IsiSensorHandle_t handle)
{
    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s (enter)\n", __func__);

    if (pOV10652Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pOV10652Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pOV10652Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pOV10652Ctx, 0, sizeof(OV10652_Context_t));
    osFree(pOV10652Ctx);
    TRACE(OV10652_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV10652_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;
    uint32_t sensorId = 0;
    uint32_t correctId = 0xA650;

    TRACE(OV10652_INFO, "%s (enter)\n", __func__);

    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL || pOV10652Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = OV10652_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(OV10652_ERROR, "%s: Read Sensor ID Error! \n",__func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(OV10652_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n", __func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(OV10652_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__, correctId, sensorId);
    TRACE(OV10652_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV10652_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(OV10652_INFO, "%s (enter)\n", __func__);

    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL || pOV10652Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (!pValue) return (RET_NULL_POINTER);

    regVal   = 0;
    result    = OV10652_IsiReadRegIss(handle, 0x300a, &regVal);
    sensorId = (regVal & 0xff) << 8;

    regVal   = 0;
    result    |= OV10652_IsiReadRegIss(handle, 0x300b, &regVal);
    sensorId |= (regVal & 0xff);

    *pValue = sensorId;
    TRACE(OV10652_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV10652_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s (enter)\n", __func__);

    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL || pOV10652Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV10652Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    result = OV10652_IsiWriteRegIss(handle, 0x3012, on);
    if (result != RET_SUCCESS) {
        TRACE(OV10652_ERROR, "%s: set sensor streaming error! \n",__func__);
        return (RET_FAILURE);
    }

    pOV10652Ctx->streaming = on;

    TRACE(OV10652_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV10652_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);

    if (pOV10652Ctx == NULL) {
        TRACE(OV10652_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(OV10652_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    //get time limit and total gain limit
    pAeBaseInfo->gain.min        = pOV10652Ctx->aecMinGain;
    pAeBaseInfo->gain.max        = pOV10652Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pOV10652Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pOV10652Ctx->aecMaxIntegrationTime;

    pAeBaseInfo->vsGain.min      = pOV10652Ctx->aecMinGain;
    pAeBaseInfo->vsGain.max      = pOV10652Ctx->aecMaxGain;
    pAeBaseInfo->vsIntTime.min   = pOV10652Ctx->aecMinVSIntegrationTime;
    pAeBaseInfo->vsIntTime.max   = pOV10652Ctx->aecMaxVSIntegrationTime;

    //get again/dgain info
    pAeBaseInfo->aGain           = pOV10652Ctx->aGain;
    pAeBaseInfo->dGain           = pOV10652Ctx->dGain;
    pAeBaseInfo->aVSGain         = pOV10652Ctx->aVSGain;
    pAeBaseInfo->dVSGain         = pOV10652Ctx->dVSGain;
    
    pAeBaseInfo->aecCurGain      = pOV10652Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime   = pOV10652Ctx->aecCurIntegrationTime;
    //pAeBaseInfo->aecCurVSGain    = (pOV10652Ctx->curVSAgain) * (pOV10652Ctx->curVSDgain);
    //pAeBaseInfo->aecCurVSIntTime = pOV10652Ctx->aecCurVSIntegrationTime;
    //pAeBaseInfo->aecCurLongGain  = 0;
    pAeBaseInfo->aecGainStep     = pOV10652Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep  = pOV10652Ctx->aecIntegrationTimeIncrement;
    pAeBaseInfo->stitchingMode   = pOV10652Ctx->sensorMode.stitchingMode;

    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT OV10652_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);

    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL || pOV10652Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    uint32_t again = 0, dgain = 0;
    float NewGain = 0;
    uint16_t data = 0;

    NewGain = pSensorAGain->gain[ISI_DUAL_EXP_L_PARAS];
    NewGain = MAX(pOV10652Ctx->aecMinGain, MIN(NewGain, pOV10652Ctx->aecMaxGain));

    if (pOV10652Ctx->sensorMode.hdrMode != ISI_SENSOR_MODE_LINEAR) {
        TRACE(OV10652_INFO, "%s: set native hdr mode L/S gain\n", __func__);
        uint32_t againSpd = 0, dgainLpd = 0, dgainSpd = 0;
        float Gain = 0, VSGain = 0;
        /* set native hdr mode L/S gain */
        //if(hdr_ratio[0] <= 1) hdr_ratio[0] =4;
        Gain = NewGain * 4;
        Gain = MAX(pOV10652Ctx->aecMinGain, MIN(Gain, pOV10652Ctx->aecMaxGain));

        if ( Gain < 1.0) Gain = 1.0;
        if ( Gain >= 1.0 && Gain < 2 ) {
            //againLpd = 1;
            dgainLpd = (uint32_t)((Gain/1.0)*256);
            result |= OV10652_IsiWriteRegIss(handle, 0x3197, 0x00);
        } else if ( Gain >= 2.0 && Gain < 4 ) {
            //againLpd = 2;
            dgainLpd = (uint32_t)((Gain/2.0)*256);
            result |= OV10652_IsiWriteRegIss(handle, 0x3197, 0x01);
        } else if ( Gain >= 4.0 && Gain < 8 ) {
            //againLpd = 4;
            dgainLpd = (uint32_t)((Gain/4.0)*256);
            result |= OV10652_IsiWriteRegIss(handle, 0x3197, 0x02);
        } else {
            //againLpd = 8;
            dgainLpd = (uint32_t)((Gain/8.0)*256);
            result |= OV10652_IsiWriteRegIss(handle, 0x3197, 0x03);
        }

        if ( NewGain < 1.0) NewGain = 1.0;
        if ( NewGain >= 1.0 && NewGain < 2 ) {
            againSpd = 1;
            dgainSpd = (uint32_t)((NewGain/1.0)*256);
            OV10652_IsiReadRegIss(handle, 0x3197, &data);
            result |= OV10652_IsiWriteRegIss(handle, 0x3197, data);
        } else if ( NewGain >= 2.0 && NewGain < 4 ) {
            againSpd = 2;
            dgainSpd = (uint32_t)((NewGain/2.0)*256);
            OV10652_IsiReadRegIss(handle, 0x3197, &data);
            result |= OV10652_IsiWriteRegIss(handle, 0x3197, data|0x04);
        } else if ( NewGain >= 4.0 && NewGain < 8 ) {
            againSpd = 4;
            dgainSpd = (uint32_t)((NewGain/4.0)*256);
            OV10652_IsiReadRegIss(handle, 0x3197, &data);
            result |= OV10652_IsiWriteRegIss(handle, 0x3197, data|0x08);
        } else {
            againSpd = 8;
            dgainSpd = (uint32_t)((NewGain/8.0)*256);
            OV10652_IsiReadRegIss(handle, 0x3197, &data);
            result |= OV10652_IsiWriteRegIss(handle, 0x3197, data|0x0c);
        }

        OV10652_IsiReadRegIss(handle, 0x3197, &data);
        //againLpd = againLpd; //fix build error in sw release mode
        TRACE(OV10652_DEBUG, "%s: againSpd=%d, 0x3197=0x%02x\n", againSpd, data);

        result |= OV10652_IsiWriteRegIss(handle,  0x3198, (dgainLpd >> 8) & 0x3f);
        result |= OV10652_IsiWriteRegIss(handle, 0x3199, (dgainLpd & 0xff));

        result |= OV10652_IsiWriteRegIss(handle,  0x319a, (dgainSpd >> 8) & 0x3f);
        result |= OV10652_IsiWriteRegIss(handle, 0x319b, (dgainSpd & 0xff));

        pOV10652Ctx->curAgain = againSpd;
        pOV10652Ctx->curDgain = dgainSpd/256.0f;
        pOV10652Ctx->aecCurGain = againSpd * (dgainSpd/256.0f);

        /* set native hdr mode VS gain */
        VSGain = pSensorAGain->gain[ISI_DUAL_EXP_S_PARAS];
        VSGain = MAX(1, MIN(VSGain, 128));

        if ( VSGain < 1.0) VSGain = 1.0;
        if ( VSGain >= 1.0 && VSGain < 2 ) {
            again = 1;
            dgain = (uint32_t)((VSGain/1)*256);
            result |= OV10652_IsiReadRegIss(handle, 0x3197, &data);
            result |= OV10652_IsiWriteRegIss(handle, 0x3197, data);
        } else if ( VSGain >= 2.0 && VSGain < 4 ) {
            again = 2;
            dgain = (uint32_t)((VSGain/2)*256);
            result |= OV10652_IsiReadRegIss(handle, 0x3197, &data);
            result |= OV10652_IsiWriteRegIss(handle, 0x3197, 0x10|data);
        } else if ( VSGain >= 4.0 && VSGain < 8 ) {
            again = 4;
            dgain = (uint32_t)((VSGain/4)*256);
            result |= OV10652_IsiReadRegIss(handle, 0x3197, &data);
            result |= OV10652_IsiWriteRegIss(handle, 0x3197, 0x20|data);
        } else {
            again = 8;
            dgain = (uint32_t)((VSGain/8)*256);
            result |= OV10652_IsiReadRegIss(handle, 0x3197, &data);
            result |= OV10652_IsiWriteRegIss(handle, 0x3197, 0x30|data);
        }
        result |= OV10652_IsiReadRegIss(handle, 0x3197, &data);
        TRACE(OV10652_DEBUG, "%s: VS_again=%d, 0x3197=0x%02x\n", __func__, again, data);

        result |= OV10652_IsiWriteRegIss(handle, 0x319c, (dgain >> 8) & 0x3f );
        result |= OV10652_IsiWriteRegIss(handle,0x319d,(dgain & 0xff));

    } else {
        TRACE(OV10652_INFO, "%s: set linear mode gain\n", __func__);

        if ( NewGain < 1.0) NewGain = 1.0;
        if ( NewGain >= 1.0 && NewGain < 2 ) {
            again = 1;
            dgain = (uint32_t)((NewGain/1)*256);
            TRACE(OV10652_INFO, "%s: again=1x\n", __func__);
            result = OV10652_IsiWriteRegIss(handle, 0x3197, 0x00);
        } else if ( NewGain >= 2.0 && NewGain < 4 ) {
            again = 2;
            dgain = (uint32_t)((NewGain/2)*256);
            TRACE(OV10652_INFO, "%s: again=2x\n", __func__);
            result = OV10652_IsiWriteRegIss(handle, 0x3197, 0x01);
        } else if ( NewGain >= 4.0 && NewGain < 8 ) {
            again = 4;
            dgain = (uint32_t)((NewGain/4)*256);
            TRACE(OV10652_INFO, "%s: again=4x\n", __func__);
            result = OV10652_IsiWriteRegIss(handle, 0x3197, 0x02);
        } else {
            again = 8;
            dgain = (uint32_t)((NewGain/8)*256);
            TRACE(OV10652_INFO, "%s: again=8x\n", __func__);
            result = OV10652_IsiWriteRegIss(handle, 0x3197, 0x03);
        }
        /* set linear mode(L channel) digital gain */
        result |= OV10652_IsiWriteRegIss(handle, 0x3198, (dgain >> 8) & 0x3f );
        result |= OV10652_IsiWriteRegIss(handle, 0x3199, (dgain & 0xff));
        pOV10652Ctx->curAgain = again;
        pOV10652Ctx->curDgain = dgain/256.0f;
        pOV10652Ctx->aecCurGain = again * (dgain/256.0f);
    }

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV10652_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV10652_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);

    if (pOV10652Ctx == NULL) {
        TRACE(OV10652_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    if(pOV10652Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        pSensorAGain->gain[ISI_LINEAR_PARAS]       = pOV10652Ctx->curAgain;

    } else if (pOV10652Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pSensorAGain->gain[ISI_DUAL_EXP_L_PARAS]   = pOV10652Ctx->curAgain;

    } else {
        TRACE(OV10652_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV10652_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);

    if (pOV10652Ctx == NULL) {
        TRACE(OV10652_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    if (pOV10652Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        pSensorDGain->gain[ISI_LINEAR_PARAS] = pOV10652Ctx->curDgain;

    } else if (pOV10652Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pSensorDGain->gain[ISI_DUAL_EXP_L_PARAS]  = pOV10652Ctx->curDgain;

    } else {
        TRACE(OV10652_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV10652_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);
    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;

    if (!pOV10652Ctx) {
        TRACE(OV10652_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOV10652Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        result = OV10652_SetIntTime(handle, pSensorIntTime->intTime[ISI_LINEAR_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(OV10652_INFO, "%s: set sensor IntTime[ISI_LINEAR_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

    } else if (pOV10652Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        result = OV10652_SetIntTime(handle, pSensorIntTime->intTime[ISI_DUAL_EXP_L_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(OV10652_INFO, "%s: set sensor IntTime[ISI_DUAL_EXP_L_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

        result = OV10652_SetVSIntTime(handle, pSensorIntTime->intTime[ISI_DUAL_EXP_S_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(OV10652_INFO, "%s: set sensor IntTime[ISI_DUAL_EXP_S_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

    }  else {
        TRACE(OV10652_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV10652_SetIntTime(IsiSensorHandle_t handle, float newIntegrationTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);
    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pOV10652Ctx) {
        TRACE(OV10652_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOV10652Ctx->sensorMode.hdrMode != ISI_SENSOR_MODE_LINEAR) {
        expLine = newIntegrationTime / pOV10652Ctx->oneLineExpTime;
        expLine = MIN(pOV10652Ctx->maxIntegrationLine, MAX(pOV10652Ctx->minIntegrationLine, expLine));
        TRACE(OV10652_DEBUG, "%s: set native hdr mode expLine=0x%04x\n", __func__, expLine);

        result =  OV10652_IsiWriteRegIss(handle, 0x3192,(expLine >> 8) & 0xff);//L channel
        result |= OV10652_IsiWriteRegIss(handle, 0x3193,(expLine & 0xff));

        result |=  OV10652_IsiWriteRegIss(handle, 0x3194,(expLine >> 8 & 0xff));//S channel
        result |= OV10652_IsiWriteRegIss(handle, 0x3195,(expLine & 0xff));

        pOV10652Ctx->aecCurIntegrationTime = expLine * pOV10652Ctx->oneLineExpTime;

    } else {
        expLine = newIntegrationTime / pOV10652Ctx->oneLineExpTime;
        expLine = MIN(pOV10652Ctx->maxIntegrationLine, MAX(pOV10652Ctx->minIntegrationLine, expLine));
        TRACE(OV10652_DEBUG, "%s: set linear mode expLine=0x%04x\n", __func__, expLine);

        result =  OV10652_IsiWriteRegIss(handle, 0x3192, (expLine >> 8) & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x3193, (expLine & 0xff));

        pOV10652Ctx->aecCurIntegrationTime = expLine * pOV10652Ctx->oneLineExpTime;

    TRACE(OV10652_DEBUG, "%s: set IntTime = %f\n", __func__, pOV10652Ctx->aecCurIntegrationTime);
    }

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV10652_SetVSIntTime(IsiSensorHandle_t handle, float newIntegrationTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);
    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pOV10652Ctx) {
        TRACE(OV10652_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    //set vs expLine, 0.5-3.9 row, Format 3.5,so *32.
    expLine = (newIntegrationTime/pOV10652Ctx->oneLineExpTime)*32;
    expLine = MIN(124, MAX(16, expLine));
    TRACE(OV10652_DEBUG, "%s: set VS expLine=0x%04x\n", __func__, expLine);

    result =  OV10652_IsiWriteRegIss(handle, 0x3196, expLine);
    pOV10652Ctx->aecCurVSIntegrationTime = (expLine * pOV10652Ctx->oneLineExpTime) / 32.0f;

    TRACE(OV10652_DEBUG, "%s: set VSIntTime = %f\n", __func__, pOV10652Ctx->aecCurVSIntegrationTime);
    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV10652_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);

    if (!pOV10652Ctx) {
        TRACE(OV10652_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);
    
    if (pOV10652Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        pSensorIntTime->intTime[ISI_LINEAR_PARAS] = pOV10652Ctx->aecCurIntegrationTime;

    } else if (pOV10652Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pSensorIntTime->intTime[ISI_DUAL_EXP_L_PARAS]   = pOV10652Ctx->aecCurIntegrationTime;
        pSensorIntTime->intTime[ISI_DUAL_EXP_S_PARAS]   = pOV10652Ctx->aecCurVSIntegrationTime;

    } else {
        TRACE(OV10652_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV10652_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t *pFps)
{
    const OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);

    if (pOV10652Ctx == NULL) {
        TRACE(OV10652_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pOV10652Ctx->currFps;

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV10652_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    RESULT result = RET_SUCCESS;
    int32_t newVts = 0;

    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);

    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL) {
        TRACE(OV10652_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (fps > pOV10652Ctx->maxFps) {
        TRACE(OV10652_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, fps, pOV10652Ctx->maxFps, pOV10652Ctx->minFps,pOV10652Ctx->maxFps);
        fps = pOV10652Ctx->maxFps;
    }
    if (fps < pOV10652Ctx->minFps) {
        TRACE(OV10652_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, fps, pOV10652Ctx->minFps, pOV10652Ctx->minFps,pOV10652Ctx->maxFps);
        fps = pOV10652Ctx->minFps;
    }

    newVts = pOV10652Ctx->frameLengthLines*pOV10652Ctx->sensorMode.fps / fps;
    result  =  OV10652_IsiWriteRegIss(handle, 0x30be, newVts >> 8);
    result |=  OV10652_IsiWriteRegIss(handle, 0x30bf, newVts & 0xff);
    pOV10652Ctx->currFps              = fps;
    pOV10652Ctx->curFrameLengthLines  = newVts;
    pOV10652Ctx->maxIntegrationLine   = pOV10652Ctx->curFrameLengthLines - 8;
    //pOV10652Ctx->aecMaxIntegrationTime = pOV10652Ctx->maxIntegrationLine * pOV10652Ctx->oneLineExpTime;

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV10652_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL || pOV10652Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);

    if (pOV10652Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pIspStatus->useSensorAWB = true;
        pIspStatus->useSensorBLC = true;
    } else {
        pIspStatus->useSensorAE  = false;
        pIspStatus->useSensorAWB = false;
        pIspStatus->useSensorBLC = false;
    }

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

static RESULT OV10652_IsiSetBlcIss(IsiSensorHandle_t handle, const IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    uint32_t rOffset=0, grOffset=0, gbOffset=0, bOffset=0;
    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL || pOV10652Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)
        return RET_NULL_POINTER;

    pOV10652Ctx->sensorBlc.red  = pBlc->red;
    pOV10652Ctx->sensorBlc.gr   = pBlc->gr;
    pOV10652Ctx->sensorBlc.gb   = pBlc->gb;
    pOV10652Ctx->sensorBlc.blue = pBlc->blue;

    if (pOV10652Ctx->sensorBlc.red  < 0x100)  pOV10652Ctx->sensorBlc.red  = 0x100;
    if (pOV10652Ctx->sensorBlc.gr   < 0x100)  pOV10652Ctx->sensorBlc.gr   = 0x100;
    if (pOV10652Ctx->sensorBlc.gb   < 0x100)  pOV10652Ctx->sensorBlc.gb   = 0x100;
    if (pOV10652Ctx->sensorBlc.blue < 0x100)  pOV10652Ctx->sensorBlc.blue = 0x100;

    rOffset  = (pOV10652Ctx->sensorBlc.red  - 0x100) * pBlc->red;
    grOffset = (pOV10652Ctx->sensorBlc.gr   - 0x100) * pBlc->gr;
    gbOffset = (pOV10652Ctx->sensorBlc.gb   - 0X100) * pBlc->gb;
    bOffset  = (pOV10652Ctx->sensorBlc.blue - 0X100) * pBlc->blue;
    /* R,Gr(C2),Gb(C1),B(C3) L Offset */
    result |= OV10652_IsiWriteRegIss(handle, 0x32e8, (rOffset >> 16) & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32e9, (rOffset >> 8)  & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32ea,  rOffset        & 0xff);

    result |= OV10652_IsiWriteRegIss(handle, 0x32eb, (grOffset >> 16) & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32ec, (grOffset >> 8)  & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32ed,  grOffset        & 0xff);

    result |= OV10652_IsiWriteRegIss(handle, 0x32ee, (gbOffset >> 16) & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32ef, (gbOffset >> 8)  & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32f0,  gbOffset        & 0xff);

    result |= OV10652_IsiWriteRegIss(handle, 0x32f1, (bOffset >> 16) & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32f2, (bOffset >> 8)  & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32f3,  bOffset        & 0xff);

    /* R,Gr(C2),Gb(C1),B(C3) S Offset */
    result |= OV10652_IsiWriteRegIss(handle, 0x32f4, (rOffset >> 16) & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32f5, (rOffset >> 8)  & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32f6,  rOffset        & 0xff);

    result |= OV10652_IsiWriteRegIss(handle, 0x32f7, (grOffset >> 16) & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32f8, (grOffset >> 8)  & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32f9,  grOffset        & 0xff);

    result |= OV10652_IsiWriteRegIss(handle, 0x32fa, (gbOffset >> 16) & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32fb, (gbOffset >> 8)  & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32fc,  gbOffset        & 0xff);

    result |= OV10652_IsiWriteRegIss(handle, 0x32fd, (bOffset >> 16) & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32fe, (bOffset >> 8)  & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x32ff,  bOffset        & 0xff);

    /* R,Gr(C2),Gb(C1),B(C3) VS Offset */
    result |= OV10652_IsiWriteRegIss(handle, 0x3300, (rOffset >> 16) & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x3301, (rOffset >> 8)  & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x3302,  rOffset        & 0xff);

    result |= OV10652_IsiWriteRegIss(handle, 0x3303, (grOffset >> 16) & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x3304, (grOffset >> 8)  & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x3305,  grOffset        & 0xff);

    result |= OV10652_IsiWriteRegIss(handle, 0x3306, (gbOffset >> 16) & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x3307, (gbOffset >> 8)  & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x3308,  gbOffset        & 0xff);

    result |= OV10652_IsiWriteRegIss(handle, 0x3309, (bOffset >> 16) & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x330a, (bOffset >> 8)  & 0xff);
    result |= OV10652_IsiWriteRegIss(handle, 0x330b,  bOffset        & 0xff);

    if (result != RET_SUCCESS) {
        TRACE(OV10652_ERROR, "%s: set sensor blc error\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

static RESULT OV10652_IsiGetBlcIss(IsiSensorHandle_t handle, IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);

    const OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)  return RET_NULL_POINTER;

    memcpy(pBlc, &pOV10652Ctx->sensorBlc, sizeof(IsiSensorBlc_t));

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV10652_IsiSetWBIss(IsiSensorHandle_t handle, const IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    bool update_flag = false;
    uint32_t rGain, grGain, gbGain, bGain;
    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL || pOV10652Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)
        return RET_NULL_POINTER;

    rGain  = pWb->rGain;
    grGain = pWb->grGain;
    gbGain= pWb->gbGain;
    bGain = pWb->bGain;
    
    if (rGain != pOV10652Ctx->sensorWb.rGain) {
        result |= OV10652_IsiWriteRegIss(handle, 0x32d0, (rGain >> 8) & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32d1,  rGain & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32d8, (rGain >> 8) & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32d9,  rGain & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32e0, (rGain >> 8) & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32e1,  rGain & 0xff);
        update_flag = true;
        pOV10652Ctx->sensorWb.rGain = rGain;
    }
    if (grGain != pOV10652Ctx->sensorWb.grGain) {
        result |= OV10652_IsiWriteRegIss(handle, 0x32d2, (grGain >> 8) & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32d3,  grGain & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32da, (grGain >> 8) & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32db,  grGain & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32e2, (grGain >> 8) & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32e3,  grGain & 0xff);
        update_flag = true;
        pOV10652Ctx->sensorWb.grGain = grGain;
    }
    if (gbGain!= pOV10652Ctx->sensorWb.gbGain) {
        result |= OV10652_IsiWriteRegIss(handle, 0x32d4, (gbGain>> 8) & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32d5,  gbGain& 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32dc, (gbGain>> 8) & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32dd,  gbGain& 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32e4, (gbGain>> 8) & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32e5,  gbGain& 0xff);
        update_flag = true;
        pOV10652Ctx->sensorWb.gbGain = gbGain;
    }
    if (bGain!= pOV10652Ctx->sensorWb.bGain) {
        result |= OV10652_IsiWriteRegIss(handle, 0x32d6, (bGain>> 8) & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32d7,  bGain& 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32de, (bGain>> 8) & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32df,  bGain& 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32e6, (bGain>> 8) & 0xff);
        result |= OV10652_IsiWriteRegIss(handle, 0x32e7,  bGain& 0xff);
        update_flag = true;
        pOV10652Ctx->sensorWb.bGain = bGain;
    }

    if (update_flag) {
        result = OV10652_IsiSetBlcIss(handle, &pOV10652Ctx->sensorBlc);
    }

    if (result != RET_SUCCESS) {
        TRACE(OV10652_ERROR, "%s: set sensor wb error\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

static RESULT OV10652_IsiGetWBIss(IsiSensorHandle_t handle, IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);

    const OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)  return RET_NULL_POINTER;

    memcpy(pWb, &pOV10652Ctx->sensorWb, sizeof(IsiSensorWb_t));

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV10652_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);

    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL || pOV10652Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV10652Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = OV10652_IsiWriteRegIss(handle, 0x3212, 0x00);
    } else {
        result = OV10652_IsiWriteRegIss(handle, 0x3212, 0x80);
    }

    pOV10652Ctx->testPattern = tpg.enable;

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV10652_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(OV10652_INFO, "%s: (enter)\n", __func__);

    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL || pOV10652Ctx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV10652Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!OV10652_IsiReadRegIss(handle, 0x3212,&value)) {
        pTpg->enable = ((value & 0x80) != 0) ? 1 : 0;
        if (pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pOV10652Ctx->testPattern = pTpg->enable;
    }

    TRACE(OV10652_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV10652_IsiGetExpandCurveIss(IsiSensorHandle_t handle, IsiSensorCompandCurve_t *pCurve)
{
    RESULT result = RET_SUCCESS;
    OV10652_Context_t *pOV10652Ctx = (OV10652_Context_t *) handle;
    if (pOV10652Ctx == NULL || pOV10652Ctx->isiCtx.halI2cHandle == NULL) {
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
            pCurve->compandYData[i] = pCurve->compandXData[i] << 2;//*4
        } else if (pCurve->compandXData[i] < 1408){
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 384) << 4;//*16
        } else if (pCurve->compandXData[i] < 2176){
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 1152) << 6;//*64
        } else {
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 2048) << 9;//*512
        }
    }

    return (result);

}

RESULT OV10652_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "OV10652";
    TRACE( OV10652_INFO, "%s (enter)\n", __func__);

    if ( pIsiSensor != NULL ) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = OV10652_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = OV10652_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = OV10652_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = OV10652_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = OV10652_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = OV10652_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = OV10652_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = OV10652_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = OV10652_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = OV10652_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = OV10652_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = OV10652_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = OV10652_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = OV10652_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = OV10652_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = OV10652_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = OV10652_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = OV10652_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = OV10652_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = OV10652_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = OV10652_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = OV10652_IsiGetIspStatusIss;
        pIsiSensor->pIsiSetBlcIss                       = OV10652_IsiSetBlcIss;
        pIsiSensor->pIsiGetBlcIss                       = OV10652_IsiGetBlcIss;
        pIsiSensor->pIsiSetWBIss                        = OV10652_IsiSetWBIss;
        pIsiSensor->pIsiGetWBIss                        = OV10652_IsiGetWBIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = OV10652_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = OV10652_IsiGetTpgIss;
        pIsiSensor->pIsiGetExpandCurveIss               = OV10652_IsiGetExpandCurveIss;

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

    TRACE( OV10652_INFO, "%s (exit)\n", __func__);
    return ( result );
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t OV10652_IsiCamDrvConfig = {
    .cameraDriverID      = 0xA650,
    .pIsiGetSensorIss    = OV10652_IsiGetSensorIss,
};
