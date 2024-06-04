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
#include <math.h>
#include "isi.h"
#include "isi_iss.h"
#include "IMX327_priv.h"

CREATE_TRACER(IMX327_INFO , "IMX327: ", INFO,    1);
CREATE_TRACER(IMX327_WARN , "IMX327: ", WARNING, 1);
CREATE_TRACER(IMX327_ERROR, "IMX327: ", ERROR,   1);
CREATE_TRACER(IMX327_DEBUG,     "IMX327: ", INFO, 1);
CREATE_TRACER(IMX327_REG_INFO , "IMX327: ", INFO, 1);
CREATE_TRACER(IMX327_REG_DEBUG, "IMX327: ", INFO, 1);

#define IMX327_MIN_GAIN_STEP    (1.0f/16.0f)  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain) */
#define IMX327_MAX_GAIN_AEC     (32.0f)       /**< max. gain used by the AEC (arbitrarily chosen, hardware limit = 62.0, driver limit = 32.0) */
#define IMX327_PLL_PCLK         74250000
#define IMX327_HMAX             0x1130
#define IMX327_VMAX             0x465

/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pimx327_mode_info[] = {
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
        .bayerPattern = ISI_BPAT_RGGB,
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
        .stitchingMode = ISI_SENSOR_STITCHING_2DOL,
        .bitWidth = 12,
        .bayerPattern = ISI_BPAT_RGGB,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    }
};

static RESULT IMX327_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s (enter)\n", __func__);

    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    if (pIMX327Ctx == NULL || pIMX327Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pIMX327Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(IMX327_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(IMX327_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT IMX327_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s (enter)\n", __func__);

    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    if (pIMX327Ctx == NULL || pIMX327Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pIMX327Ctx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(IMX327_ERROR, "%s: hal write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(IMX327_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT IMX327_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(IMX327_INFO, "%s (enter)\n", __func__);
    const IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    if (pIMX327Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pIMX327Ctx->sensorMode), sizeof(pIMX327Ctx->sensorMode));

    TRACE(IMX327_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT IMX327_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(IMX327_INFO, "%s (enter)\n", __func__);
    const IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    if (pIMX327Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pimx327_mode_info)/sizeof(pimx327_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pimx327_mode_info)/sizeof(pimx327_mode_info[0])); i++) {
        if (pimx327_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pimx327_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(IMX327_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT IMX327_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(IMX327_INFO, "%s (enter)\n", __func__);

    if (pIMX327Ctx == NULL) return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    } else {
        pCaps->bitWidth          = pIMX327Ctx->sensorMode.bitWidth;
        pCaps->mode              = ISI_MODE_BAYER;
        pCaps->bayerPattern      = pIMX327Ctx->sensorMode.bayerPattern;
        pCaps->resolution.width  = pIMX327Ctx->sensorMode.size.width;
        pCaps->resolution.height = pIMX327Ctx->sensorMode.size.height;
        pCaps->mipiLanes         = ISI_MIPI_4LANES;
        pCaps->vinType           = ISI_ITF_TYPE_MIPI;

        if (pCaps->bitWidth == 10) {
            pCaps->mipiMode      = ISI_FORMAT_RAW_10;
        } else if (pCaps->bitWidth == 12) {
            pCaps->mipiMode      = ISI_FORMAT_RAW_12;
        } else {
            pCaps->mipiMode      = ISI_MIPI_OFF;
        }
    }

    TRACE(IMX327_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX327_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s (enter)\n", __func__);

    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) malloc(sizeof(IMX327_Context_t));
    if (!pIMX327Ctx) {
        TRACE(IMX327_ERROR, "%s: Can't allocate IMX327 context\n",__func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pIMX327Ctx, 0, sizeof(IMX327_Context_t));

    pIMX327Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pIMX327Ctx->groupHold          = BOOL_FALSE;
    pIMX327Ctx->oldGain            = 1.0;
    pIMX327Ctx->oldIntegrationTime = 0.01;
    pIMX327Ctx->configured         = BOOL_FALSE;
    pIMX327Ctx->streaming          = BOOL_FALSE;
    pIMX327Ctx->testPattern        = BOOL_FALSE;
    pIMX327Ctx->isAfpsRun          = BOOL_FALSE;
    pIMX327Ctx->sensorMode.index   = 0;
    
    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x34;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pIMX327Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pIMX327Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(IMX327_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pIMX327Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pIMX327Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pIMX327Ctx;

    TRACE(IMX327_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX327_AecSetModeParameters(IsiSensorHandle_t handle, IMX327_Context_t * pIMX327Ctx)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s%s: (enter)\n", __func__,pIMX327Ctx->isAfpsRun ? "(AFPS)" : "");

    pIMX327Ctx->aecIntegrationTimeIncrement = pIMX327Ctx->oneLineExpTime;
    pIMX327Ctx->aecMinIntegrationTime = 0.001;
    pIMX327Ctx->aecMaxIntegrationTime = 0.033;

    TRACE(IMX327_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__,pIMX327Ctx->isAfpsRun ? "(AFPS)" : "",pIMX327Ctx->aecMaxIntegrationTime);

    pIMX327Ctx->aecGainIncrement = IMX327_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pIMX327Ctx->aecCurGain = pIMX327Ctx->aecMinGain;
    pIMX327Ctx->aecCurIntegrationTime = 0.0f;
    pIMX327Ctx->oldGain = 0;
    pIMX327Ctx->oldIntegrationTime = 0;

    TRACE(IMX327_INFO, "%s%s: (exit)\n", __func__,pIMX327Ctx->isAfpsRun ? "(AFPS)" : "");
    return (result);
}

static RESULT IMX327_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode) 
{
    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX327_INFO, "%s (enter)\n", __func__);

    if (!pIMX327Ctx) {
        TRACE(IMX327_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pIMX327Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pIMX327Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pimx327_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (pimx327_mode_info[i].index == pIMX327Ctx->sensorMode.index) {
            SensorDefaultMode = &(pimx327_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(IMX327_mipi4lane_1080p_init) / sizeof(IMX327_mipi4lane_1080p_init[0]); i++) {
                    IMX327_IsiWriteRegIss(handle, IMX327_mipi4lane_1080p_init[i][0], IMX327_mipi4lane_1080p_init[i][1]);
                }
                break;
            default:
                break;
        }

        memcpy(&(pIMX327Ctx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));
    } else {
        TRACE(IMX327_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pIMX327Ctx->sensorMode.index) {
        case 0:
            pIMX327Ctx->oneLineExpTime      = IMX327_HMAX/IMX327_PLL_PCLK;
            pIMX327Ctx->frameLengthLines    = 0x465;
            pIMX327Ctx->curFrameLengthLines = pIMX327Ctx->frameLengthLines;
            pIMX327Ctx->maxIntegrationLine  = pIMX327Ctx->curFrameLengthLines - 3;
            pIMX327Ctx->minIntegrationLine  = 1;
            pIMX327Ctx->aecMaxGain          = 24;
            pIMX327Ctx->aecMinGain          = 1;
            pIMX327Ctx->aGain.min           = 1.0;
            pIMX327Ctx->aGain.max           = 24;
            pIMX327Ctx->aGain.step          = (1.0f/128.0f);
            pIMX327Ctx->dGain.min           = 1;
            pIMX327Ctx->dGain.max           = 1;
            pIMX327Ctx->dGain.step          = 1;
            break;
        case 1:
            pIMX327Ctx->oneLineExpTime = IMX327_HMAX/IMX327_PLL_PCLK;
            pIMX327Ctx->frameLengthLines =  0x465;
            pIMX327Ctx->curFrameLengthLines = pIMX327Ctx->frameLengthLines;
            pIMX327Ctx->maxIntegrationLine = pIMX327Ctx->curFrameLengthLines - 3;
            pIMX327Ctx->minIntegrationLine = 1;
            pIMX327Ctx->aecMaxGain = 21;
            pIMX327Ctx->aecMinGain = 3;
            break;
        default:
            return (RET_NOTAVAILABLE);
            break;
    }

    pIMX327Ctx->maxFps  = pIMX327Ctx->sensorMode.fps;
    pIMX327Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pIMX327Ctx->currFps = pIMX327Ctx->maxFps;

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(IMX327_DEBUG, "%s: IMX327 System-Reset executed\n", __func__);
    osSleep(100);

    result = IMX327_AecSetModeParameters(handle, pIMX327Ctx);
    if (result != RET_SUCCESS) {
        TRACE(IMX327_ERROR, "%s: SetupOutputWindow failed.\n",__func__);
        return (result);
    }

    pIMX327Ctx->configured = BOOL_TRUE;
    TRACE(IMX327_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT IMX327_IsiCloseIss(IsiSensorHandle_t handle)
{
    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s (enter)\n", __func__);

    if (pIMX327Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)IMX327_IsiSetStreamingIss(pIMX327Ctx, BOOL_FALSE);

    TRACE(IMX327_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX327_IsiReleaseIss(IsiSensorHandle_t handle)
{
    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s (enter)\n", __func__);

    if (pIMX327Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pIMX327Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pIMX327Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pIMX327Ctx, 0, sizeof(IMX327_Context_t));
    free(pIMX327Ctx);
    TRACE(IMX327_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX327_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;
    uint32_t correctId = 0x2049;
    uint32_t sensorId = 0;

    TRACE(IMX327_INFO, "%s (enter)\n", __func__);

    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    if (pIMX327Ctx == NULL || pIMX327Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = IMX327_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(IMX327_ERROR, "%s: Read Sensor ID Error! \n",__func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(IMX327_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n",__func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(IMX327_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__,correctId, sensorId);
    TRACE(IMX327_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX327_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(IMX327_INFO, "%s (enter)\n", __func__);

    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    if (pIMX327Ctx == NULL || pIMX327Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (!pValue) return (RET_NULL_POINTER);

    regVal = 0;
    result = IMX327_IsiReadRegIss(handle, 0x3405, &regVal);
    sensorId = (regVal & 0xff) << 8;
    regVal = 0;
    result |= IMX327_IsiReadRegIss(handle, 0x3480, &regVal);
    sensorId |= (regVal & 0xff);

    *pValue = sensorId;
    TRACE(IMX327_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX327_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s (enter)\n", __func__);

    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    if (pIMX327Ctx == NULL || pIMX327Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if ((pIMX327Ctx->configured != BOOL_TRUE)|| (pIMX327Ctx->streaming == on))
        return RET_WRONG_STATE;

    result |= IMX327_IsiWriteRegIss(handle, 0x3002, 0x00);
    result |= IMX327_IsiWriteRegIss(handle, 0x3000, 0x00);
    if (result != RET_SUCCESS) {
        TRACE(IMX327_ERROR, "%s: set sensor streaming error! \n",__func__);
        return (RET_FAILURE);
    }

    pIMX327Ctx->streaming = on;

    TRACE(IMX327_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX327_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX327_INFO, "%s: (enter)\n", __func__);

    if (pIMX327Ctx == NULL) {
        TRACE(IMX327_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(IMX327_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    //get time limit and total gain limit
    pAeBaseInfo->gain.min        = pIMX327Ctx->aecMinGain;
    pAeBaseInfo->gain.max        = pIMX327Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pIMX327Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pIMX327Ctx->aecMaxIntegrationTime;

    //get again/dgain info
    pAeBaseInfo->aGain           = pIMX327Ctx->aGain;
    pAeBaseInfo->dGain           = pIMX327Ctx->dGain;
    pAeBaseInfo->aecCurGain      = pIMX327Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime   = pIMX327Ctx->aecCurIntegrationTime;
    pAeBaseInfo->aecGainStep     = pIMX327Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep  = pIMX327Ctx->aecIntegrationTimeIncrement;
    pAeBaseInfo->stitchingMode   = pIMX327Ctx->sensorMode.stitchingMode;

    TRACE(IMX327_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT IMX327_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s: (enter)\n", __func__);
    uint32_t again = 0, vsagain = 0;

    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    if (pIMX327Ctx == NULL || pIMX327Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pIMX327Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        again = (uint32_t)(20 * log10(pSensorAGain->gain[ISI_LINEAR_PARAS]) * (10/3));
        result |= IMX327_IsiWriteRegIss(handle, 0x3001, 0x01);
        result |= IMX327_IsiWriteRegIss(handle, 0x3014, (again));
        result |= IMX327_IsiWriteRegIss(handle, 0x3001, 0x00);
        pIMX327Ctx->curAgain = (float)(pSensorAGain->gain[ISI_LINEAR_PARAS]);

    } else if (pIMX327Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        again = (uint32_t)(20 * log10(pSensorAGain->gain[ISI_DUAL_EXP_L_PARAS]) * (10/3));
        result |= IMX327_IsiWriteRegIss(handle, 0x3001, 0x01);
        result |= IMX327_IsiWriteRegIss(handle, 0x3014, (again));
        result |= IMX327_IsiWriteRegIss(handle, 0x3001, 0x00);
        pIMX327Ctx->curAgain = (float)(pSensorAGain->gain[ISI_DUAL_EXP_L_PARAS]);

        vsagain = (uint32_t)(20 * log10(pSensorAGain->gain[ISI_DUAL_EXP_S_PARAS]) * (10/3));
        result |= IMX327_IsiWriteRegIss(handle, 0x3001, 0x01);
        result |=IMX327_IsiWriteRegIss(handle, 0x3018, (vsagain & 0x00FF));
        result |=IMX327_IsiWriteRegIss(handle, 0x3019, (vsagain & 0x0700)>>8);
        result |= IMX327_IsiWriteRegIss(handle, 0x3001, 0x00);
        pIMX327Ctx->curVSAgain = pSensorAGain->gain[ISI_DUAL_EXP_S_PARAS];

    }  else {
        TRACE(IMX327_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(IMX327_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX327_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    return (result);
}

RESULT IMX327_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s: (enter)\n", __func__);

    if (pIMX327Ctx == NULL) {
        TRACE(IMX327_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    if(pIMX327Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        pSensorAGain->gain[ISI_LINEAR_PARAS]       = pIMX327Ctx->curAgain;

    } else if (pIMX327Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        pSensorAGain->gain[ISI_DUAL_EXP_L_PARAS]   = pIMX327Ctx->curAgain;
        pSensorAGain->gain[ISI_DUAL_EXP_S_PARAS]   = pIMX327Ctx->curVSAgain;

    } else {
        TRACE(IMX327_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(IMX327_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX327_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    return (result);
}

RESULT IMX327_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s: (enter)\n", __func__);
    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;

    if (!pIMX327Ctx) {
        TRACE(IMX327_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pIMX327Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        result = IMX327_SetIntTime(handle, pSensorIntTime->intTime[ISI_LINEAR_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(IMX327_INFO, "%s: set sensor IntTime[ISI_LINEAR_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

    } else if (pIMX327Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        result = IMX327_SetIntTime(handle, pSensorIntTime->intTime[ISI_DUAL_EXP_L_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(IMX327_INFO, "%s: set sensor IntTime[ISI_DUAL_EXP_L_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

        result = IMX327_SetVSIntTime(handle, pSensorIntTime->intTime[ISI_DUAL_EXP_S_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(IMX327_INFO, "%s: set sensor IntTime[ISI_DUAL_EXP_S_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

    }  else {
        TRACE(IMX327_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(IMX327_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT IMX327_SetIntTime(IsiSensorHandle_t handle, float newIntegrationTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s: (enter)\n", __func__);
    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pIMX327Ctx) {
        TRACE(IMX327_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = (uint32_t)(newIntegrationTime / pIMX327Ctx->oneLineExpTime);

    expLine = IMX327_VMAX - expLine + 1;
    if(expLine > IMX327_VMAX-2) expLine = IMX327_VMAX - 2;

    result |= IMX327_IsiWriteRegIss(handle, 0x3001, 0x01);
    result |= IMX327_IsiWriteRegIss(handle, 0x3020, (expLine & 0x0000FF));
    result |= IMX327_IsiWriteRegIss(handle, 0x3021, (expLine & 0x00FF00)>>8);
    result |= IMX327_IsiWriteRegIss(handle, 0x3022, (expLine & 0x030000)>>16);
    result |= IMX327_IsiWriteRegIss(handle, 0x3001, 0x00);

    pIMX327Ctx->aecCurIntegrationTime = expLine * pIMX327Ctx->oneLineExpTime;

    TRACE(IMX327_DEBUG, "%s: set IntTime = %f\n", __func__, pIMX327Ctx->aecCurIntegrationTime);
    TRACE(IMX327_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT IMX327_SetVSIntTime(IsiSensorHandle_t handle, float newIntegrationTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s: (enter)\n", __func__);
    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pIMX327Ctx) {
        TRACE(IMX327_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = (uint32_t)(newIntegrationTime / pIMX327Ctx->oneLineExpTime);
    expLine = 2200 - expLine +1;
    expLine = expLine > 5 ? expLine : 5;
    expLine = expLine < IMX327_VMAX - 2 ? expLine : IMX327_VMAX - 2;
    result |= IMX327_IsiWriteRegIss(handle, 0x3001, 0x01);
    result |= IMX327_IsiWriteRegIss(handle, 0x305c,(expLine & 0x0000ff));
    result |= IMX327_IsiWriteRegIss(handle, 0x305D,(expLine & 0x00ff00)>>8);
    result |= IMX327_IsiWriteRegIss(handle, 0x305e,(expLine & 0x070000)>>16);
    //result = IMX327_IsiWriteRegIss(handle, 0x3001, 0x00);

    pIMX327Ctx->aecCurVSIntegrationTime = expLine * pIMX327Ctx->oneLineExpTime;

    TRACE(IMX327_DEBUG, "%s: set VSIntTime = %f\n", __func__, pIMX327Ctx->aecCurVSIntegrationTime);
    TRACE(IMX327_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX327_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s: (enter)\n", __func__);

    if (!pIMX327Ctx) {
        TRACE(IMX327_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    if (pIMX327Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        pSensorIntTime->intTime[ISI_LINEAR_PARAS] = pIMX327Ctx->aecCurIntegrationTime;

    } else if (pIMX327Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        pSensorIntTime->intTime[ISI_DUAL_EXP_L_PARAS]   = pIMX327Ctx->aecCurIntegrationTime;
        pSensorIntTime->intTime[ISI_DUAL_EXP_S_PARAS]   = pIMX327Ctx->aecCurVSIntegrationTime;

    } else {
        TRACE(IMX327_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(IMX327_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX327_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t *pFps)
{
    const IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s: (enter)\n", __func__);

    if (pIMX327Ctx == NULL) {
        TRACE(IMX327_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pIMX327Ctx->currFps;

    TRACE(IMX327_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX327_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s: (enter)\n", __func__);
    if (pIMX327Ctx == NULL) {
        TRACE(IMX327_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (fps > pIMX327Ctx->maxFps) {
        TRACE(IMX327_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",
                            __func__, fps, pIMX327Ctx->maxFps, pIMX327Ctx->minFps,pIMX327Ctx->maxFps);
        fps = pIMX327Ctx->maxFps;
    }
    if (fps < pIMX327Ctx->minFps) {
        TRACE(IMX327_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", 
                           __func__, fps, pIMX327Ctx->minFps, pIMX327Ctx->minFps,pIMX327Ctx->maxFps);
        fps = pIMX327Ctx->minFps;
    }

    uint16_t NewVts;
    NewVts  = pIMX327Ctx->frameLengthLines * pIMX327Ctx->maxFps / fps;
    result  = IMX327_IsiWriteRegIss(handle, 0x301a, NewVts >> 16);
    result |= IMX327_IsiWriteRegIss(handle, 0x3019, ((NewVts >> 8) & 0xff));
    result |= IMX327_IsiWriteRegIss(handle, 0x3018, NewVts & 0xff);
    if (result != RET_SUCCESS) {
        TRACE(IMX327_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_FAILURE);
    }
    pIMX327Ctx->currFps = fps;
    pIMX327Ctx->curFrameLengthLines   = NewVts;
    pIMX327Ctx->maxIntegrationLine    = pIMX327Ctx->curFrameLengthLines - 3;
    pIMX327Ctx->aecMaxIntegrationTime = pIMX327Ctx->maxIntegrationLine *pIMX327Ctx->oneLineExpTime;

    TRACE(IMX327_INFO, "%s: set sensor fps = %d\n", __func__,pIMX327Ctx->currFps);
    TRACE(IMX327_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX327_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    const IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    if (pIMX327Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(IMX327_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    pIspStatus->useSensorAWB = false;

    TRACE(IMX327_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT IMX327_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX327_INFO, "%s: (enter)\n", __func__);

    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    if (pIMX327Ctx == NULL || pIMX327Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pIMX327Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = IMX327_IsiWriteRegIss(handle, 0x5081, 0x00);
    } else {
        result = IMX327_IsiWriteRegIss(handle, 0x5081, 0x80);
    }

    pIMX327Ctx->testPattern = tpg.enable;

    TRACE(IMX327_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX327_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(IMX327_INFO, "%s: (enter)\n", __func__);

    IMX327_Context_t *pIMX327Ctx = (IMX327_Context_t *) handle;
    if (pIMX327Ctx == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pIMX327Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!IMX327_IsiReadRegIss(handle, 0x5081,&value)) {
        pTpg->enable = ((value & 0x80) != 0) ? 1 : 0;
        if(pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pIMX327Ctx->testPattern = pTpg->enable;
    }

    TRACE(IMX327_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX327_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "IMX327";
    TRACE(IMX327_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = IMX327_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = IMX327_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = IMX327_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = IMX327_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = IMX327_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = IMX327_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = IMX327_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = IMX327_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = IMX327_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = IMX327_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = IMX327_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = IMX327_IsiSetStreamingIss;

        /* AEC functions */
        pIsiSensor->pIsiGetAeBaseInfoIss                = IMX327_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = IMX327_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = IMX327_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = IMX327_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = IMX327_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = IMX327_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = IMX327_IsiSetIntTimeIss;
		pIsiSensor->pIsiGetFpsIss                       = IMX327_IsiGetFpsIss;
		pIsiSensor->pIsiSetFpsIss                       = IMX327_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = IMX327_IsiGetIspStatusIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = IMX327_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = IMX327_IsiGetTpgIss;

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

    TRACE(IMX327_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t IMX327_IsiCamDrvConfig = {
    .cameraDriverID        = 0x2049,
    .pIsiGetSensorIss      = IMX327_IsiGetSensorIss,
};
