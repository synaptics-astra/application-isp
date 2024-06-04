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
#include "OX08a4Y_priv.h"

CREATE_TRACER( OX08a4Y_INFO , "OX08a4Y: ", INFO,    1 );
CREATE_TRACER( OX08a4Y_WARN , "OX08a4Y: ", WARNING, 1 );
CREATE_TRACER( OX08a4Y_ERROR, "OX08a4Y: ", ERROR,   1 );
CREATE_TRACER( OX08a4Y_DEBUG,     "OX08a4Y: ", INFO, 0 );
CREATE_TRACER( OX08a4Y_REG_INFO , "OX08a4Y: ", INFO, 1 );
CREATE_TRACER( OX08a4Y_REG_DEBUG, "OX08a4Y: ", INFO, 1 );

#define OX08a4Y_MIN_GAIN_STEP    ( 1.0f/128.0f )  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain ) */

/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pox08a4y_mode_info[] = {
    {
        .index     = 0,
        .size      ={
            .boundsWidth  = 1920,
            .boundsHeight = 1080,
            .top           = 0,
            .left          = 0,
            .width         = 1920,
            .height        = 1080,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps       = 10 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_HDR_NATIVE,
        .nativeMode = ISI_SENSOR_NATIVE_DCG_SPD_VS,
        .bitWidth = 12,
        .compress.enable = 1,
        .compress.xBit  = 24,
        .compress.yBit  = 12,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
};

static RESULT OX08a4Y_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s (enter)\n", __func__);

    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL || pOX08a4YCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pOX08a4YCtx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(OX08a4Y_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OX08a4Y_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OX08a4Y_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s (enter)\n", __func__);

    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL || pOX08a4YCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pOX08a4YCtx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(OX08a4Y_ERROR, "%s: hal write sensor register error!\n",__func__);
        return (RET_FAILURE);
    }

    TRACE(OX08a4Y_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OX08a4Y_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(OX08a4Y_INFO, "%s (enter)\n", __func__);
    const OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    if (pMode == NULL) {
        return (RET_WRONG_HANDLE);
    }

    memcpy(pMode, &(pOX08a4YCtx->sensorMode), sizeof(pOX08a4YCtx->sensorMode));

    TRACE(OX08a4Y_INFO, "%s (exit)\n", __func__);
    return ( RET_SUCCESS );
}

static  RESULT OX08a4Y_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(OX08a4Y_INFO, "%s (enter)\n", __func__);
    const OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pox08a4y_mode_info)/sizeof(pox08a4y_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pox08a4y_mode_info)/sizeof(pox08a4y_mode_info[0])); i++) {
        if (pox08a4y_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pox08a4y_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(OX08a4Y_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT OX08a4Y_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(OX08a4Y_INFO, "%s (enter)\n", __func__);

    if (pOX08a4YCtx == NULL) return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pOX08a4YCtx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pOX08a4YCtx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pOX08a4YCtx->sensorMode.size.width;
    pCaps->resolution.height = pOX08a4YCtx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(OX08a4Y_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OX08a4Y_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle) 
{
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s (enter)\n", __func__);

    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) osMalloc(sizeof(OX08a4Y_Context_t));
    if (!pOX08a4YCtx) {
        TRACE(OX08a4Y_ERROR, "%s: Can't allocate ox08a4y context\n", __func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pOX08a4YCtx, 0, sizeof(OX08a4Y_Context_t));

    pOX08a4YCtx->isiCtx.pSensor     = pConfig->pSensor;
    pOX08a4YCtx->groupHold          = BOOL_FALSE;
    pOX08a4YCtx->oldGain            = 0;
    pOX08a4YCtx->oldIntegrationTime = 0;
    pOX08a4YCtx->configured         = BOOL_FALSE;
    pOX08a4YCtx->streaming          = BOOL_FALSE;
    pOX08a4YCtx->testPattern        = BOOL_FALSE;
    pOX08a4YCtx->isAfpsRun          = BOOL_FALSE;
    pOX08a4YCtx->sensorMode.index   = 0;
    
    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x6c;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pOX08a4YCtx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pOX08a4YCtx->isiCtx.halI2cHandle == NULL) {
        TRACE(OX08a4Y_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pOX08a4YCtx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        osFree(pOX08a4YCtx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pOX08a4YCtx;

    TRACE(OX08a4Y_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OX08a4Y_AecSetModeParameters(IsiSensorHandle_t handle, OX08a4Y_Context_t *pOX08a4YCtx)
{
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s%s: (enter)\n", __func__, pOX08a4YCtx->isAfpsRun ? "(AFPS)" : "");
    uint32_t expLine = 0, again = 0, dgain = 0;
    uint16_t value = 0;

    pOX08a4YCtx->aecIntegrationTimeIncrement = pOX08a4YCtx->oneLineExpTime;
    pOX08a4YCtx->aecMinIntegrationTime       = pOX08a4YCtx->oneLineExpTime * pOX08a4YCtx->minIntegrationLine;
    //pOX08a4YCtx->aecMaxIntegrationTime       = pOX08a4YCtx->oneLineExpTime * pOX08a4YCtx->maxIntegrationLine;
    pOX08a4YCtx->aecMaxIntegrationTime        = 0.04f;

    TRACE(OX08a4Y_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pOX08a4YCtx->isAfpsRun ? "(AFPS)" : "", pOX08a4YCtx->aecMaxIntegrationTime);

    pOX08a4YCtx->aecGainIncrement = OX08a4Y_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    //get again
    OX08a4Y_IsiReadRegIss(handle, 0x3588, &value);
    again = (value & 0x0f) << 8;
    OX08a4Y_IsiReadRegIss(handle, 0x3589, &value);
    again = again | (value & 0xf0);

    //get dgain
    OX08a4Y_IsiReadRegIss(handle, 0x358a, &value);
    dgain = (value & 0x0f) << 16;
    OX08a4Y_IsiReadRegIss(handle, 0x358b, &value);
    dgain = dgain | ((value & 0xff) << 8);
    OX08a4Y_IsiReadRegIss(handle, 0x358c, &value);
    dgain = dgain | (value & 0xc0);
    pOX08a4YCtx->aecCurGain.gain[0] = ((float)again/16.0) * ((float)dgain/1024.0);

    //get expLine
    OX08a4Y_IsiReadRegIss(handle, 0x3501, &value);
    expLine = value << 8;
    OX08a4Y_IsiReadRegIss(handle, 0x3502, &value);
    expLine = expLine | (value & 0xff);
    pOX08a4YCtx->aecCurIntTime.intTime[0] = expLine * pOX08a4YCtx->oneLineExpTime;
    
    pOX08a4YCtx->oldGain               = 0;
    pOX08a4YCtx->oldIntegrationTime    = 0;

    TRACE(OX08a4Y_INFO, "%s%s: (exit)\n", __func__,pOX08a4YCtx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT OX08a4Y_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OX08a4Y_INFO, "%s (enter)\n", __func__);

    if (!pOX08a4YCtx) {
        TRACE(OX08a4Y_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOX08a4YCtx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pOX08a4YCtx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pox08a4y_mode_info) / sizeof(IsiSensorMode_t); i++) {
        if (pox08a4y_mode_info[i].index == pOX08a4YCtx->sensorMode.index) {
            SensorDefaultMode = &(pox08a4y_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(OX08a4Y_mipi4lane_1080p_native4dol_init) / sizeof(OX08a4Y_mipi4lane_1080p_native4dol_init[0]); i++) {
                    OX08a4Y_IsiWriteRegIss(handle, OX08a4Y_mipi4lane_1080p_native4dol_init[i][0], OX08a4Y_mipi4lane_1080p_native4dol_init[i][1]);
                }
                break;
            default:
                TRACE(OX08a4Y_INFO, "%s:not support sensor mode %d\n", __func__, pOX08a4YCtx->sensorMode.index);
                osFree(pOX08a4YCtx);
                return RET_NOTSUPP;
                break;
    }

        memcpy(&(pOX08a4YCtx->sensorMode), SensorDefaultMode, sizeof(IsiSensorMode_t));
    } else {
        TRACE(OX08a4Y_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pOX08a4YCtx->sensorMode.index) {
        case 0:
            pOX08a4YCtx->oneLineDCGExpTime    = 0.0000393;
            pOX08a4YCtx->oneLineExpTime       = 0.0000249;
            pOX08a4YCtx->oneLineVSExpTime     = 0.0000247;
            pOX08a4YCtx->frameLengthLines     = 0x466;
            pOX08a4YCtx->curFrameLengthLines  = pOX08a4YCtx->frameLengthLines;
            pOX08a4YCtx->maxDCGIntegrationLine= 848;
            pOX08a4YCtx->minDCGIntegrationLine= 2;
            pOX08a4YCtx->maxIntegrationLine   = 212;
            pOX08a4YCtx->minIntegrationLine   = 2;
            pOX08a4YCtx->maxVSIntegrationLine = 53;
            pOX08a4YCtx->minVSIntegrationLine = 1;
            pOX08a4YCtx->aecMaxGain           = 240;
            pOX08a4YCtx->aecMinGain           = 1.0;
            pOX08a4YCtx->aGain.min            = 1.0;
            pOX08a4YCtx->aGain.max            = 15.5;
            pOX08a4YCtx->aGain.step           = (1.0f/16.0f);
            pOX08a4YCtx->dGain.min            = 1.0;
            pOX08a4YCtx->dGain.max            = 15.99;
            pOX08a4YCtx->dGain.step           = (1.0f/1024.0f);
            break;
        default:
            TRACE(OX08a4Y_INFO, "%s:not support sensor mode %d\n", __func__, pOX08a4YCtx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    pOX08a4YCtx->maxFps  = pOX08a4YCtx->sensorMode.fps;
    pOX08a4YCtx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pOX08a4YCtx->currFps = pOX08a4YCtx->maxFps;

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(OX08a4Y_DEBUG, "%s: OX08a4Y System-Reset executed\n", __func__);
    osSleep(100);

    result = OX08a4Y_AecSetModeParameters(handle, pOX08a4YCtx);
    if (result != RET_SUCCESS) {
        TRACE(OX08a4Y_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pOX08a4YCtx->configured = BOOL_TRUE;
    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT OX08a4Y_IsiCloseIss(IsiSensorHandle_t handle)
{
    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s (enter)\n", __func__);

    if (pOX08a4YCtx == NULL) return (RET_WRONG_HANDLE);

    (void)OX08a4Y_IsiSetStreamingIss(pOX08a4YCtx, BOOL_FALSE);

    TRACE(OX08a4Y_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OX08a4Y_IsiReleaseIss(IsiSensorHandle_t handle)
{
    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s (enter)\n", __func__);

    if (pOX08a4YCtx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pOX08a4YCtx->isiCtx.halI2cHandle);
    (void)HalDelRef(pOX08a4YCtx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pOX08a4YCtx, 0, sizeof(OX08a4Y_Context_t));
    osFree(pOX08a4YCtx);
    TRACE(OX08a4Y_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OX08a4Y_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;

    uint32_t sensorId = 0;
    uint32_t correctId = 0x0100;

    TRACE(OX08a4Y_INFO, "%s (enter)\n", __func__);

    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL || pOX08a4YCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = OX08a4Y_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(OX08a4Y_ERROR, "%s: Read Sensor ID Error! \n",__func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(OX08a4Y_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n", __func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(OX08a4Y_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__, correctId, sensorId);
    TRACE(OX08a4Y_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OX08a4Y_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(OX08a4Y_INFO, "%s (enter)\n", __func__);

    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL || pOX08a4YCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    regVal   = 0;
    result   = OX08a4Y_IsiReadRegIss(handle, 0x302a, &regVal);
    sensorId = (regVal & 0xff) << 8;

    regVal   = 0;
    result   |= OX08a4Y_IsiReadRegIss(handle, 0x302b, &regVal);
    sensorId |= (regVal & 0xff);

    *pValue = sensorId;
    TRACE(OX08a4Y_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OX08a4Y_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s (enter)\n", __func__);

    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL || pOX08a4YCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOX08a4YCtx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    result = OX08a4Y_IsiWriteRegIss(handle, 0x0100, on);
    if (result != RET_SUCCESS) {
        TRACE(OX08a4Y_ERROR, "%s: set sensor streaming error! \n",__func__);
        return (RET_FAILURE);
    }

    pOX08a4YCtx->streaming = on;

    TRACE(OX08a4Y_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OX08a4Y_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    if (pOX08a4YCtx == NULL) {
        TRACE(OX08a4Y_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(OX08a4Y_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    //get time limit and total gain limit
    pAeBaseInfo->gain.min        = pOX08a4YCtx->aecMinGain;
    pAeBaseInfo->gain.max        = pOX08a4YCtx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pOX08a4YCtx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pOX08a4YCtx->aecMaxIntegrationTime;

    //get again/dgain info
    pAeBaseInfo->aGain           = pOX08a4YCtx->aGain;
    pAeBaseInfo->dGain           = pOX08a4YCtx->dGain;
    
    pAeBaseInfo->aecCurGain     = pOX08a4YCtx->aecCurGain.gain[0];
    pAeBaseInfo->aecCurIntTime  = pOX08a4YCtx->aecCurIntTime.intTime[0];
    pAeBaseInfo->aecGainStep    = pOX08a4YCtx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep = pOX08a4YCtx->aecIntegrationTimeIncrement;

    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT OX08a4Y_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL || pOX08a4YCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOX08a4YCtx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        uint32_t again = 0;
        //set HCG analog gain, conversion gain = HCG/LCG = 3.67, base exp
        again = (uint32_t)((pSensorAGain->gain[ISI_LINEAR_PARAS]) * 16);
        result |= OX08a4Y_IsiWriteRegIss(handle, 0x3508, (again & 0x0ff0) >> 8);
        result |= OX08a4Y_IsiWriteRegIss(handle, 0x3509, (again & 0xf0));
        pOX08a4YCtx->curAgain.gain[0] = (float)again/16.0f;

        //set LCG analog gain
        again = (uint32_t)(pSensorAGain->gain[ISI_LINEAR_PARAS] * 16);
        result |= OX08a4Y_IsiWriteRegIss(handle, 0x3588, (again & 0x0ff0) >> 8);
        result |= OX08a4Y_IsiWriteRegIss(handle, 0x3589, (again & 0xf0));
        pOX08a4YCtx->curAgain.gain[1] = (float)again/16.0f;
        
        //set S analog gain
        again = (uint32_t)(pSensorAGain->gain[ISI_LINEAR_PARAS] * 16);
        result |= OX08a4Y_IsiWriteRegIss(handle, 0x3548, (again & 0x0ff0) >> 8);
        result |= OX08a4Y_IsiWriteRegIss(handle, 0x3549, (again & 0xf0));
        pOX08a4YCtx->curAgain.gain[2] = (float)again/16.0f;

        //set VS analog gain
        again = (uint32_t)(pSensorAGain->gain[ISI_LINEAR_PARAS] * 16);
        result |= OX08a4Y_IsiWriteRegIss(handle, 0x35c8, (again & 0x0ff0) >> 8);
        result |= OX08a4Y_IsiWriteRegIss(handle, 0x35c9, (again & 0xf0));
        pOX08a4YCtx->curAgain.gain[3] = (float)again/16.0f;

    } else {
        TRACE(OX08a4Y_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX08a4Y_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL || pOX08a4YCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOX08a4YCtx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        uint32_t dgain = 0;
        //set HCG digital gain, base exp
        dgain = (uint32_t)(pSensorDGain->gain[ISI_LINEAR_PARAS] * 1024);
        result = OX08a4Y_IsiWriteRegIss(handle, 0x350a, (dgain >> 16) & 0x0f);
        result |= OX08a4Y_IsiWriteRegIss(handle,0x350b, (dgain >> 8) & 0xff);
        result |= OX08a4Y_IsiWriteRegIss(handle,0x350c, (dgain & 0xc0));
        pOX08a4YCtx->curDgain.gain[0] = (float)dgain/1024.0f;
        pOX08a4YCtx->aecCurGain.gain[0] = pOX08a4YCtx->curAgain.gain[0] * pOX08a4YCtx->curDgain.gain[0];

        //set LCG digital gain
        dgain = (uint32_t)(pSensorDGain->gain[ISI_LINEAR_PARAS] * 1024);
        result |= OX08a4Y_IsiWriteRegIss(handle, 0x358a, (dgain >> 16) & 0x0f);
        result |= OX08a4Y_IsiWriteRegIss(handle,0x358b, (dgain >> 8) & 0xff);
        result |= OX08a4Y_IsiWriteRegIss(handle,0x358c, (dgain & 0xc0));
        pOX08a4YCtx->curDgain.gain[1] = (float)dgain/1024.0f;
        pOX08a4YCtx->aecCurGain.gain[1] = pOX08a4YCtx->curAgain.gain[1] * pOX08a4YCtx->curDgain.gain[1];

        //set S digital gain
        dgain = (uint32_t)(pSensorDGain->gain[ISI_LINEAR_PARAS] * 1024);
        result |= OX08a4Y_IsiWriteRegIss(handle, 0x354a, (dgain >> 16) & 0x0f);
        result |= OX08a4Y_IsiWriteRegIss(handle,0x354b, (dgain >> 8) & 0xff);
        result |= OX08a4Y_IsiWriteRegIss(handle,0x354c, (dgain & 0xc0));
        pOX08a4YCtx->curDgain.gain[2] = (float)dgain/1024.0f;
        pOX08a4YCtx->aecCurGain.gain[2] = pOX08a4YCtx->curAgain.gain[2] * pOX08a4YCtx->curDgain.gain[2];

        //set VS digital gain
        dgain = (uint32_t)(pSensorDGain->gain[ISI_LINEAR_PARAS] * 1024);
        result |= OX08a4Y_IsiWriteRegIss(handle, 0x35ca, (dgain >> 16) & 0x0f);
        result |= OX08a4Y_IsiWriteRegIss(handle,0x35cb, (dgain >> 8) & 0xff);
        result |= OX08a4Y_IsiWriteRegIss(handle,0x35cc, (dgain & 0xc0));
        pOX08a4YCtx->curDgain.gain[3] = (float)dgain/1024.0f;
        pOX08a4YCtx->aecCurGain.gain[3] = pOX08a4YCtx->curAgain.gain[3] * pOX08a4YCtx->curDgain.gain[3];

    } else {
        TRACE(OX08a4Y_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX08a4Y_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    if (pOX08a4YCtx == NULL) {
        TRACE(OX08a4Y_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    if(pOX08a4YCtx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        *pSensorAGain = pOX08a4YCtx->curAgain;
    } else {
        TRACE(OX08a4Y_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX08a4Y_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    if (pOX08a4YCtx == NULL) {
        TRACE(OX08a4Y_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    if (pOX08a4YCtx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        *pSensorDGain = pOX08a4YCtx->curDgain;

    } else {
        TRACE(OX08a4Y_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX08a4Y_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);
    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;

    if (!pOX08a4YCtx) {
        TRACE(OX08a4Y_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOX08a4YCtx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        result = OX08a4Y_SetIntTime(handle, pSensorIntTime->intTime[ISI_LINEAR_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(OX08a4Y_INFO, "%s: set sensor IntTime[ISI_LINEAR_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

    } else {
        TRACE(OX08a4Y_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OX08a4Y_SetIntTime(IsiSensorHandle_t handle, float newIntegrationTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);
    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    uint32_t expLine = 0;
    float dcgIntegrationTime = 0, sIntegrationTime = 0, vsIntegrationTime = 0;

    if (!pOX08a4YCtx) {
        TRACE(OX08a4Y_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    //set DCG(HCG & LCG) exp, base exp
    dcgIntegrationTime = newIntegrationTime;
    expLine = dcgIntegrationTime / pOX08a4YCtx->oneLineDCGExpTime;
    expLine = MIN(pOX08a4YCtx->maxDCGIntegrationLine, MAX(pOX08a4YCtx->minDCGIntegrationLine, expLine));
    TRACE(OX08a4Y_DEBUG, "%s: set DCG expLine = 0x%04x\n", __func__, expLine);

    result =  OX08a4Y_IsiWriteRegIss(handle, 0x3501,(expLine >> 8) & 0xff);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x3502,(expLine & 0xff));
    pOX08a4YCtx->aecCurIntTime.intTime[0] = expLine * pOX08a4YCtx->oneLineExpTime;
    pOX08a4YCtx->aecCurIntTime.intTime[1] = expLine * pOX08a4YCtx->oneLineExpTime;

    //set S exp
    sIntegrationTime = dcgIntegrationTime/4;
    expLine = sIntegrationTime / pOX08a4YCtx->oneLineExpTime;
    expLine = MIN(pOX08a4YCtx->maxIntegrationLine, MAX(pOX08a4YCtx->minIntegrationLine, expLine));
    TRACE(OX08a4Y_DEBUG, "%s: set S expLine = 0x%04x\n", __func__, expLine);

    result |= OX08a4Y_IsiWriteRegIss(handle, 0x3541,(expLine >> 8) & 0xff);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x3542,(expLine & 0xff));
    pOX08a4YCtx->aecCurIntTime.intTime[2] = expLine * pOX08a4YCtx->oneLineExpTime;

    //set VS exp
    vsIntegrationTime = sIntegrationTime/4;
    expLine = vsIntegrationTime / pOX08a4YCtx->oneLineVSExpTime;
    expLine = MIN(pOX08a4YCtx->maxVSIntegrationLine,MAX(pOX08a4YCtx->minVSIntegrationLine, expLine));
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x35c1,(expLine >> 8) & 0xff);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x35c2,(expLine & 0xff));
    pOX08a4YCtx->aecCurIntTime.intTime[3] = expLine * pOX08a4YCtx->oneLineExpTime;

    TRACE(OX08a4Y_DEBUG, "%s: set IntTime = %f\n", __func__, pOX08a4YCtx->aecCurIntTime.intTime[0]);
    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX08a4Y_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    if (!pOX08a4YCtx) {
        TRACE(OX08a4Y_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);


    if (pOX08a4YCtx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        *pSensorIntTime = pOX08a4YCtx->aecCurIntTime;

    } else {
        TRACE(OX08a4Y_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX08a4Y_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t *pFps)
{
    const OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    if (pOX08a4YCtx == NULL) {
        TRACE(OX08a4Y_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pOX08a4YCtx->currFps;

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX08a4Y_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    RESULT result = RET_SUCCESS;
    int32_t NewVts = 0;

    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL) {
        TRACE(OX08a4Y_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (fps > pOX08a4YCtx->maxFps) {
        TRACE(OX08a4Y_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, fps, pOX08a4YCtx->maxFps, pOX08a4YCtx->minFps,pOX08a4YCtx->maxFps);
        fps = pOX08a4YCtx->maxFps;
    }
    if (fps < pOX08a4YCtx->minFps) {
        TRACE(OX08a4Y_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, fps, pOX08a4YCtx->minFps, pOX08a4YCtx->minFps,pOX08a4YCtx->maxFps);
        fps = pOX08a4YCtx->minFps;
    }

    NewVts = pOX08a4YCtx->frameLengthLines*pOX08a4YCtx->sensorMode.fps / fps;
    result  =  OX08a4Y_IsiWriteRegIss(handle, 0x380e, NewVts >> 8);
    result |=  OX08a4Y_IsiWriteRegIss(handle, 0x380f, NewVts & 0xff);
    pOX08a4YCtx->currFps              = fps;
    pOX08a4YCtx->curFrameLengthLines  = NewVts;
    pOX08a4YCtx->maxIntegrationLine   = pOX08a4YCtx->curFrameLengthLines - 8;
    //pOX08a4YCtx->aecMaxIntegrationTime = pOX08a4YCtx->maxIntegrationLine * pOX08a4YCtx->oneLineExpTime;

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX08a4Y_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL || pOX08a4YCtx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = true;
    pIspStatus->useSensorAWB = true;

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT OX08a4Y_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL || pOX08a4YCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOX08a4YCtx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = OX08a4Y_IsiWriteRegIss(handle, 0x3212, 0x00);
    } else {
        result = OX08a4Y_IsiWriteRegIss(handle, 0x3212, 0x80);
    }

    pOX08a4YCtx->testPattern = tpg.enable;

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX08a4Y_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL || pOX08a4YCtx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOX08a4YCtx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!OX08a4Y_IsiReadRegIss(handle, 0x3212,&value)) {
        pTpg->enable = ((value & 0x80) != 0) ? 1 : 0;
        if (pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pOX08a4YCtx->testPattern = pTpg->enable;
    }

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OX08a4Y_IsiSetWBIss(IsiSensorHandle_t handle, const IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    uint32_t rGain, yrGain, ycGain, cyGain;
    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL || pOX08a4YCtx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)   return RET_NULL_POINTER;

    rGain  = (uint32_t)(pWb->rGain  * 1024);
    yrGain = (uint32_t)(pWb->grGain * 1024);
    ycGain = (uint32_t)(pWb->gbGain * 1024);
    cyGain = (uint32_t)(pWb->bGain  * 1024);

    //set HCG channel awb gain
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5280, (cyGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5281, cyGain & 0xff);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5282, (ycGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5283, ycGain & 0xff);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5284, (yrGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5285, yrGain & 0xff);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5286, (rGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5287, rGain & 0xff);

    //set LCG channel awb gain
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5480, (cyGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5481, cyGain & 0xff);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5482, (ycGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5483, ycGain & 0xff);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5484, (yrGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5485, yrGain & 0xff);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5486, (rGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5487, rGain & 0xff);

    //set S channel awb gain
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5680, (cyGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5681, cyGain & 0xff);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5682, (ycGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5683, ycGain & 0xff);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5684, (yrGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5685, yrGain & 0xff);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5686, (rGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5687, rGain & 0xff);

    //set VS channel awb gain
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5880, (cyGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5881, cyGain & 0xff);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5882, (ycGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5883, ycGain & 0xff);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5884, (yrGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5885, yrGain & 0xff);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5886, (rGain >> 8) & 0x7f);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x5887, rGain & 0xff);

    memcpy(&pOX08a4YCtx->sensorWb, pWb, sizeof(IsiSensorWb_t));

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return result;
}

static RESULT OX08a4Y_IsiGetWBIss(IsiSensorHandle_t handle, IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    const OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL ) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)  return RET_NULL_POINTER;

    memcpy(pWb, &pOX08a4YCtx->sensorWb, sizeof(IsiSensorWb_t));

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OX08a4Y_IsiSetBlcIss(IsiSensorHandle_t handle, const IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    uint16_t blcGain = 0;
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)  return RET_NULL_POINTER;

    blcGain = pBlc->red;
    //set HCG blc
    result = OX08a4Y_IsiWriteRegIss(handle, 0x4026, (blcGain >> 8) & 0x03);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x4027, blcGain & 0xff);
    //set LCG blc
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x4028, (blcGain >> 8) & 0x03);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x4029, blcGain & 0xff);
    //set S blc
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x402a, (blcGain >> 8) & 0x03);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x402b, blcGain & 0xff);
    //set VS blc
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x402c, (blcGain >> 8) & 0x03);
    result |= OX08a4Y_IsiWriteRegIss(handle, 0x402d, blcGain & 0xff);

    pOX08a4YCtx->sensorBlc = *pBlc;

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return result;
}

static RESULT OX08a4Y_IsiGetBlcIss(IsiSensorHandle_t handle, IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    const OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)  return RET_NULL_POINTER;

    *pBlc = pOX08a4YCtx->sensorBlc;

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return result;
}


static RESULT OX08a4Y_IsiGetExpandCurveIss(IsiSensorHandle_t handle, IsiSensorCompandCurve_t *pCurve)
{
    RESULT result = RET_SUCCESS;
    TRACE(OX08a4Y_INFO, "%s: (enter)\n", __func__);

    const OX08a4Y_Context_t *pOX08a4YCtx = (OX08a4Y_Context_t *) handle;
    if (pOX08a4YCtx == NULL) {
        return RET_NULL_POINTER;
    }

    //suppose isp pipeline is 24bit, expand_px left shift 12bit
    uint8_t expand_px[64] = {22, 20, 12, 20, 20, 20, 20, 19, 19, 19, 19, 19, 18, 18, 18, 18, 18,
                            18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 17, 17, 17, 17, 12};//dx_exp[i], index number, x[i]=x[i-1]+2^dx_exp[i]
    memcpy(pCurve->compandPx, expand_px, sizeof(expand_px));

    pCurve->compandXData[0] = 0;//x value in expand curve
    pCurve->compandYData[0] = 0;//y value in expand curve
    for(int i=1; i<65; i++){
        if (pCurve->compandXData[i-1] == 0 && pCurve->compandPx[i-1] > 0){
            pCurve->compandXData[i] = pCurve->compandXData[i-1] + ((1 << pCurve->compandPx[i-1]) - 1);
        } else if (pCurve->compandXData[i-1] > 0 && pCurve->compandPx[i-1] > 0){
            pCurve->compandXData[i] = pCurve->compandXData[i-1] + (1 << pCurve->compandPx[i-1]);
        } else if (pCurve->compandXData[i-1] > 0 && pCurve->compandPx[i-1] == 0){
            pCurve->compandXData[i] = pCurve->compandXData[i-1];
        } else {
            TRACE(OX08a4Y_INFO, "%s: invalid paramter\n", __func__);
            return RET_INVALID_PARM;
        }
    }

    const uint16_t expandXValue[34]={0, 1023, 1279, 1279, 1535, 1791, 2047, 2303, 2431, 2559, 2687, 2815, 2943, 3007, 3071,
                            3135, 3199, 3263, 3327, 3391, 3455, 3519, 3583, 3647, 3711, 3775, 3839, 3903, 3967, 3999,
                            4031, 4063, 4095, 4095};
    const uint32_t expandYValue[34]={0, 1023, 2047, 2047, 4095, 8191, 12287, 16383, 20479, 24575, 32767, 40959, 49151, 57343,
                            65535, 81919, 98303, 114687, 131071, 163839, 196607, 262143, 393215, 524287, 786431, 1048575,
                            1572863, 2097151, 3145727, 4194303, 8388607, 12582911, 16777215, 16777215};
    float slope[33] = {0};
    for(int i=0; i<33; i++){
        slope[i] = (float)(expandYValue[i+1] - expandYValue[i])/(float)(expandXValue[i+1] - expandXValue[i]);
    }

    for(int i=1; i<65; i++){
        for(int j=1; j<34; j++){
            if(pCurve->compandXData[i] >= expandXValue[j-1] && pCurve->compandXData[i] < expandXValue[j]){
                pCurve->compandYData[i] = expandYValue[j-1] + (pCurve->compandXData[i] - expandXValue[j-1]) * slope[j-1];
            }
        }
    }

    TRACE(OX08a4Y_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OX08a4Y_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "OX08a4Y";
    TRACE( OX08a4Y_INFO, "%s (enter)\n", __func__);

    if ( pIsiSensor != NULL ) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = OX08a4Y_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = OX08a4Y_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = OX08a4Y_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = OX08a4Y_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = OX08a4Y_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = OX08a4Y_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = OX08a4Y_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = OX08a4Y_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = OX08a4Y_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = OX08a4Y_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = OX08a4Y_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = OX08a4Y_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = OX08a4Y_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = OX08a4Y_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = OX08a4Y_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = OX08a4Y_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = OX08a4Y_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = OX08a4Y_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = OX08a4Y_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = OX08a4Y_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = OX08a4Y_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = OX08a4Y_IsiGetIspStatusIss;
        pIsiSensor->pIsiSetWBIss                        = OX08a4Y_IsiSetWBIss;
        pIsiSensor->pIsiGetWBIss                        = OX08a4Y_IsiGetWBIss;
        pIsiSensor->pIsiSetBlcIss                       = OX08a4Y_IsiSetBlcIss;
        pIsiSensor->pIsiGetBlcIss                       = OX08a4Y_IsiGetBlcIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = OX08a4Y_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = OX08a4Y_IsiGetTpgIss;
        pIsiSensor->pIsiGetExpandCurveIss               = OX08a4Y_IsiGetExpandCurveIss;

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

    TRACE( OX08a4Y_INFO, "%s (exit)\n", __func__);
    return ( result );
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t OX08a4Y_IsiCamDrvConfig = {
    .cameraDriverID      = 0x5308,
    .pIsiGetSensorIss    = OX08a4Y_IsiGetSensorIss,
};
