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
#include "OV16E10_priv.h"

CREATE_TRACER(OV16E10_INFO , "OV16E10: ", INFO,    1);
CREATE_TRACER(OV16E10_WARN , "OV16E10: ", WARNING, 1);
CREATE_TRACER(OV16E10_ERROR, "OV16E10: ", ERROR,   1);
CREATE_TRACER(OV16E10_DEBUG,     "OV16E10: ", INFO, 1);
CREATE_TRACER(OV16E10_REG_INFO , "OV16E10: ", INFO, 1);
CREATE_TRACER(OV16E10_REG_DEBUG, "OV16E10: ", INFO, 1);

#define OV16E10_MIN_GAIN_STEP    (1.0f/128.0f)  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain ) */
#define VCM_ADDR                 0x0c
#define MIN_VCM_POS              0x00
#define MAX_VCM_POS              0x3FF


/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pov16e10_mode_info[] = {
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
        .fps       = 8 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_CDAF,
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
        .fps       = 8 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_PDAF,
    },
    {
        .index     = 2,
        .size      = {
            .boundsWidth  = 3840,
            .boundsHeight = 2160,
            .top           = 0,
            .left          = 0,
            .width         = 3840,
            .height        = 2160,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps       = 5 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_CDAF,
    },
    {
        .index     = 3,
        .size      = {
            .boundsWidth  = 3840,
            .boundsHeight = 2160,
            .top           = 0,
            .left          = 0,
            .width         = 3840,
            .height        = 2160,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps       = 5 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_PDAF,

    },
};

static RESULT OV16E10_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s (enter)\n", __func__);

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL || pOV16E10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pOV16E10Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(OV16E10_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV16E10_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV16E10_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s (enter)\n", __func__);

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL || pOV16E10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pOV16E10Ctx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(OV16E10_ERROR, "%s: hal write sensor register error!\n",__func__);
        return (RET_FAILURE);
    }

    TRACE(OV16E10_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV16E10_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(OV16E10_INFO, "%s (enter)\n", __func__);
    const OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pOV16E10Ctx->sensorMode), sizeof(pOV16E10Ctx->sensorMode));

    TRACE(OV16E10_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT OV16E10_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(OV16E10_INFO, "%s (enter)\n", __func__);
    const OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pov16e10_mode_info)/sizeof(pov16e10_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pov16e10_mode_info)/sizeof(pov16e10_mode_info[0])); i++) {
        if (pov16e10_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pov16e10_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(OV16E10_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT OV16E10_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(OV16E10_INFO, "%s (enter)\n", __func__);

    if (pOV16E10Ctx == NULL)
        return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pOV16E10Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pOV16E10Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pOV16E10Ctx->sensorMode.size.width;
    pCaps->resolution.height = pOV16E10Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(OV16E10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV16E10_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s (enter)\n", __func__);

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) malloc(sizeof(OV16E10_Context_t));
    if (!pOV16E10Ctx) {
        TRACE(OV16E10_ERROR, "%s: Can't allocate ov16e10 context\n", __func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pOV16E10Ctx, 0, sizeof(OV16E10_Context_t));

    pOV16E10Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pOV16E10Ctx->groupHold          = BOOL_FALSE;
    pOV16E10Ctx->oldGain            = 0;
    pOV16E10Ctx->oldIntegrationTime = 0;
    pOV16E10Ctx->configured         = BOOL_FALSE;
    pOV16E10Ctx->streaming          = BOOL_FALSE;
    pOV16E10Ctx->testPattern        = BOOL_FALSE;
    pOV16E10Ctx->isAfpsRun          = BOOL_FALSE;
    pOV16E10Ctx->sensorMode.index   = 0;
    
    pOV16E10Ctx->i2cBusID = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x6c;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pOV16E10Ctx->isiCtx.halI2cHandle = HalI2cOpen(pOV16E10Ctx->i2cBusID, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pOV16E10Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(OV16E10_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pOV16E10Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pOV16E10Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pOV16E10Ctx;

    TRACE(OV16E10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV16E10_AecSetModeParameters(IsiSensorHandle_t handle, OV16E10_Context_t * pOV16E10Ctx)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0, regVal = 0;
    float again = 0, dgain = 0;
    TRACE(OV16E10_INFO, "%s%s: (enter)\n", __func__,pOV16E10Ctx->isAfpsRun ? "(AFPS)" : "");

    pOV16E10Ctx->aecIntegrationTimeIncrement = pOV16E10Ctx->oneLineExpTime;
    pOV16E10Ctx->aecMinIntegrationTime = pOV16E10Ctx->oneLineExpTime * pOV16E10Ctx->minIntegrationLine;
    pOV16E10Ctx->aecMaxIntegrationTime = pOV16E10Ctx->oneLineExpTime * pOV16E10Ctx->maxIntegrationLine;

    TRACE(OV16E10_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__,pOV16E10Ctx->isAfpsRun ? "(AFPS)" : "",pOV16E10Ctx->aecMaxIntegrationTime);

    pOV16E10Ctx->aecGainIncrement = OV16E10_MIN_GAIN_STEP;
    pOV16E10Ctx->oldGain = 0;
    pOV16E10Ctx->oldIntegrationTime = 0;

    OV16E10_IsiReadRegIss(handle, 0x3501, &value);
    regVal = value << 8;
    OV16E10_IsiReadRegIss(handle, 0x3502, &value);
    regVal = regVal | (value & 0xFF);
    pOV16E10Ctx->aecCurIntegrationTime = regVal * pOV16E10Ctx->oneLineExpTime;

    OV16E10_IsiReadRegIss(handle, 0x3508, &value);
    regVal = (value & 0x7f) << 7;
    OV16E10_IsiReadRegIss(handle, 0x3509, &value);
    regVal = regVal | ((value & 0xfe) >> 1);
    again = (float)regVal/128.0f;

    OV16E10_IsiReadRegIss(handle, 0x350a, &value);
    regVal = (value & 0x0f) << 10;
    OV16E10_IsiReadRegIss(handle, 0x350b, &value);
    regVal = regVal | ((value & 0xFF) << 2);
    OV16E10_IsiReadRegIss(handle, 0x350c, &value);
    regVal = regVal | ((value & 0xc0) >> 6);
    dgain = (float)regVal/1024.0f;
    pOV16E10Ctx->aecCurGain = again * dgain;

    TRACE(OV16E10_INFO, "%s%s: (exit)\n", __func__,pOV16E10Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT OV16E10_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OV16E10_INFO, "%s (enter)\n", __func__);

    if (!pOV16E10Ctx) {
        TRACE(OV16E10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOV16E10Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pOV16E10Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pov16e10_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (pov16e10_mode_info[i].index == pOV16E10Ctx->sensorMode.index) {
            SensorDefaultMode = &(pov16e10_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(OV16E10_mipi4lane_1080p_init) / sizeof(OV16E10_mipi4lane_1080p_init[0]); i++) {
                    OV16E10_IsiWriteRegIss(handle, OV16E10_mipi4lane_1080p_init[i][0], OV16E10_mipi4lane_1080p_init[i][1]);
                }
                break;
            case 1:
                for (int i = 0; i<sizeof(OV16E10_mipi4lane_1080p_PDAF_init) / sizeof(OV16E10_mipi4lane_1080p_PDAF_init[0]); i++) {
                    OV16E10_IsiWriteRegIss(handle, OV16E10_mipi4lane_1080p_PDAF_init[i][0], OV16E10_mipi4lane_1080p_PDAF_init[i][1]);
                }
                break;
            case 2:
                for (int i = 0; i<sizeof(OV16E10_mipi4lane_4k_init) / sizeof(OV16E10_mipi4lane_4k_init[0]); i++) {
                    OV16E10_IsiWriteRegIss(handle, OV16E10_mipi4lane_4k_init[i][0], OV16E10_mipi4lane_4k_init[i][1]);
                }
                break;
            case 3:
                for (int i = 0; i<sizeof(OV16E10_mipi4lane_4k_PDAF_init) / sizeof(OV16E10_mipi4lane_4k_PDAF_init[0]); i++) {
                    OV16E10_IsiWriteRegIss(handle, OV16E10_mipi4lane_4k_PDAF_init[i][0], OV16E10_mipi4lane_4k_PDAF_init[i][1]);
                }
                break;
            default:
                TRACE(OV16E10_INFO, "%s:not support sensor mode %d\n", __func__, pOV16E10Ctx->sensorMode.index);
                return RET_NOTSUPP;
                break;
        }

        memcpy(&(pOV16E10Ctx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));
    } else {
        TRACE(OV16E10_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pOV16E10Ctx->sensorMode.index) {
        case 0:
            pOV16E10Ctx->oneLineExpTime      = 0.000048;
            pOV16E10Ctx->frameLengthLines    = 0xa00;
            pOV16E10Ctx->curFrameLengthLines = pOV16E10Ctx->frameLengthLines;
            pOV16E10Ctx->maxIntegrationLine  = pOV16E10Ctx->frameLengthLines -16 ;
            pOV16E10Ctx->minIntegrationLine  = 8;
            pOV16E10Ctx->aecMaxGain          = 255;
            pOV16E10Ctx->aecMinGain          = 1.0;
            pOV16E10Ctx->aGain.min           = 1.0;
            pOV16E10Ctx->aGain.max           = 15.5;
            pOV16E10Ctx->aGain.step          = (1.0f/128.0f);
            pOV16E10Ctx->dGain.min           = 1.0;
            pOV16E10Ctx->dGain.max           = 15;
            pOV16E10Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 1:
            pOV16E10Ctx->oneLineExpTime      = 0.000048;
            pOV16E10Ctx->frameLengthLines    = 0xa00;
            pOV16E10Ctx->curFrameLengthLines = pOV16E10Ctx->frameLengthLines;
            pOV16E10Ctx->maxIntegrationLine  = pOV16E10Ctx->curFrameLengthLines - 16;
            pOV16E10Ctx->minIntegrationLine  = 8;
            pOV16E10Ctx->aecMaxGain          = 255;
            pOV16E10Ctx->aecMinGain          = 1.0;
            pOV16E10Ctx->aGain.min           = 1.0;
            pOV16E10Ctx->aGain.max           = 15.5;
            pOV16E10Ctx->aGain.step          = (1.0f/128.0f);
            pOV16E10Ctx->dGain.min           = 1.0;
            pOV16E10Ctx->dGain.max           = 15;
            pOV16E10Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 2:
            pOV16E10Ctx->oneLineExpTime      = 0.000075;
            pOV16E10Ctx->frameLengthLines    = 0xa68;
            pOV16E10Ctx->curFrameLengthLines = pOV16E10Ctx->frameLengthLines;
            pOV16E10Ctx->maxIntegrationLine  = pOV16E10Ctx->curFrameLengthLines - 16;
            pOV16E10Ctx->minIntegrationLine  = 8;
            pOV16E10Ctx->aecMaxGain          = 255;
            pOV16E10Ctx->aecMinGain          = 1.0;
            pOV16E10Ctx->aGain.min           = 1.0;
            pOV16E10Ctx->aGain.max           = 15.5;
            pOV16E10Ctx->aGain.step          = (1.0f/128.0f);
            pOV16E10Ctx->dGain.min           = 1.0;
            pOV16E10Ctx->dGain.max           = 15;
            pOV16E10Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 3:
            pOV16E10Ctx->oneLineExpTime      = 0.000075;
            pOV16E10Ctx->frameLengthLines    = 0xa68;
            pOV16E10Ctx->curFrameLengthLines = pOV16E10Ctx->frameLengthLines;
            pOV16E10Ctx->maxIntegrationLine  = pOV16E10Ctx->curFrameLengthLines - 16;
            pOV16E10Ctx->minIntegrationLine  = 8;
            pOV16E10Ctx->aecMaxGain          = 255;
            pOV16E10Ctx->aecMinGain          = 1.0;
            pOV16E10Ctx->aGain.min           = 1.0;
            pOV16E10Ctx->aGain.max           = 15.5;
            pOV16E10Ctx->aGain.step          = (1.0f/128.0f);
            pOV16E10Ctx->dGain.min           = 1.0;
            pOV16E10Ctx->dGain.max           = 15;
            pOV16E10Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        default:
            TRACE(OV16E10_INFO, "%s:not support sensor mode %d\n", __func__,pOV16E10Ctx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    pOV16E10Ctx->maxFps  = pOV16E10Ctx->sensorMode.fps;
    pOV16E10Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pOV16E10Ctx->currFps = pOV16E10Ctx->maxFps;

    TRACE(OV16E10_DEBUG, "%s: OV16E10 System-Reset executed\n", __func__);
    osSleep(100);

    result = OV16E10_AecSetModeParameters(handle, pOV16E10Ctx);
    if (result != RET_SUCCESS) {
        TRACE(OV16E10_ERROR, "%s: SetupOutputWindow failed.\n",__func__);
        return (result);
    }
    
    pOV16E10Ctx->configured = BOOL_TRUE;
    TRACE(OV16E10_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT OV16E10_IsiCloseIss(IsiSensorHandle_t handle)
{
    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s (enter)\n", __func__);

    if (pOV16E10Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)OV16E10_IsiSetStreamingIss(pOV16E10Ctx, BOOL_FALSE);

    TRACE(OV16E10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV16E10_IsiReleaseIss(IsiSensorHandle_t handle)
{
    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s (enter)\n", __func__);

    if (pOV16E10Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pOV16E10Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pOV16E10Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pOV16E10Ctx, 0, sizeof(OV16E10_Context_t));
    free(pOV16E10Ctx);
    TRACE(OV16E10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV16E10_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;

    uint32_t sensorId = 0;
    uint32_t correctId = 0x5616;

    TRACE(OV16E10_INFO, "%s (enter)\n", __func__);

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL || pOV16E10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = OV16E10_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(OV16E10_ERROR, "%s: Read Sensor ID Error! \n", __func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(OV16E10_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n", __func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(OV16E10_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__,correctId, sensorId);
    TRACE(OV16E10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV16E10_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(OV16E10_INFO, "%s (enter)\n", __func__);

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL || pOV16E10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    regVal = 0;
    result = OV16E10_IsiReadRegIss(handle, 0x300a, &regVal);
    sensorId = (regVal & 0xff) << 8;

    regVal = 0;
    result |= OV16E10_IsiReadRegIss(handle, 0x300b, &regVal);
    sensorId |= (regVal & 0xff);
    TRACE(OV16E10_INFO, "%s sensorId = %d \n", __func__,sensorId);

    *pValue = sensorId;
    TRACE(OV16E10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV16E10_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s (enter)\n", __func__);

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL || pOV16E10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV16E10Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    result = OV16E10_IsiWriteRegIss(handle, 0x0100, on);
    if (result != RET_SUCCESS) {
        TRACE(OV16E10_ERROR, "%s: set sensor streaming error! \n",__func__);
        return (RET_FAILURE);
    }

    pOV16E10Ctx->streaming = on;

    TRACE(OV16E10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV16E10_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    const OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);

    if (pOV16E10Ctx == NULL) {
        TRACE(OV16E10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(OV16E10_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    //get time limit and total gain limit
    pAeBaseInfo->gain.min        = pOV16E10Ctx->aecMinGain;
    pAeBaseInfo->gain.max        = pOV16E10Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pOV16E10Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pOV16E10Ctx->aecMaxIntegrationTime;

    //get again/dgain info
    pAeBaseInfo->aGain           = pOV16E10Ctx->aGain;
    pAeBaseInfo->dGain           = pOV16E10Ctx->dGain;
    
    pAeBaseInfo->aecCurGain      = pOV16E10Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime   = pOV16E10Ctx->aecCurIntegrationTime;
    pAeBaseInfo->aecGainStep     = pOV16E10Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep  = pOV16E10Ctx->aecIntegrationTimeIncrement;

    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT OV16E10_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);
    uint32_t again = 0; 

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL || pOV16E10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    again = (uint32_t)(pSensorAGain->gain[ISI_LINEAR_PARAS] * 128);
    result = OV16E10_IsiWriteRegIss(handle, 0x3508,  (again & 0x3f80)>>7);
    result |= OV16E10_IsiWriteRegIss(handle, 0x3509, (again & 0x7f)<<1);
    pOV16E10Ctx->curAgain = (float)again/128.0f;

    TRACE(OV16E10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV16E10_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);
    uint32_t dgain = 0; 

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL || pOV16E10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    dgain = (uint32_t)((pSensorDGain->gain[ISI_LINEAR_PARAS]) * 1024);
    result = OV16E10_IsiWriteRegIss(handle,  0x350a, (dgain & 0x3c00)>>10);
    result |= OV16E10_IsiWriteRegIss(handle, 0x350b, (dgain & 0x3fc)>>2);
    result |= OV16E10_IsiWriteRegIss(handle, 0x350c, (dgain & 0x3)<<6);
    pOV16E10Ctx->curDgain = (float)dgain/1024.0f;
    pOV16E10Ctx->aecCurGain = pOV16E10Ctx->curAgain * pOV16E10Ctx->curDgain;

    TRACE(OV16E10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV16E10_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    const OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);

    if (pOV16E10Ctx == NULL) {
        TRACE(OV16E10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorAGain->gain[ISI_LINEAR_PARAS] = pOV16E10Ctx->curAgain;

    TRACE(OV16E10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV16E10_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    const OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);

    if (pOV16E10Ctx == NULL) {
        TRACE(OV16E10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorDGain->gain[ISI_LINEAR_PARAS] = pOV16E10Ctx->curDgain;

    TRACE(OV16E10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV16E10_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);
    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pOV16E10Ctx) {
        TRACE(OV16E10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = pSensorIntTime->intTime[ISI_LINEAR_PARAS] / pOV16E10Ctx->oneLineExpTime;
    expLine = MIN(pOV16E10Ctx->maxIntegrationLine, MAX(pOV16E10Ctx->minIntegrationLine, expLine));
    TRACE(OV16E10_DEBUG, "%s: set expLine=0x%04x\n", __func__, expLine);

    result =  OV16E10_IsiWriteRegIss(handle, 0x3501,(expLine >>8) & 0xff);
    result |= OV16E10_IsiWriteRegIss(handle, 0x3502,(expLine & 0xff));

    pOV16E10Ctx->aecCurIntegrationTime = expLine * pOV16E10Ctx->oneLineExpTime;

    TRACE(OV16E10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV16E10_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    const OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);

    if (!pOV16E10Ctx) {
        TRACE(OV16E10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    pSensorIntTime->intTime[ISI_LINEAR_PARAS] = pOV16E10Ctx->aecCurIntegrationTime;

    TRACE(OV16E10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV16E10_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t *pFps)
{
    const OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);

    if (pOV16E10Ctx == NULL) {
        TRACE(OV16E10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pOV16E10Ctx->currFps;

    TRACE(OV16E10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV16E10_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    RESULT result = RET_SUCCESS;
    int32_t NewVts = 0;
    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL) {
        TRACE(OV16E10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (fps > pOV16E10Ctx->maxFps) {
        TRACE(OV16E10_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, fps, pOV16E10Ctx->maxFps, pOV16E10Ctx->minFps,pOV16E10Ctx->maxFps);
        fps = pOV16E10Ctx->maxFps;
    }
    if (fps < pOV16E10Ctx->minFps) {
        TRACE(OV16E10_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",
              __func__, fps, pOV16E10Ctx->minFps, pOV16E10Ctx->minFps,pOV16E10Ctx->maxFps);
        fps = pOV16E10Ctx->minFps;
    }

    NewVts = pOV16E10Ctx->frameLengthLines*pOV16E10Ctx->sensorMode.fps / fps;
    result =  OV16E10_IsiWriteRegIss(handle, 0x380e, NewVts >> 8);
    result |=  OV16E10_IsiWriteRegIss(handle, 0x380f, NewVts & 0xff);
    pOV16E10Ctx->currFps              = fps;
    pOV16E10Ctx->curFrameLengthLines  = NewVts;
    pOV16E10Ctx->maxIntegrationLine   = pOV16E10Ctx->curFrameLengthLines - 8;
    //pOV16E10Ctx->aecMaxIntegrationTime = pOV16E10Ctx->maxIntegrationLine * pOV16E10Ctx->oneLineExpTime;

    TRACE(OV16E10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV16E10_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    const OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    pIspStatus->useSensorAWB = false;

    TRACE(OV16E10_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT OV16E10_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL || pOV16E10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV16E10Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = OV16E10_IsiWriteRegIss(handle, 0x5081, 0x00);
    } else {
        result = OV16E10_IsiWriteRegIss(handle, 0x5081, 0x80);
    }

    pOV16E10Ctx->testPattern = tpg.enable;

    TRACE(OV16E10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV16E10_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL || pOV16E10Ctx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV16E10Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!OV16E10_IsiReadRegIss(handle, 0x5081,&value)) {
        pTpg->enable = ((value & 0x80) != 0) ? 1 : 0;
        if(pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pOV16E10Ctx->testPattern = pTpg->enable;
    }

    TRACE(OV16E10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV16E10_IsiVCMReadRegIss(IsiSensorHandle_t handle, uint8_t addr, uint16_t *data)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    uint16_t regAddr = addr;
    result = HalReadI2CReg(pOV16E10Ctx->halMotorI2cHandle, regAddr, data);
    if (result != RET_SUCCESS) {
        TRACE(OV16E10_ERROR, "%s: hal read VCM register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV16E10_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;

}

static RESULT OV16E10_IsiVCMWriteRegIss(IsiSensorHandle_t handle, const uint16_t param)
{
    uint16_t regAddr = 0;
    uint16_t regData = 0;
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    regAddr = (param >> 8) & 0xff;
    regData = (param & 0xff) << 8;
    result = HalWriteI2CReg(pOV16E10Ctx->halMotorI2cHandle, regAddr, regData);
    if (result != RET_SUCCESS) {
        TRACE(OV16E10_ERROR, "%s: hal write VCM register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV16E10_INFO, "%s: (exit)\n", __func__);
    return result;
}

RESULT OV16E10_IsiFocusCreateIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);
    uint16_t param = 0;

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x0c << 1;
    sccbConfig.addrByte  = 1;
    sccbConfig.dataByte  = 2;

    pOV16E10Ctx->halMotorI2cHandle = HalI2cOpen(pOV16E10Ctx->i2cBusID, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pOV16E10Ctx->halMotorI2cHandle == NULL) {
        TRACE(OV16E10_ERROR, "%s: hal I2c open Motor error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pOV16E10Ctx->halMotorI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pOV16E10Ctx);
        return (result);
    }

    /* w2w1w0:000  limite current mode , set to min vcm position */
    param |= 0xc000;
    param |= MIN_VCM_POS;
    result = OV16E10_IsiVCMWriteRegIss(handle, param);
    if (result != RET_SUCCESS) {
        TRACE(OV16E10_ERROR, "%s: init vcm failed\n",__func__);
        return (RET_FAILURE);
    }

    return result;
}

RESULT OV16E10_IsiFocusReleaseIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;

    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    HalI2cClose(pOV16E10Ctx->halMotorI2cHandle);
    (void)HalDelRef(pOV16E10Ctx->halMotorI2cHandle, HAL_DEV_I2C);

    return result;
}


RESULT OV16E10_IsiFocusGetCalibrateIss(IsiSensorHandle_t handle, IsiFocusCalibAttr_t *pFocusCalib)
{
    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);

    OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    switch (pOV16E10Ctx->sensorMode.index)
    {
    case 1:
        pFocusCalib->pdInfo.sensorType     = ISI_PDAF_SENSOR_OCL2X1;
        pFocusCalib->pdInfo.ocl2x1Shield   = 0;
        pFocusCalib->pdInfo.bayerPattern   = ISI_BPAT_BGGR;
        pFocusCalib->pdInfo.bitWidth       = 16;//10bit
        pFocusCalib->pdInfo.imageWidth     = 1920;
        pFocusCalib->pdInfo.imageHeight    = 1080;
        pFocusCalib->pdInfo.pdArea[0]      = 8;  //relate to sensor configure
        pFocusCalib->pdInfo.pdArea[1]      = 4;
        // if filp or mirror, pd position will change at the same time
        pFocusCalib->pdInfo.pdShiftL2R[0]  = 1;
        pFocusCalib->pdInfo.pdShiftL2R[1]  = 0;
        MEMSET(&pFocusCalib->pdInfo.pdShiftMark, 0, sizeof(pFocusCalib->pdInfo.pdShiftMark));
        pFocusCalib->pdInfo.pdShiftMark[0] = 6;
        pFocusCalib->pdInfo.pdShiftMark[1] = 2;
        pFocusCalib->pdInfo.pdShiftMark[2] = 14;
        pFocusCalib->pdInfo.pdShiftMark[3] = 2;
        pFocusCalib->pdInfo.pdShiftMark[4] = 2;
        pFocusCalib->pdInfo.pdShiftMark[5] = 10;
        pFocusCalib->pdInfo.pdShiftMark[6] = 10;
        pFocusCalib->pdInfo.pdShiftMark[7] = 10;

        pFocusCalib->pdInfo.correctRect[0] = 0;
        pFocusCalib->pdInfo.correctRect[1] = 0;
        pFocusCalib->pdInfo.correctRect[2] = 1920;
        pFocusCalib->pdInfo.correctRect[3] = 1080;
        break;
    
    case 2:
        pFocusCalib->pdInfo.sensorType     = ISI_PDAF_SENSOR_OCL2X1;
        pFocusCalib->pdInfo.ocl2x1Shield   = 0;
        pFocusCalib->pdInfo.bayerPattern   = ISI_BPAT_BGGR;
        pFocusCalib->pdInfo.bitWidth       = 10;
        pFocusCalib->pdInfo.imageWidth     = 3840;
        pFocusCalib->pdInfo.imageHeight    = 2160;
        pFocusCalib->pdInfo.pdArea[0]      = 0;
        pFocusCalib->pdInfo.pdArea[1]      = 0;

        pFocusCalib->pdInfo.pdShiftL2R[0]  = 1;
        pFocusCalib->pdInfo.pdShiftL2R[1]  = 0;
        MEMSET(&pFocusCalib->pdInfo.pdShiftMark, 0, sizeof(pFocusCalib->pdInfo.pdShiftMark));
        pFocusCalib->pdInfo.pdShiftMark[0] = 5;
        pFocusCalib->pdInfo.pdShiftMark[1] = 2;
        pFocusCalib->pdInfo.pdShiftMark[2] = 13;
        pFocusCalib->pdInfo.pdShiftMark[3] = 2;
        pFocusCalib->pdInfo.pdShiftMark[4] = 1;
        pFocusCalib->pdInfo.pdShiftMark[5] = 10;
        pFocusCalib->pdInfo.pdShiftMark[6] = 9;
        pFocusCalib->pdInfo.pdShiftMark[7] = 10;

        pFocusCalib->pdInfo.correctRect[0] = 0;
        pFocusCalib->pdInfo.correctRect[1] = 0;
        pFocusCalib->pdInfo.correctRect[2] = 3840;
        pFocusCalib->pdInfo.correctRect[3] = 2160;
        break;
    default:
        // ISI_PDAF_ISI_SENSOR_TYPE_MAX means not support PDAF
        pFocusCalib->pdInfo.sensorType = ISI_PDAF_SENSOR_TYPE_MAX;
        TRACE(OV16E10_WARN, "%s: mode index %d is not support PDAF\n",__func__, pOV16E10Ctx->sensorMode.index);
        break;
    }

    pFocusCalib->pdInfo.pdArea[2]       = 16;
    pFocusCalib->pdInfo.pdArea[3]       = 16;
    pFocusCalib->pdInfo.pdNumPerArea[0] = 2;
    pFocusCalib->pdInfo.pdNumPerArea[1] = 2;
    pFocusCalib->pdInfo.pdShiftL2R[0]   = 1;
    pFocusCalib->pdInfo.pdShiftL2R[1]   = 0;
    pFocusCalib->pdInfo.pdFocalHeigh    = 3;
    pFocusCalib->pdInfo.pdFocalWidth    = 3;
    int pdFocalInit[48] ={-38,-38,-38,-38,-38,-38,-38,-38,-38,-38,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    MEMCPY(&pFocusCalib->pdInfo.pdFocal, pdFocalInit, sizeof(pdFocalInit));

    pFocusCalib->posInfo.minPos  = MIN_VCM_POS;
    pFocusCalib->posInfo.maxPos  = MAX_VCM_POS;
    pFocusCalib->posInfo.minStep = 2;

    return RET_SUCCESS;
}

RESULT OV16E10_IsiFocusSetIss(IsiSensorHandle_t handle, const IsiFocusPos_t *pPos)
{
    RESULT result = RET_SUCCESS;
    uint16_t param = 0;

    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);

    const OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    /* w2w1w0:000  limite current mode */
    param |= 0xc000;
    param |= (pPos->pos & 0x3FF);
    result = OV16E10_IsiVCMWriteRegIss(handle, param);
    if (result != RET_SUCCESS) {
        TRACE(OV16E10_ERROR, "%s: send i2c message failed\n",__func__);
    }

    return RET_SUCCESS;
}

RESULT OV16E10_IsiFocusGetIss(IsiSensorHandle_t handle, IsiFocusPos_t *pPos)
{
    RESULT result = RET_SUCCESS;
    uint8_t addr;
    uint16_t regVal = 0;

    TRACE(OV16E10_INFO, "%s: (enter)\n", __func__);

    const OV16E10_Context_t *pOV16E10Ctx = (OV16E10_Context_t *) handle;
    if (pOV16E10Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    /* w2w1w0:000  limite current mode */
    addr = 0xc0;
    result = OV16E10_IsiVCMReadRegIss(handle, addr, &regVal);
    if (result != RET_SUCCESS) {
        return RET_FAILURE;
    }

    pPos->posType = ISI_FOCUS_POS_ABSOLUTE;
    pPos->pos = regVal & 0x3FF;

    return result;
}

static RESULT OV16E10_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "OV16E10";
    TRACE(OV16E10_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = OV16E10_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = OV16E10_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = OV16E10_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = OV16E10_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = OV16E10_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = OV16E10_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = OV16E10_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = OV16E10_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = OV16E10_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = OV16E10_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = OV16E10_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = OV16E10_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = OV16E10_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = OV16E10_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = OV16E10_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = OV16E10_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = OV16E10_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = OV16E10_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = OV16E10_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = OV16E10_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = OV16E10_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = OV16E10_IsiGetIspStatusIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = OV16E10_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = OV16E10_IsiGetTpgIss;

        /* AF */
        pIsiSensor->pIsiFocusCreateIss                  = OV16E10_IsiFocusCreateIss;
        pIsiSensor->pIsiFocusReleaseIss                 = OV16E10_IsiFocusReleaseIss;
        pIsiSensor->pIsiFocusGetCalibrateIss            = OV16E10_IsiFocusGetCalibrateIss;
        pIsiSensor->pIsiFocusSetIss                     = OV16E10_IsiFocusSetIss;
        pIsiSensor->pIsiFocusGetIss                     = OV16E10_IsiFocusGetIss;
        /* metadata*/
        pIsiSensor->pIsiQueryMetadataAttrIss            = NULL;
        pIsiSensor->pIsiSetMetadataAttrEnableIss        = NULL;
        pIsiSensor->pIsiGetMetadataAttrEnableIss        = NULL;
        pIsiSensor->pIsiGetMetadataWinIss               = NULL;
        pIsiSensor->pIsiParserMetadataIss               = NULL;

    } else {
        result = RET_NULL_POINTER;
    }

    TRACE(OV16E10_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t OV16E10_IsiCamDrvConfig = {
    .cameraDriverID      = 0x5616,
    .pIsiGetSensorIss    = OV16E10_IsiGetSensorIss,
};
