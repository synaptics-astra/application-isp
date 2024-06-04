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
#include "SC1330T_priv.h"

CREATE_TRACER(SC1330T_INFO , "SC1330T: ", INFO,    1);
CREATE_TRACER(SC1330T_WARN , "SC1330T: ", WARNING, 1);
CREATE_TRACER(SC1330T_ERROR, "SC1330T: ", ERROR,   1);
CREATE_TRACER(SC1330T_DEBUG,     "SC1330T: ", INFO, 0);
CREATE_TRACER(SC1330T_REG_INFO , "SC1330T: ", INFO, 1);
CREATE_TRACER(SC1330T_REG_DEBUG, "SC1330T: ", INFO, 1);

#define SC1330T_MIN_GAIN_STEP    (1.0f/128.0f)  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain) */

/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t psc1330t_mode_info[] = {
    {
        .index     = 0,
        .size      = {
            .boundsWidth  = 1280,
            .boundsHeight = 960,
            .top           = 0,
            .left          = 0,
            .width         = 1280,
            .height        = 960,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps       = 10 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 1,
        .size      = {
            .boundsWidth  = 1280,
            .boundsHeight = 960,
            .top           = 0,
            .left          = 0,
            .width         = 1280,
            .height        = 960,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps       = 15 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
};

static RESULT SC1330T_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(SC1330T_INFO, "%s (enter)\n", __func__);

    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    if (pSC1330TCtx == NULL || pSC1330TCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pSC1330TCtx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(SC1330T_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(SC1330T_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT SC1330T_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(SC1330T_INFO, "%s (enter)\n", __func__);

    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    if (pSC1330TCtx == NULL || pSC1330TCtx->isiCtx.halI2cHandle == NULL) {
        TRACE(SC1330T_ERROR, "%s: NULL POINTER!\n", __func__);
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pSC1330TCtx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(SC1330T_ERROR, "%s: hal write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(SC1330T_INFO, "%s addr 0x%04x, value 0x%02x (exit) result = %d\n", __func__, addr, value, result);
    return (result);
}

static RESULT SC1330T_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(SC1330T_INFO, "%s (enter)\n", __func__);
    const SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    if (pSC1330TCtx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pSC1330TCtx->sensorMode), sizeof(pSC1330TCtx->sensorMode));

    TRACE(SC1330T_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT SC1330T_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(SC1330T_INFO, "%s (enter)\n", __func__);
    const SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    if (pSC1330TCtx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(psc1330t_mode_info)/sizeof(psc1330t_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(psc1330t_mode_info)/sizeof(psc1330t_mode_info[0])); i++) {
        if (psc1330t_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &psc1330t_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(SC1330T_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT SC1330T_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(SC1330T_INFO, "%s (enter)\n", __func__);

    if (pSC1330TCtx == NULL) return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pSC1330TCtx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pSC1330TCtx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pSC1330TCtx->sensorMode.size.width;
    pCaps->resolution.height = pSC1330TCtx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_1LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(SC1330T_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT SC1330T_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle)
{
    RESULT result = RET_SUCCESS;
    TRACE(SC1330T_INFO, "%s (enter)\n", __func__);

    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) malloc(sizeof(SC1330T_Context_t));
    if (!pSC1330TCtx) {
        TRACE(SC1330T_ERROR, "%s: Can't allocate sc1330t context\n",__func__);
        return (RET_OUTOFMEM);
    }
    MEMSET(pSC1330TCtx, 0, sizeof(SC1330T_Context_t));

    pSC1330TCtx->isiCtx.pSensor     = pConfig->pSensor;
    pSC1330TCtx->groupHold          = BOOL_FALSE;
    pSC1330TCtx->oldGain            = 0;
    pSC1330TCtx->oldIntegrationTime = 0;
    pSC1330TCtx->configured         = BOOL_FALSE;
    pSC1330TCtx->streaming          = BOOL_FALSE;
    pSC1330TCtx->testPattern        = BOOL_FALSE;
    pSC1330TCtx->isAfpsRun          = BOOL_FALSE;
    pSC1330TCtx->sensorMode.index   = 0;

    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x60;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pSC1330TCtx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pSC1330TCtx->isiCtx.halI2cHandle == NULL) {
        TRACE(SC1330T_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pSC1330TCtx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pSC1330TCtx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pSC1330TCtx;

    TRACE(SC1330T_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT SC1330T_AecSetModeParameters(IsiSensorHandle_t handle, SC1330T_Context_t * pSC1330TCtx)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0, regVal = 0;
    float again = 0, dgain = 0;
    TRACE(SC1330T_INFO, "%s%s: (enter)\n", __func__, pSC1330TCtx->isAfpsRun ? "(AFPS)" : "");

    pSC1330TCtx->aecIntegrationTimeIncrement = pSC1330TCtx->oneLineExpTime;
    pSC1330TCtx->aecMinIntegrationTime       = pSC1330TCtx->oneLineExpTime * pSC1330TCtx->minIntegrationLine;
    pSC1330TCtx->aecMaxIntegrationTime       = pSC1330TCtx->oneLineExpTime * pSC1330TCtx->maxIntegrationLine;

    TRACE(SC1330T_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pSC1330TCtx->isAfpsRun ? "(AFPS)" : "",pSC1330TCtx->aecMaxIntegrationTime);

    pSC1330TCtx->aecGainIncrement = SC1330T_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pSC1330TCtx->oldGain               = 0;
    pSC1330TCtx->oldIntegrationTime    = 0;

    SC1330T_IsiReadRegIss(handle, 0x3e00, &value);
    regVal = (value & 0x0f) << 12;
    SC1330T_IsiReadRegIss(handle, 0x3e01, &value);
    regVal = regVal | ((value & 0xFF) << 4);
    SC1330T_IsiReadRegIss(handle, 0x3e02, &value);
    regVal = regVal | ((value & 0xF0) >> 4);
    pSC1330TCtx->aecCurIntegrationTime = regVal * pSC1330TCtx->oneLineExpTime;

    //read again/dgain value in intial setting
    result = SC1330T_IsiReadRegIss(handle, 0x3e08, &value);
    TRACE(SC1330T_INFO, "%s 0x3e08 = 0x%02x********\n", __func__, value);
    result |= SC1330T_IsiReadRegIss(handle, 0x3e09, &value);
    TRACE(SC1330T_INFO, "%s 0x3e09 = 0x%02x********\n", __func__, value);
    result |= SC1330T_IsiReadRegIss(handle, 0x3e06, &value);
    TRACE(SC1330T_INFO, "%s 0x3e06 = 0x%02x********\n", __func__, value);
    result |= SC1330T_IsiReadRegIss(handle, 0x3e07, &value);
    TRACE(SC1330T_INFO, "%s 0x3e07 = 0x%02x********\n", __func__, value);
    again = 1.0f; /*0x3e08, 3e09 = 0x0340*/
    dgain = 1.0f; /*0x3e06, 3e07 = 0x0080*/
    pSC1330TCtx->aecCurGain = again * dgain;

    TRACE(SC1330T_INFO, "%s%s: (exit)\n", __func__,pSC1330TCtx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT SC1330T_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(SC1330T_INFO, "%s (enter)\n", __func__);

    if (!pSC1330TCtx) {
        TRACE(SC1330T_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSC1330TCtx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pSC1330TCtx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(psc1330t_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (psc1330t_mode_info[i].index == pSC1330TCtx->sensorMode.index) {
            SensorDefaultMode = &(psc1330t_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(SC1330T_mipi1lane_1280_960_10fps_init) / sizeof(SC1330T_mipi1lane_1280_960_10fps_init[0]); i++) {
                    SC1330T_IsiWriteRegIss(handle, SC1330T_mipi1lane_1280_960_10fps_init[i][0], SC1330T_mipi1lane_1280_960_10fps_init[i][1]);
                }
                break;
            case 1:
                for (int i = 0; i<sizeof(SC1330T_mipi1lane_1280_960_15fps_init) / sizeof(SC1330T_mipi1lane_1280_960_15fps_init[0]); i++) {
                    SC1330T_IsiWriteRegIss(handle, SC1330T_mipi1lane_1280_960_15fps_init[i][0], SC1330T_mipi1lane_1280_960_15fps_init[i][1]);
                }
                break;
            default:
                TRACE(SC1330T_INFO, "%s:not support sensor mode %d\n", __func__,pSC1330TCtx->sensorMode.index);
                return RET_NOTSUPP;
                break;
    }

        memcpy(&(pSC1330TCtx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));

    } else {
        TRACE(SC1330T_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pSC1330TCtx->sensorMode.index) {
        case 0:
            pSC1330TCtx->oneLineExpTime      = 0.00005;
            pSC1330TCtx->frameLengthLines    = 0x7d0;
            pSC1330TCtx->curFrameLengthLines = pSC1330TCtx->frameLengthLines;
            pSC1330TCtx->maxIntegrationLine  = pSC1330TCtx->curFrameLengthLines -4 ;
            pSC1330TCtx->minIntegrationLine  = 1;
            pSC1330TCtx->aecMaxGain          = 2740;
            pSC1330TCtx->aecMinGain          = 1.0;
            pSC1330TCtx->aGain.min           = 1.0;
            pSC1330TCtx->aGain.max           = 86.328;
            pSC1330TCtx->aGain.step          = (1.0f/64.0f);
            pSC1330TCtx->dGain.min           = 1.0;
            pSC1330TCtx->dGain.max           = 31.75;
            pSC1330TCtx->dGain.step          = (1.0f/128.0f);
            break;
        case 1:
            pSC1330TCtx->oneLineExpTime      = 0.000052;
            pSC1330TCtx->frameLengthLines    = 0x500;
            pSC1330TCtx->curFrameLengthLines = pSC1330TCtx->frameLengthLines;
            pSC1330TCtx->maxIntegrationLine  = pSC1330TCtx->curFrameLengthLines -4 ;
            pSC1330TCtx->minIntegrationLine  = 1;
            pSC1330TCtx->aecMaxGain          = 2740;
            pSC1330TCtx->aecMinGain          = 1.0;
            pSC1330TCtx->aGain.min           = 1.0;
            pSC1330TCtx->aGain.max           = 86.328;
            pSC1330TCtx->aGain.step          = (1.0f/64.0f);
            pSC1330TCtx->dGain.min           = 1.0;
            pSC1330TCtx->dGain.max           = 31.75;
            pSC1330TCtx->dGain.step          = (1.0f/128.0f);
            break;
        default:
            TRACE(SC1330T_INFO, "%s:not support sensor mode %d\n", __func__,pSC1330TCtx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    pSC1330TCtx->maxFps  = pSC1330TCtx->sensorMode.fps;
    pSC1330TCtx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pSC1330TCtx->currFps = pSC1330TCtx->maxFps;

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(SC1330T_DEBUG, "%s: SC1330T System-Reset executed\n", __func__);
    osSleep(100);

    result = SC1330T_AecSetModeParameters(handle, pSC1330TCtx);
    if (result != RET_SUCCESS) {
        TRACE(SC1330T_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pSC1330TCtx->configured = BOOL_TRUE;
    TRACE(SC1330T_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT SC1330T_IsiCloseIss(IsiSensorHandle_t handle)
{
    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(SC1330T_INFO, "%s (enter)\n", __func__);

    if (pSC1330TCtx == NULL) return (RET_WRONG_HANDLE);

    (void)SC1330T_IsiSetStreamingIss(pSC1330TCtx, BOOL_FALSE);

    TRACE(SC1330T_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT SC1330T_IsiReleaseIss(IsiSensorHandle_t handle)
{
    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(SC1330T_INFO, "%s (enter)\n", __func__);

    if (pSC1330TCtx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pSC1330TCtx->isiCtx.halI2cHandle);
    (void)HalDelRef(pSC1330TCtx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pSC1330TCtx, 0, sizeof(SC1330T_Context_t));
    free(pSC1330TCtx);
    TRACE(SC1330T_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT SC1330T_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;
    uint32_t sensorId = 0;
    uint32_t correctId = 0xca18;

    TRACE(SC1330T_INFO, "%s (enter)\n", __func__);

    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    if (pSC1330TCtx == NULL || pSC1330TCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = SC1330T_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(SC1330T_ERROR, "%s: Read Sensor ID Error! \n",__func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(SC1330T_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n", __func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(SC1330T_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__, correctId, sensorId);
    TRACE(SC1330T_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT SC1330T_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(SC1330T_INFO, "%s (enter)\n", __func__);

    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    if (pSC1330TCtx == NULL || pSC1330TCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (!pValue) return (RET_NULL_POINTER);

    regVal   = 0;
    result   = SC1330T_IsiReadRegIss(handle, 0x3107, &regVal);
    sensorId = (regVal & 0xff) << 8;

    regVal   = 0;
    result   |= SC1330T_IsiReadRegIss(handle, 0x3108, &regVal);
    sensorId |= (regVal & 0xff);

    *pValue = sensorId;
    TRACE(SC1330T_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT SC1330T_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;
    TRACE(SC1330T_INFO, "%s (enter)\n", __func__);

    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    if (pSC1330TCtx == NULL || pSC1330TCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pSC1330TCtx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    if(on){
        result = SC1330T_IsiWriteRegIss(handle, 0x0100, 1);
        if (result != RET_SUCCESS) {
            TRACE(SC1330T_ERROR, "%s: set sensor streaming on error! \n",__func__);
            return (RET_FAILURE);
        }
    }

    if(!on){
        result = SC1330T_IsiWriteRegIss(handle, 0x0100, 0);
        if (result != RET_SUCCESS) {
            TRACE(SC1330T_ERROR, "%s: set sensor streaming off error! \n",__func__);
            return (RET_FAILURE);
        }
    }

    pSC1330TCtx->streaming = on;

    TRACE(SC1330T_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT SC1330T_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    const SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(SC1330T_INFO, "%s: (enter)\n", __func__);

    if (pSC1330TCtx == NULL) {
        TRACE(SC1330T_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(SC1330T_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    /* linear */
    pAeBaseInfo->gain.min        = pSC1330TCtx->aecMinGain;//total gain
    pAeBaseInfo->gain.max        = pSC1330TCtx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pSC1330TCtx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pSC1330TCtx->aecMaxIntegrationTime;

    pAeBaseInfo->aGain           = pSC1330TCtx->aGain;//min, max, step
    pAeBaseInfo->dGain           = pSC1330TCtx->dGain;

    pAeBaseInfo->aecCurGain      = pSC1330TCtx->aecCurGain;
    pAeBaseInfo->aecCurIntTime   = pSC1330TCtx->aecCurIntegrationTime;

    pAeBaseInfo->aecGainStep     = pSC1330TCtx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep  = pSC1330TCtx->aecIntegrationTimeIncrement;

    TRACE(SC1330T_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT SC1330T_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(SC1330T_INFO, "%s: (enter)\n", __func__);
    float again = 0, anaGain = 0;
    uint32_t value = 0, coarseAgain = 0, fineAgain = 0;
    float dcgGain = 5.438;

    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    if (pSC1330TCtx == NULL || pSC1330TCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    again = pSensorAGain->gain[ISI_LINEAR_PARAS];
    if (again > dcgGain) {
        anaGain = again / dcgGain;
    } else {
        anaGain = again;
    }

    if (anaGain >= 1 && anaGain < 2) {
        value = 3;
    } else if (anaGain >= 2 && anaGain < 4) {
        value = 7;
    } else if (anaGain >= 4 && anaGain < 8) {
        value = 15;
    } else if (anaGain >= 8 && anaGain < 16) {
        value = 31;
    } else if (anaGain >= 16 && anaGain < 32) {
        value = 63;
    }

    if (again > dcgGain) {
        coarseAgain = value + 32;
        result = SC1330T_IsiWriteRegIss(handle, 0x3e08, coarseAgain);
    } else {
        coarseAgain = value;
        result = SC1330T_IsiWriteRegIss(handle, 0x3e08, coarseAgain);
    }

    if (anaGain >= 1 && anaGain < 2) {
        fineAgain = (uint32_t)(anaGain * 64);
        result = SC1330T_IsiWriteRegIss(handle, 0x3e09, fineAgain);
    } else if (anaGain >= 2 && anaGain < 4) {
        fineAgain = (uint32_t)(anaGain * 32);
        result = SC1330T_IsiWriteRegIss(handle, 0x3e09, fineAgain);
    } else if (anaGain >= 4 && anaGain < 8) {
        fineAgain = (uint32_t)(anaGain * 16);
        result = SC1330T_IsiWriteRegIss(handle, 0x3e09, fineAgain);
    } else if (anaGain >= 8 && anaGain < 16) {
        fineAgain = (uint32_t)(anaGain * 8);
        result = SC1330T_IsiWriteRegIss(handle, 0x3e09, fineAgain);
    } else if (anaGain >= 16 && anaGain < 32) {
        fineAgain = (uint32_t)(anaGain * 4);
        result = SC1330T_IsiWriteRegIss(handle, 0x3e09, fineAgain);
    } else {
        TRACE(SC1330T_ERROR,"%s: Invalid sensor analog gain value.\n", __func__);
        return (RET_INVALID_PARM);
    }

    pSC1330TCtx->curAgain = again;

    TRACE(SC1330T_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT SC1330T_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(SC1330T_INFO, "%s: (enter)\n", __func__);
    float dgain = 0;
    uint32_t coarseDgain = 0, fineDgain = 0;

    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    if (pSC1330TCtx == NULL || pSC1330TCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    dgain = pSensorDGain->gain[ISI_LINEAR_PARAS];
    if (dgain >= 1 && dgain < 2) {
        coarseDgain = 0;
        fineDgain = (uint32_t)(dgain * 128);
        result = SC1330T_IsiWriteRegIss(handle, 0x3e06, coarseDgain);
        result |= SC1330T_IsiWriteRegIss(handle, 0x3e07, fineDgain);
    } else if (dgain >= 2 && dgain < 4) {
        coarseDgain = 1;
        fineDgain = (uint32_t)(dgain * 64);
        result = SC1330T_IsiWriteRegIss(handle, 0x3e06, coarseDgain);
        result |= SC1330T_IsiWriteRegIss(handle, 0x3e07, fineDgain);
    } else if (dgain >= 4 && dgain < 8) {
        coarseDgain = 3;
        fineDgain = (uint32_t)(dgain * 32);
        result = SC1330T_IsiWriteRegIss(handle, 0x3e06, coarseDgain);
        result |= SC1330T_IsiWriteRegIss(handle, 0x3e07, fineDgain);
    } else if (dgain >= 8 && dgain < 16) {
        coarseDgain = 7;
        fineDgain = (uint32_t)(dgain * 16);
        result = SC1330T_IsiWriteRegIss(handle, 0x3e06, coarseDgain);
        result |= SC1330T_IsiWriteRegIss(handle, 0x3e07, fineDgain);
    } else if (dgain >= 16 && dgain < 32) {
        coarseDgain = 15;
        fineDgain = (uint32_t)(dgain * 8);
        result = SC1330T_IsiWriteRegIss(handle, 0x3e06, coarseDgain);
        result |= SC1330T_IsiWriteRegIss(handle, 0x3e07, fineDgain);
    } else {
        TRACE(SC1330T_ERROR,"%s: Invalid sensor digital gain value.\n", __func__);
        return (RET_INVALID_PARM);
    }
    pSC1330TCtx->curDgain = dgain;
    pSC1330TCtx->aecCurGain = pSC1330TCtx->curAgain * pSC1330TCtx->curDgain;

    TRACE(SC1330T_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT SC1330T_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    const SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(SC1330T_INFO, "%s: (enter)\n", __func__);

    if (pSC1330TCtx == NULL) {
        TRACE(SC1330T_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorAGain->gain[ISI_LINEAR_PARAS]       = pSC1330TCtx->curAgain;

    TRACE(SC1330T_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT SC1330T_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    const SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(SC1330T_INFO, "%s: (enter)\n", __func__);

    if (pSC1330TCtx == NULL) {
        TRACE(SC1330T_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorDGain->gain[ISI_LINEAR_PARAS] = pSC1330TCtx->curDgain;


    TRACE(SC1330T_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT SC1330T_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(SC1330T_INFO, "%s: (enter)\n", __func__);
    uint16_t expLine = 0;
    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;

    if (!pSC1330TCtx) {
        TRACE(SC1330T_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = pSensorIntTime->intTime[ISI_LINEAR_PARAS] / pSC1330TCtx->oneLineExpTime;
    expLine = MIN(pSC1330TCtx->maxIntegrationLine, MAX(pSC1330TCtx->minIntegrationLine, expLine));
    TRACE(SC1330T_DEBUG, "%s: set expLine = 0x%04x\n", __func__, expLine);

    result =  SC1330T_IsiWriteRegIss(handle, 0x3e00, (expLine >> 12) & 0x0f);
    result |= SC1330T_IsiWriteRegIss(handle, 0x3e01, (expLine >> 4) & 0xff);
    result |= SC1330T_IsiWriteRegIss(handle, 0x3e02, (expLine & 0x0f) << 4);

    pSC1330TCtx->aecCurIntegrationTime = expLine * pSC1330TCtx->oneLineExpTime;


    TRACE(SC1330T_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT SC1330T_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    const SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(SC1330T_INFO, "%s: (enter)\n", __func__);

    if (!pSC1330TCtx) {
        TRACE(SC1330T_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    pSensorIntTime->intTime[ISI_LINEAR_PARAS] = pSC1330TCtx->aecCurIntegrationTime;

    TRACE(SC1330T_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT SC1330T_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t * pFps)
{
    const SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(SC1330T_INFO, "%s: (enter)\n", __func__);

    if (pSC1330TCtx == NULL) {
        TRACE(SC1330T_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pSC1330TCtx->currFps;

    TRACE(SC1330T_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT SC1330T_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    RESULT result = RET_SUCCESS;
    int32_t newVts = 0;

    TRACE(SC1330T_INFO, "%s: (enter)\n", __func__);

    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    if (pSC1330TCtx == NULL) {
        TRACE(SC1330T_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

   if (fps > pSC1330TCtx->maxFps) {
        TRACE(SC1330T_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, fps, pSC1330TCtx->maxFps, pSC1330TCtx->minFps,pSC1330TCtx->maxFps);
        fps = pSC1330TCtx->maxFps;
    }
    if (fps < pSC1330TCtx->minFps) {
        TRACE(SC1330T_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, fps, pSC1330TCtx->minFps, pSC1330TCtx->minFps,pSC1330TCtx->maxFps);
        fps = pSC1330TCtx->minFps;
    }
     
    newVts = pSC1330TCtx->frameLengthLines*pSC1330TCtx->sensorMode.fps / fps;
    result =  SC1330T_IsiWriteRegIss(handle, 0x320e, newVts >> 8);
    result |=  SC1330T_IsiWriteRegIss(handle, 0x320f, newVts & 0xff);
    pSC1330TCtx->currFps              = fps;
    pSC1330TCtx->curFrameLengthLines  = newVts;
    pSC1330TCtx->maxIntegrationLine   = pSC1330TCtx->curFrameLengthLines - 4;
    pSC1330TCtx->aecMaxIntegrationTime = pSC1330TCtx->maxIntegrationLine * pSC1330TCtx->oneLineExpTime;

    TRACE(SC1330T_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT SC1330T_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    const SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    if (pSC1330TCtx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(SC1330T_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    pIspStatus->useSensorAWB = false;

    TRACE(SC1330T_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT SC1330T_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(SC1330T_INFO, "%s: (enter)\n", __func__);

    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    if (pSC1330TCtx == NULL || pSC1330TCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pSC1330TCtx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = SC1330T_IsiWriteRegIss(handle, 0x4501, 0xc4);
        result |= SC1330T_IsiWriteRegIss(handle, 0x3902, 0xc5);
    } else {
        result = SC1330T_IsiWriteRegIss(handle, 0x4501, 0xbc);
        result |= SC1330T_IsiWriteRegIss(handle, 0x3902, 0x95);
    }

    pSC1330TCtx->testPattern = tpg.enable;

    TRACE(SC1330T_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT SC1330T_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(SC1330T_INFO, "%s: (enter)\n", __func__);

    SC1330T_Context_t *pSC1330TCtx = (SC1330T_Context_t *) handle;
    if (pSC1330TCtx == NULL || pSC1330TCtx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pSC1330TCtx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!SC1330T_IsiReadRegIss(handle, 0x4501,&value)) {
        pTpg->enable = ((value & 0x08) != 0) ? 1 : 0;
        if(pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pSC1330TCtx->testPattern = pTpg->enable;
    }

    TRACE(SC1330T_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT SC1330T_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "SC1330T";
    TRACE(SC1330T_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = SC1330T_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = SC1330T_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = SC1330T_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = SC1330T_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = SC1330T_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = SC1330T_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = SC1330T_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = SC1330T_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = SC1330T_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = SC1330T_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = SC1330T_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = SC1330T_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = SC1330T_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = SC1330T_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = SC1330T_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = SC1330T_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = SC1330T_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = SC1330T_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = SC1330T_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = SC1330T_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = SC1330T_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = SC1330T_IsiGetIspStatusIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = SC1330T_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = SC1330T_IsiGetTpgIss;

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

    TRACE(SC1330T_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t SC1330T_IsiCamDrvConfig = {
    .cameraDriverID      = 0xca18,
    .pIsiGetSensorIss    = SC1330T_IsiGetSensorIss,
};
