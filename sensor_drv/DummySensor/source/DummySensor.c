// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Synaptics Incorporated

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

#include "DummySensor_priv.h"

CREATE_TRACER(DUMMYSENSOR_INFO , "DummySensor: ", INFO,    1);
CREATE_TRACER(DUMMYSENSOR_ERRO, "DummySensor: ", ERROR,   1);

#define DUMMYSENSOR_UNITY_GAIN               1.0f
/* Chip ID */
#define DUMMYSENSOR_CHIP_ID                  0x0101

/* Mode configs */
static IsiSensorMode_t pdummysensor_mode_info[] = {
    {
        .index         = 0,
        .size      = {
            .boundsWidth  = 5760,
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
        .fps          = 60 * ISI_FPS_QUANTIZE,
        .hdrMode      = ISI_SENSOR_MODE_LINEAR,
        .bitWidth     = 8,
        .bayerPattern = ISI_BPAT_RGGB,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
        .mipiLane = ISI_MIPI_4LANES,
        .dphyFreq = 445000,
    },
    {
        .index         = 1,
        .size      = {
            .boundsWidth  = 5760,
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
        .fps          = 60 * ISI_FPS_QUANTIZE,
        .hdrMode      = ISI_SENSOR_MODE_LINEAR,
        .bitWidth     = 8,
        .bayerPattern = ISI_BPAT_RGGB,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
        .mipiLane = ISI_MIPI_4LANES,
        .dphyFreq = 445000,
    },
};

static RESULT DummySensor_IsiGetModeIss(IsiSensorHandle_t handle,
        IsiSensorMode_t *pMode)
{
    TRACE(DUMMYSENSOR_INFO, "%s (enter)\n", __func__);

    const DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    if (pDummySensorCtx == NULL)
        return RET_WRONG_HANDLE;

    memcpy(pMode, &(pDummySensorCtx->sensorMode), sizeof(pDummySensorCtx->sensorMode));

    TRACE(DUMMYSENSOR_INFO, "%s (exit)\n", __func__);

    return RET_SUCCESS;
}

static RESULT DummySensor_IsiEnumModeIss(IsiSensorHandle_t handle,
        IsiSensorEnumMode_t *pEnumMode)
{
    const DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    if (pDummySensorCtx == NULL)
        return RET_NULL_POINTER;

    if (pEnumMode->index >= (sizeof(pdummysensor_mode_info)/sizeof(pdummysensor_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pdummysensor_mode_info)/sizeof(pdummysensor_mode_info[0])); i++) {
        if (pdummysensor_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pdummysensor_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(DUMMYSENSOR_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT DummySensor_IsiGetCapsIss(IsiSensorHandle_t handle,
        IsiCaps_t *pCaps)
{
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(DUMMYSENSOR_INFO, "%s (enter)\n", __func__);

    if (pDummySensorCtx == NULL)
        return RET_WRONG_HANDLE;

    if (pCaps == NULL)
        return RET_NULL_POINTER;

    pCaps->bitWidth          = pDummySensorCtx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pDummySensorCtx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pDummySensorCtx->sensorMode.size.width;
    pCaps->resolution.height = pDummySensorCtx->sensorMode.size.height;
    pCaps->mipiLanes         = pDummySensorCtx->sensorMode.mipiLane;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;
    pCaps->dphyFreq          = pDummySensorCtx->sensorMode.dphyFreq;
    if (pDummySensorCtx->sensorMode.index == 0)
        pCaps->mipiMode      = ISI_FORMAT_YUV422P_8;
    else
        pCaps->mipiMode      = ISI_FORMAT_RGB888;

    TRACE(DUMMYSENSOR_INFO, "%s (exit)\n", __func__);

    return result;
}

static RESULT DummySensor_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig,
        IsiSensorHandle_t *pHandle)
{
    RESULT result = RET_SUCCESS;

    TRACE(DUMMYSENSOR_INFO, "%s (enter)\n", __func__);

    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) malloc(sizeof(DummySensor_Context_t));
    if (!pDummySensorCtx) {
        TRACE(DUMMYSENSOR_ERRO, "%s: Can't allocate dummysensor context\n",__func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pDummySensorCtx, 0, sizeof(DummySensor_Context_t));

    pDummySensorCtx->isiCtx.pSensor     = pConfig->pSensor;
    pDummySensorCtx->groupHold          = BOOL_FALSE;
    pDummySensorCtx->oldGain            = 0;
    pDummySensorCtx->oldIntegrationTime = 0;
    pDummySensorCtx->configured         = BOOL_FALSE;
    pDummySensorCtx->streaming          = BOOL_FALSE;
    pDummySensorCtx->testPattern        = BOOL_FALSE;
    pDummySensorCtx->isAfpsRun          = BOOL_FALSE;
    pDummySensorCtx->sensorMode.index   = 0;

    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x55;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pDummySensorCtx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr,
            sccbConfig.addrByte, sccbConfig.dataByte);
    if (pDummySensorCtx->isiCtx.halI2cHandle == NULL) {
        TRACE(DUMMYSENSOR_ERRO, "%s: hal I2c open dev error!\n", __func__);
        return RET_NULL_POINTER;
    }

    result = HalAddRef(pDummySensorCtx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pDummySensorCtx);
        return result;
    }

    *pHandle = (IsiSensorHandle_t) pDummySensorCtx;

    TRACE(DUMMYSENSOR_INFO, "%s (exit)\n", __func__);

    return result;
}

static RESULT DummySensor_AecSetModeParameters(IsiSensorHandle_t handle,
        DummySensor_Context_t * pDummySensorCtx)
{
    RESULT result = RET_SUCCESS;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    pDummySensorCtx->aecIntegrationTimeIncrement = 1.0;
    pDummySensorCtx->aecMinIntegrationTime = 1.0;
    pDummySensorCtx->aecMaxIntegrationTime = 1.0;

    pDummySensorCtx->aecGainIncrement = 1.0;
    pDummySensorCtx->aecCurIntegrationTime = 1.0;
    pDummySensorCtx->aecCurLongIntegrationTime = 1.0;

    pDummySensorCtx->aecCurGain = 1.0;
    pDummySensorCtx->aecCurLongGain = 0;

    pDummySensorCtx->oldGain               = 0;
    pDummySensorCtx->oldIntegrationTime    = 0;

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

static RESULT DummySensor_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(DUMMYSENSOR_INFO, "%s (enter)\n", __func__);

    if (!pDummySensorCtx) {
        TRACE(DUMMYSENSOR_ERRO,"%s: Invalid sensor handle\n", __func__);
        return RET_WRONG_HANDLE;
    }

    if (pDummySensorCtx->streaming != BOOL_FALSE)
        return RET_WRONG_STATE;

    pDummySensorCtx->sensorMode.index   = mode;

    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i = 0; i < sizeof(pdummysensor_mode_info) / sizeof(IsiSensorMode_t); i++) {
        if (pdummysensor_mode_info[i].index == pDummySensorCtx->sensorMode.index) {
            SensorDefaultMode = &(pdummysensor_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        memcpy(&(pDummySensorCtx->sensorMode), SensorDefaultMode, sizeof(IsiSensorMode_t));
    } else {
        TRACE(DUMMYSENSOR_INFO, "%s: Invalid SensorDefaultMode\n", __func__);
        return RET_NULL_POINTER;
    }

    switch(pDummySensorCtx->sensorMode.index) {
        case 0:
        case 1:
            pDummySensorCtx->oneLineExpTime      = DUMMYSENSOR_UNITY_GAIN;
            pDummySensorCtx->frameLengthLines    = DUMMYSENSOR_UNITY_GAIN;
            pDummySensorCtx->curFrameLengthLines = pDummySensorCtx->frameLengthLines;
            pDummySensorCtx->maxIntegrationLine  = pDummySensorCtx->frameLengthLines - 4;
            pDummySensorCtx->minIntegrationLine  = 1;
            pDummySensorCtx->aecMinGain          = DUMMYSENSOR_UNITY_GAIN;
            pDummySensorCtx->aecMaxGain          = DUMMYSENSOR_UNITY_GAIN;
            pDummySensorCtx->aGain.min           = DUMMYSENSOR_UNITY_GAIN;
            pDummySensorCtx->aGain.max           = DUMMYSENSOR_UNITY_GAIN;
            pDummySensorCtx->aGain.step          = DUMMYSENSOR_UNITY_GAIN;
            pDummySensorCtx->dGain.min           = DUMMYSENSOR_UNITY_GAIN;
            pDummySensorCtx->dGain.max           = DUMMYSENSOR_UNITY_GAIN;
            pDummySensorCtx->dGain.step          = DUMMYSENSOR_UNITY_GAIN;
            break;
        default:
            return (RET_NOTAVAILABLE);
            break;
    }

    pDummySensorCtx->maxFps  = pDummySensorCtx->sensorMode.fps;
    pDummySensorCtx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pDummySensorCtx->currFps = pDummySensorCtx->maxFps;

    result = DummySensor_AecSetModeParameters(handle, pDummySensorCtx);
    if (result != RET_SUCCESS) {
        TRACE(DUMMYSENSOR_ERRO, "%s: SetupOutputWindow failed.\n", __func__);
        return result;
    }

    pDummySensorCtx->configured = BOOL_TRUE;
    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return 0;
}

static RESULT DummySensor_IsiCloseIss(IsiSensorHandle_t handle)
{
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(DUMMYSENSOR_INFO, "%s (enter)\n", __func__);

    if (pDummySensorCtx == NULL)
        return RET_WRONG_HANDLE;

    (void)DummySensor_IsiSetStreamingIss(pDummySensorCtx, BOOL_FALSE);

    TRACE(DUMMYSENSOR_INFO, "%s (exit)\n", __func__);

    return result;
}

static RESULT DummySensor_IsiReleaseIss(IsiSensorHandle_t handle)
{
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(DUMMYSENSOR_INFO, "%s (enter)\n", __func__);

    if (pDummySensorCtx == NULL)
        return RET_WRONG_HANDLE;

    MEMSET(pDummySensorCtx, 0, sizeof(DummySensor_Context_t));
    free(pDummySensorCtx);

    TRACE(DUMMYSENSOR_INFO,  "%s (exit)\n", __func__);

    return result;
}

static RESULT DummySensor_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;
    uint32_t correctId = DUMMYSENSOR_CHIP_ID;
    uint32_t sensorId = 0;

    TRACE(DUMMYSENSOR_INFO,  "%s (enter)\n", __func__);

    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    if (pDummySensorCtx == NULL || pDummySensorCtx->isiCtx.halI2cHandle == NULL)
        return RET_NULL_POINTER;

    result = DummySensor_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(DUMMYSENSOR_ERRO, "%s: Read Sensor ID Error! \n",__func__);
        return RET_FAILURE;
    }

    if (correctId != sensorId) {
        TRACE(DUMMYSENSOR_ERRO, "%s:ChipID =0x%x sensorId=%x error! \n", __func__,
                correctId, sensorId);
        return RET_FAILURE;
    }

    TRACE(DUMMYSENSOR_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__,
            correctId, sensorId);

    TRACE(DUMMYSENSOR_INFO, "%s (exit)\n", __func__);

    return result;
}

static RESULT DummySensor_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t * pValue)
{
    RESULT result = RET_SUCCESS;
    uint32_t sensorId = DUMMYSENSOR_CHIP_ID;

    TRACE(DUMMYSENSOR_INFO, "%s (enter)\n", __func__);

    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    if (pDummySensorCtx == NULL || pDummySensorCtx->isiCtx.halI2cHandle == NULL)
        return RET_NULL_POINTER;

    if (!pValue)
        return RET_NULL_POINTER;

    *pValue = sensorId;

    TRACE(DUMMYSENSOR_INFO,  "%s (exit)\n", __func__);

    return result;
}

static RESULT DummySensor_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;

    TRACE(DUMMYSENSOR_INFO, "%s (enter)\n", __func__);

    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    if (pDummySensorCtx == NULL || pDummySensorCtx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pDummySensorCtx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    pDummySensorCtx->streaming = on;

    TRACE(DUMMYSENSOR_INFO, "%s (exit)\n", __func__);

    return result;
}

static RESULT DummySensor_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle,
        IsiAeBaseInfo_t *pAeBaseInfo)
{
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (pDummySensorCtx == NULL) {
        TRACE(DUMMYSENSOR_ERRO,"%s: Invalid sensor handle  \n", __func__);
        return RET_WRONG_HANDLE;
    }

    if (pAeBaseInfo == NULL) {
        TRACE(DUMMYSENSOR_ERRO, "%s: NULL pointer received!!\n", __func__);
        return RET_NULL_POINTER;
    }

    pAeBaseInfo->gain.min        = pDummySensorCtx->aecMinGain;
    pAeBaseInfo->gain.max        = pDummySensorCtx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pDummySensorCtx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pDummySensorCtx->aecMaxIntegrationTime;

    pAeBaseInfo->aGain           = pDummySensorCtx->aGain;
    pAeBaseInfo->dGain           = pDummySensorCtx->dGain;

    pAeBaseInfo->aecCurGain      = pDummySensorCtx->aecCurGain;
    pAeBaseInfo->aecCurIntTime   = pDummySensorCtx->aecCurIntegrationTime;
    pAeBaseInfo->aecGainStep     = pDummySensorCtx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep  = pDummySensorCtx->aecIntegrationTimeIncrement;

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT DummySensor_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *)handle;

    if (!pDummySensorCtx || !pSensorAGain) {
        TRACE(DUMMYSENSOR_ERRO, "%s: Invalid parameter\n", __func__);
        return RET_NULL_POINTER;
    }

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    pDummySensorCtx->curAgain = DUMMYSENSOR_UNITY_GAIN;
    pDummySensorCtx->curDgain = DUMMYSENSOR_UNITY_GAIN;
    pDummySensorCtx->aecCurGain = DUMMYSENSOR_UNITY_GAIN;

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT DummySensor_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    if (pDummySensorCtx == NULL || pDummySensorCtx->isiCtx.halI2cHandle == NULL)
        return RET_NULL_POINTER;

    pDummySensorCtx->curDgain = DUMMYSENSOR_UNITY_GAIN;

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT DummySensor_IsiGetAGainIss(IsiSensorHandle_t handle,
        IsiSensorGain_t *pSensorAGain)
{
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (pDummySensorCtx == NULL) {
        TRACE(DUMMYSENSOR_ERRO,"%s: Invalid sensor handle\n", __func__);
        return RET_WRONG_HANDLE;
    }

    if (pSensorAGain == NULL)
        return RET_NULL_POINTER;

    pSensorAGain->gain[ISI_LINEAR_PARAS] = pDummySensorCtx->curAgain;

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT DummySensor_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (pDummySensorCtx == NULL) {
        TRACE(DUMMYSENSOR_ERRO,"%s: Invalid sensor handle\n", __func__);
        return RET_WRONG_HANDLE;
    }

    if (pSensorDGain == NULL)
        return RET_NULL_POINTER;

    pSensorDGain->gain[ISI_LINEAR_PARAS] = pDummySensorCtx->curDgain;

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT DummySensor_IsiSetIntTimeIss(IsiSensorHandle_t handle,
        const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (!pDummySensorCtx) {
        TRACE(DUMMYSENSOR_ERRO,"%s: Invalid sensor handle  \n", __func__);
        return RET_WRONG_HANDLE;
    }

    result = DummySensor_SetIntTime(handle, pSensorIntTime->intTime[ISI_LINEAR_PARAS]);
    if (result != RET_SUCCESS) {
        TRACE(DUMMYSENSOR_INFO, "%s: set sensor IntTime[ISI_LINEAR_PARAS] error!\n", __func__);
        return RET_FAILURE;
    }

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

static RESULT DummySensor_SetIntTime(IsiSensorHandle_t handle, float newIntegrationTime)
{
    RESULT result = RET_SUCCESS;
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *)handle;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (!pDummySensorCtx) {
        TRACE(DUMMYSENSOR_ERRO, "%s: Invalid sensor handle  \n", __func__);
        return RET_WRONG_HANDLE;
    }

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT DummySensor_IsiGetIntTimeIss(IsiSensorHandle_t handle,
        IsiSensorIntTime_t *pSensorIntTime)
{
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (!pDummySensorCtx) {
        TRACE(DUMMYSENSOR_ERRO,"%s: Invalid sensor handle  \n",__func__);
        return RET_WRONG_HANDLE;
    }

    if (!pSensorIntTime) return RET_NULL_POINTER;

    pSensorIntTime->intTime[ISI_TRI_EXP_S_PARAS] = pDummySensorCtx->aecCurIntegrationTime;

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT DummySensor_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t * pFps)
{
    const DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (pDummySensorCtx == NULL) {
        TRACE(DUMMYSENSOR_ERRO,"%s: Invalid sensor handle  \n", __func__);
        return RET_WRONG_HANDLE;
    }

    *pFps = pDummySensorCtx->currFps;

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT DummySensor_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (pDummySensorCtx == NULL) {
        TRACE(DUMMYSENSOR_ERRO,"%s: Invalid sensor handle\n", __func__);
        return RET_WRONG_HANDLE;
    }

    if (fps > pDummySensorCtx->maxFps)
        fps = pDummySensorCtx->maxFps;

    if (fps < pDummySensorCtx->minFps)
        fps = pDummySensorCtx->minFps;

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT DummySensor_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (pDummySensorCtx == NULL)
        return RET_WRONG_HANDLE;

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorAWB = false;
    pIspStatus->useSensorBLC = false;

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return RET_SUCCESS;
}

static RESULT DummySensor_IsiSetBlcIss(IsiSensorHandle_t handle, const IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (pDummySensorCtx == NULL || pDummySensorCtx->isiCtx.halI2cHandle == NULL)
        return RET_WRONG_HANDLE;

    if (pBlc == NULL)
        return RET_NULL_POINTER;

    memcpy(&pDummySensorCtx->sensorBlc, pBlc, sizeof(IsiSensorBlc_t));

    if (result != RET_SUCCESS) {
        TRACE(DUMMYSENSOR_ERRO, "%s: set sensor blc error\n", __func__);
        return RET_FAILURE;
    }

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

static RESULT DummySensor_IsiGetBlcIss(IsiSensorHandle_t handle, IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    const DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (pDummySensorCtx == NULL)
        return RET_WRONG_HANDLE;

    if (pBlc == NULL)
        return RET_NULL_POINTER;

    memcpy(pBlc, &pDummySensorCtx->sensorBlc, sizeof(IsiSensorBlc_t));

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

static RESULT DummySensor_IsiSetWBIss(IsiSensorHandle_t handle, const IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (pDummySensorCtx == NULL || pDummySensorCtx->isiCtx.halI2cHandle == NULL)
        return RET_WRONG_HANDLE;

    if (pWb == NULL)
        return RET_NULL_POINTER;

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

static RESULT DummySensor_IsiGetWBIss(IsiSensorHandle_t handle, IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    const DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (pDummySensorCtx == NULL )
        return RET_WRONG_HANDLE;

    if (pWb == NULL)
        return RET_NULL_POINTER;

    memcpy(pWb, &pDummySensorCtx->sensorWb, sizeof(IsiSensorWb_t));

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT DummySensor_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (pDummySensorCtx == NULL || pDummySensorCtx->isiCtx.halI2cHandle == NULL)
        return RET_NULL_POINTER;

    if (pDummySensorCtx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    pDummySensorCtx->testPattern = tpg.enable;

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT DummySensor_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (pDummySensorCtx == NULL || pDummySensorCtx->isiCtx.halI2cHandle == NULL
            || pTpg == NULL)
        return RET_NULL_POINTER;

    if (pDummySensorCtx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

static RESULT DummySensor_IsiGetExpandCurveIss(IsiSensorHandle_t handle,
        IsiSensorCompandCurve_t *pCurve)
{
    RESULT result = RET_SUCCESS;
    DummySensor_Context_t *pDummySensorCtx = (DummySensor_Context_t *) handle;

    TRACE(DUMMYSENSOR_INFO, "%s: (enter)\n", __func__);

    if (pDummySensorCtx == NULL || pDummySensorCtx->isiCtx.halI2cHandle == NULL)
        return RET_NULL_POINTER;

    TRACE(DUMMYSENSOR_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT DummySensor_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "DummySensor";

    TRACE(DUMMYSENSOR_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = DummySensor_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = DummySensor_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = DummySensor_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = DummySensor_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = NULL;
        pIsiSensor->pIsiWriteRegIss                     = NULL;
        pIsiSensor->pIsiGetModeIss                      = DummySensor_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = DummySensor_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = DummySensor_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = DummySensor_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = DummySensor_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = DummySensor_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = DummySensor_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = DummySensor_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = DummySensor_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = DummySensor_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = DummySensor_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = DummySensor_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = DummySensor_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = DummySensor_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = DummySensor_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = DummySensor_IsiGetIspStatusIss;
        pIsiSensor->pIsiSetBlcIss                       = DummySensor_IsiSetBlcIss;
        pIsiSensor->pIsiGetBlcIss                       = DummySensor_IsiGetBlcIss;
        pIsiSensor->pIsiSetWBIss                        = DummySensor_IsiSetWBIss;
        pIsiSensor->pIsiGetWBIss                        = DummySensor_IsiGetWBIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = DummySensor_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = DummySensor_IsiGetTpgIss;
        pIsiSensor->pIsiGetExpandCurveIss               = DummySensor_IsiGetExpandCurveIss;

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

    TRACE(DUMMYSENSOR_INFO, "%s (exit)\n", __func__);

    return result;
}

/*****************************************************************************
 * each sensor driver need to declare this struct for isi load
 *****************************************************************************/
IsiCamDrvConfig_t DummySensor_IsiCamDrvConfig = {
    .cameraDriverID      = 0x101,
    .pIsiGetSensorIss    = DummySensor_IsiGetSensorIss,
};
