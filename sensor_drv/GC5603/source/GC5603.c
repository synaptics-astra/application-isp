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
#include "GC5603_priv.h"

CREATE_TRACER(GC5603_INFO , "GC5603: ", INFO,    1);
CREATE_TRACER(GC5603_WARN , "GC5603: ", WARNING, 1);
CREATE_TRACER(GC5603_ERROR, "GC5603: ", ERROR,   1);
CREATE_TRACER(GC5603_DEBUG,     "GC5603: ", INFO, 0);
CREATE_TRACER(GC5603_REG_INFO , "GC5603: ", INFO, 1);
CREATE_TRACER(GC5603_REG_DEBUG, "GC5603: ", INFO, 1);

#define GC5603_MIN_GAIN_STEP    (1.0f/64.0f)  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain) */

/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pgc5603_mode_info[] = {
    {
        .index     = 0,
        .size      = {
            .boundsWidth  = 2960,
            .boundsHeight = 1666,
            .top           = 0,
            .left          = 0,
            .width         = 2960,
            .height        = 1666,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps       = 5 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
};

static RESULT GC5603_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(GC5603_INFO, "%s (enter)\n", __func__);

    GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    if (pGC5603Ctx == NULL || pGC5603Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pGC5603Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(GC5603_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(GC5603_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT GC5603_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(GC5603_INFO, "%s (enter)\n", __func__);

    GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    if (pGC5603Ctx == NULL || pGC5603Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(GC5603_ERROR, "%s: NULL POINTER!\n", __func__);
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pGC5603Ctx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(GC5603_ERROR, "%s: hal write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(GC5603_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT GC5603_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(GC5603_INFO, "%s (enter)\n", __func__);
    const GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    if (pGC5603Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pGC5603Ctx->sensorMode), sizeof(pGC5603Ctx->sensorMode));

    TRACE(GC5603_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT GC5603_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(GC5603_INFO, "%s (enter)\n", __func__);
    const GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    if (pGC5603Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pgc5603_mode_info)/sizeof(pgc5603_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pgc5603_mode_info)/sizeof(pgc5603_mode_info[0])); i++) {
        if (pgc5603_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pgc5603_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(GC5603_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT GC5603_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(GC5603_INFO, "%s (enter)\n", __func__);

    if (pGC5603Ctx == NULL) return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pGC5603Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pGC5603Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pGC5603Ctx->sensorMode.size.width;
    pCaps->resolution.height = pGC5603Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(GC5603_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT GC5603_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle)
{
    RESULT result = RET_SUCCESS;
    TRACE(GC5603_INFO, "%s (enter)\n", __func__);

    GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) malloc(sizeof(GC5603_Context_t));
    if (!pGC5603Ctx) {
        TRACE(GC5603_ERROR, "%s: Can't allocate gc5603 context\n",__func__);
        return (RET_OUTOFMEM);
    }
    MEMSET(pGC5603Ctx, 0, sizeof(GC5603_Context_t));

    pGC5603Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pGC5603Ctx->groupHold          = BOOL_FALSE;
    pGC5603Ctx->oldGain            = 0;
    pGC5603Ctx->oldIntegrationTime = 0;
    pGC5603Ctx->configured         = BOOL_FALSE;
    pGC5603Ctx->streaming          = BOOL_FALSE;
    pGC5603Ctx->testPattern        = BOOL_FALSE;
    pGC5603Ctx->isAfpsRun          = BOOL_FALSE;
    pGC5603Ctx->sensorMode.index   = 0;

    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x62;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pGC5603Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pGC5603Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(GC5603_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pGC5603Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pGC5603Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pGC5603Ctx;

    TRACE(GC5603_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT GC5603_AecSetModeParameters(IsiSensorHandle_t handle, GC5603_Context_t * pGC5603Ctx)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0, regVal = 0;
    TRACE(GC5603_INFO, "%s%s: (enter)\n", __func__, pGC5603Ctx->isAfpsRun ? "(AFPS)" : "");

    pGC5603Ctx->aecIntegrationTimeIncrement = pGC5603Ctx->oneLineExpTime;
    pGC5603Ctx->aecMinIntegrationTime       = pGC5603Ctx->oneLineExpTime * pGC5603Ctx->minIntegrationLine;
    pGC5603Ctx->aecMaxIntegrationTime       = pGC5603Ctx->oneLineExpTime * pGC5603Ctx->maxIntegrationLine;

    TRACE(GC5603_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pGC5603Ctx->isAfpsRun ? "(AFPS)" : "",pGC5603Ctx->aecMaxIntegrationTime);

    pGC5603Ctx->aecGainIncrement = GC5603_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pGC5603Ctx->oldGain               = 0;
    pGC5603Ctx->oldIntegrationTime    = 0;

    GC5603_IsiReadRegIss(handle, 0x0202, &value);
    regVal = (value & 0x3f) << 8;
    GC5603_IsiReadRegIss(handle, 0x0203, &value);
    regVal = regVal | (value & 0xff);
    pGC5603Ctx->aecCurIntegrationTime = regVal * pGC5603Ctx->oneLineExpTime;
    pGC5603Ctx->aecCurGain = 1;

    TRACE(GC5603_INFO, "%s%s: (exit)\n", __func__,pGC5603Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT GC5603_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(GC5603_INFO, "%s (enter)\n", __func__);

    if (!pGC5603Ctx) {
        TRACE(GC5603_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pGC5603Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pGC5603Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pgc5603_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (pgc5603_mode_info[i].index == pGC5603Ctx->sensorMode.index) {
            SensorDefaultMode = &(pgc5603_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(GC5603_mipi4lane_2960_1666_init) / sizeof(GC5603_mipi4lane_2960_1666_init[0]); i++) {
                    GC5603_IsiWriteRegIss(handle, GC5603_mipi4lane_2960_1666_init[i][0], GC5603_mipi4lane_2960_1666_init[i][1]);
                }
                break;
            default:
                TRACE(GC5603_INFO, "%s:not support sensor mode %d\n", __func__,pGC5603Ctx->sensorMode.index);
                return RET_NOTSUPP;
                break;
    }
        memcpy(&(pGC5603Ctx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));

    } else {
        TRACE(GC5603_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pGC5603Ctx->sensorMode.index) {
        case 0:
            pGC5603Ctx->oneLineExpTime      = 0.00011429;
            pGC5603Ctx->frameLengthLines    = 0x6d6;
            pGC5603Ctx->curFrameLengthLines = pGC5603Ctx->frameLengthLines;
            pGC5603Ctx->maxIntegrationLine  = pGC5603Ctx->curFrameLengthLines - 8;
            pGC5603Ctx->minIntegrationLine  = 1;
            pGC5603Ctx->aecMaxGain          = 240;
            pGC5603Ctx->aecMinGain          = 1.0;
            pGC5603Ctx->aGain.min           = 1.0;
            pGC5603Ctx->aGain.max           = 240;
            pGC5603Ctx->aGain.step          = (1.0f/64.0f);
            pGC5603Ctx->dGain.min           = 1.0;
            pGC5603Ctx->dGain.max           = 1.0;
            pGC5603Ctx->dGain.step          = (1.0f/64.0f);
            break;
        default:
            TRACE(GC5603_INFO, "%s:not support sensor mode %d\n", __func__,pGC5603Ctx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    pGC5603Ctx->maxFps  = pGC5603Ctx->sensorMode.fps;
    pGC5603Ctx->minFps  = 1.2 * ISI_FPS_QUANTIZE;
    pGC5603Ctx->currFps = pGC5603Ctx->maxFps;

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(GC5603_DEBUG, "%s: GC5603 System-Reset executed\n", __func__);
    osSleep(100);

    result = GC5603_AecSetModeParameters(handle, pGC5603Ctx);
    if (result != RET_SUCCESS) {
        TRACE(GC5603_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pGC5603Ctx->configured = BOOL_TRUE;
    TRACE(GC5603_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT GC5603_IsiCloseIss(IsiSensorHandle_t handle)
{
    GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(GC5603_INFO, "%s (enter)\n", __func__);

    if (pGC5603Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)GC5603_IsiSetStreamingIss(pGC5603Ctx, BOOL_FALSE);

    TRACE(GC5603_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT GC5603_IsiReleaseIss(IsiSensorHandle_t handle)
{
    GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(GC5603_INFO, "%s (enter)\n", __func__);

    if (pGC5603Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pGC5603Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pGC5603Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pGC5603Ctx, 0, sizeof(GC5603_Context_t));
    free(pGC5603Ctx);
    TRACE(GC5603_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT GC5603_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;

    uint32_t sensorId = 0;
    uint32_t correctId = 0x5603;

    TRACE(GC5603_INFO, "%s (enter)\n", __func__);

    GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    if (pGC5603Ctx == NULL || pGC5603Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = GC5603_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(GC5603_ERROR, "%s: Read Sensor ID Error! \n",__func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(GC5603_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n", __func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(GC5603_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__, correctId, sensorId);
    TRACE(GC5603_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT GC5603_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(GC5603_INFO, "%s (enter)\n", __func__);

    GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    if (pGC5603Ctx == NULL || pGC5603Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (!pValue) return (RET_NULL_POINTER);

    regVal   = 0;
    result    = GC5603_IsiReadRegIss(handle, 0x03f0, &regVal);
    sensorId = (regVal & 0xff) << 8;

    regVal   = 0;
    result   |= GC5603_IsiReadRegIss(handle, 0x03f1, &regVal);
    sensorId |= (regVal & 0xff);

    *pValue = sensorId;
    TRACE(GC5603_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT GC5603_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;
    TRACE(GC5603_INFO, "%s (enter)\n", __func__);

    GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    if (pGC5603Ctx == NULL || pGC5603Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pGC5603Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    if(on){
        result = GC5603_IsiWriteRegIss(handle, 0x0100, 9);
        if (result != RET_SUCCESS) {
            TRACE(GC5603_ERROR, "%s: set sensor streaming on error! \n",__func__);
            return (RET_FAILURE);
        }
    }

    if(!on){
        result = GC5603_IsiWriteRegIss(handle, 0x0100, 0);
        if (result != RET_SUCCESS) {
            TRACE(GC5603_ERROR, "%s: set sensor streaming off error! \n",__func__);
            return (RET_FAILURE);
        }
    }

    pGC5603Ctx->streaming = on;

    TRACE(GC5603_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT GC5603_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    const GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(GC5603_INFO, "%s: (enter)\n", __func__);

    if (pGC5603Ctx == NULL) {
        TRACE(GC5603_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(GC5603_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    /* linear */
    pAeBaseInfo->gain.min        = pGC5603Ctx->aecMinGain;//total gain
    pAeBaseInfo->gain.max        = pGC5603Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pGC5603Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pGC5603Ctx->aecMaxIntegrationTime;

    pAeBaseInfo->aGain           = pGC5603Ctx->aGain;//min, max, step
    pAeBaseInfo->dGain           = pGC5603Ctx->dGain;

    pAeBaseInfo->aecCurGain      = pGC5603Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime   = pGC5603Ctx->aecCurIntegrationTime;

    pAeBaseInfo->aecGainStep     = pGC5603Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep  = pGC5603Ctx->aecIntegrationTimeIncrement;

    TRACE(GC5603_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT GC5603_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(GC5603_INFO, "%s: (enter)\n", __func__);
    uint32_t totalGain = 0, dgain_reg = 0;
    float aGain = 0, dGain = 0;

    GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    if (pGC5603Ctx == NULL || pGC5603Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    totalGain = (uint32_t)(pSensorAGain->gain[ISI_LINEAR_PARAS] * 64);
    for (int i=0; i < 26; i++){
        if(totalGain < GC5603_analog_gain[0][0]){
            TRACE(GC5603_ERROR,"%s: Invalid sensor gain\n", __func__);
            return (RET_INVALID_PARM);
        } else if (totalGain >= GC5603_analog_gain[25][0]){
            aGain = (float)(GC5603_analog_gain[25][0]/64.0f);
            dGain = (float)(totalGain/64.0f) / aGain;
            pGC5603Ctx->curAgain = aGain;
            pGC5603Ctx->curDgain = dGain;

            //set again registers
            result  = GC5603_IsiWriteRegIss(handle, 0x0614, GC5603_analog_gain[25][1]);
            result |= GC5603_IsiWriteRegIss(handle, 0x0615, GC5603_analog_gain[25][2]);
            result |= GC5603_IsiWriteRegIss(handle, 0x0225, GC5603_analog_gain[25][3]);
            result |= GC5603_IsiWriteRegIss(handle, 0x1467, GC5603_analog_gain[25][4]);
            result |= GC5603_IsiWriteRegIss(handle, 0x1468, GC5603_analog_gain[25][5]);
            result |= GC5603_IsiWriteRegIss(handle, 0x00b8, GC5603_analog_gain[25][6]);
            result |= GC5603_IsiWriteRegIss(handle, 0x00b9, GC5603_analog_gain[25][7]);

            //set dgain registers
            dgain_reg = (uint32_t)(dGain * 64);
            result |= GC5603_IsiWriteRegIss(handle, 0x0064, (dgain_reg >> 6) & 0x0f);
            result |= GC5603_IsiWriteRegIss(handle, 0x0065, (dgain_reg & 0x3f) << 2);
            break;
        } else if (totalGain >= GC5603_analog_gain[i][0] && totalGain < GC5603_analog_gain[i+1][0]){
            aGain = (float)(GC5603_analog_gain[i][0]/64.0f);
            dGain = (float)(totalGain/64.0f) / aGain;
            pGC5603Ctx->curAgain = aGain;
            pGC5603Ctx->curDgain = dGain;
            //set again registers
            result  = GC5603_IsiWriteRegIss(handle, 0x0614, GC5603_analog_gain[i][1]);
            result |= GC5603_IsiWriteRegIss(handle, 0x0615, GC5603_analog_gain[i][2]);
            result |= GC5603_IsiWriteRegIss(handle, 0x0225, GC5603_analog_gain[i][3]);
            result |= GC5603_IsiWriteRegIss(handle, 0x1467, GC5603_analog_gain[i][4]);
            result |= GC5603_IsiWriteRegIss(handle, 0x1468, GC5603_analog_gain[i][5]);
            result |= GC5603_IsiWriteRegIss(handle, 0x00b8, GC5603_analog_gain[i][6]);
            result |= GC5603_IsiWriteRegIss(handle, 0x00b9, GC5603_analog_gain[i][7]);

            //set dgain registers
            dgain_reg = (uint32_t)(dGain * 64);
            result |= GC5603_IsiWriteRegIss(handle, 0x0064, (dgain_reg >> 6) & 0x0f);
            result |= GC5603_IsiWriteRegIss(handle, 0x0065, (dgain_reg & 0x3f) << 2);
            break;
        }
    }

    pGC5603Ctx->aecCurGain = pSensorAGain->gain[ISI_LINEAR_PARAS];

    TRACE(GC5603_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC5603_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(GC5603_INFO, "%s: (enter)\n", __func__);

    TRACE(GC5603_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC5603_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    const GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(GC5603_INFO, "%s: (enter)\n", __func__);

    if (pGC5603Ctx == NULL) {
        TRACE(GC5603_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorAGain->gain[ISI_LINEAR_PARAS]       = pGC5603Ctx->curAgain;

    TRACE(GC5603_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC5603_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    const GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(GC5603_INFO, "%s: (enter)\n", __func__);

    if (pGC5603Ctx == NULL) {
        TRACE(GC5603_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorDGain->gain[ISI_LINEAR_PARAS] = pGC5603Ctx->curDgain;

    TRACE(GC5603_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC5603_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(GC5603_INFO, "%s: (enter)\n", __func__);
    GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pGC5603Ctx) {
        TRACE(GC5603_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = pSensorIntTime->intTime[ISI_LINEAR_PARAS] / pGC5603Ctx->oneLineExpTime;
    expLine = MIN(pGC5603Ctx->maxIntegrationLine, MAX(pGC5603Ctx->minIntegrationLine, expLine));
    TRACE(GC5603_DEBUG, "%s: set expLine = 0x%04x\n", __func__, expLine);

    result =  GC5603_IsiWriteRegIss(handle, 0x0202, (expLine >> 8) & 0x3f);
    result |= GC5603_IsiWriteRegIss(handle, 0x0203, (expLine & 0xff));

    pGC5603Ctx->aecCurIntegrationTime = expLine * pGC5603Ctx->oneLineExpTime;

    TRACE(GC5603_DEBUG, "%s: set IntTime = %f\n", __func__, pGC5603Ctx->aecCurIntegrationTime);

    TRACE(GC5603_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC5603_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    const GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(GC5603_INFO, "%s: (enter)\n", __func__);

    if (!pGC5603Ctx) {
        TRACE(GC5603_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    pSensorIntTime->intTime[ISI_LINEAR_PARAS] = pGC5603Ctx->aecCurIntegrationTime;

    TRACE(GC5603_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC5603_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t * pFps)
{
    const GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(GC5603_INFO, "%s: (enter)\n", __func__);

    if (pGC5603Ctx == NULL) {
        TRACE(GC5603_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pGC5603Ctx->currFps;

    TRACE(GC5603_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC5603_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    RESULT result = RET_SUCCESS;
    int32_t newVts = 0;

    TRACE(GC5603_INFO, "%s: (enter)\n", __func__);

    GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    if (pGC5603Ctx == NULL) {
        TRACE(GC5603_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

   if (fps > pGC5603Ctx->maxFps) {
        TRACE(GC5603_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, fps, pGC5603Ctx->maxFps, pGC5603Ctx->minFps,pGC5603Ctx->maxFps);
        fps = pGC5603Ctx->maxFps;
    }
    if (fps < pGC5603Ctx->minFps) {
        TRACE(GC5603_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, fps, pGC5603Ctx->minFps, pGC5603Ctx->minFps,pGC5603Ctx->maxFps);
        fps = pGC5603Ctx->minFps;
    }
     
    newVts = pGC5603Ctx->frameLengthLines*pGC5603Ctx->sensorMode.fps / fps;
    result =  GC5603_IsiWriteRegIss(handle, 0x0340, (newVts >> 8) & 0x3f);
    result |=  GC5603_IsiWriteRegIss(handle, 0x0341, newVts & 0xff);
    pGC5603Ctx->currFps              = fps;
    pGC5603Ctx->curFrameLengthLines  = newVts;
    pGC5603Ctx->maxIntegrationLine   = pGC5603Ctx->curFrameLengthLines - 8;
    pGC5603Ctx->aecMaxIntegrationTime = pGC5603Ctx->maxIntegrationLine * pGC5603Ctx->oneLineExpTime;

    TRACE(GC5603_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC5603_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    const GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    if (pGC5603Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(GC5603_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    pIspStatus->useSensorAWB = false;

    TRACE(GC5603_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT GC5603_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(GC5603_INFO, "%s: (enter)\n", __func__);

    GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    if (pGC5603Ctx == NULL || pGC5603Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pGC5603Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = GC5603_IsiWriteRegIss(handle, 0x008c, 0x10);
    } else {
        result = GC5603_IsiWriteRegIss(handle, 0x008c, 0x11);
    }

    pGC5603Ctx->testPattern = tpg.enable;

    TRACE(GC5603_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC5603_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(GC5603_INFO, "%s: (enter)\n", __func__);

    GC5603_Context_t *pGC5603Ctx = (GC5603_Context_t *) handle;
    if (pGC5603Ctx == NULL || pGC5603Ctx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pGC5603Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!GC5603_IsiReadRegIss(handle, 0x008c,&value)) {
        pTpg->enable = ((value & 0x01) != 0) ? 1 : 0;
        if(pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pGC5603Ctx->testPattern = pTpg->enable;
    }

    TRACE(GC5603_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC5603_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "GC5603";
    TRACE(GC5603_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = GC5603_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = GC5603_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = GC5603_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = GC5603_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = GC5603_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = GC5603_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = GC5603_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = GC5603_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = GC5603_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = GC5603_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = GC5603_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = GC5603_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = GC5603_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = GC5603_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = GC5603_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = GC5603_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = GC5603_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = GC5603_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = GC5603_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = GC5603_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = GC5603_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = GC5603_IsiGetIspStatusIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = GC5603_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = GC5603_IsiGetTpgIss;

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

    TRACE(GC5603_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t GC5603_IsiCamDrvConfig = {
    .cameraDriverID      = 0x5603,
    .pIsiGetSensorIss    = GC5603_IsiGetSensorIss,
};
