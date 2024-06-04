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
#include "OS08a20_priv.h"

CREATE_TRACER(OS08a20_INFO , "OS08a20: ", INFO,    1);
CREATE_TRACER(OS08a20_WARN , "OS08a20: ", WARNING, 1);
CREATE_TRACER(OS08a20_ERROR, "OS08a20: ", ERROR,   1);
CREATE_TRACER(OS08a20_DEBUG,     "OS08a20: ", INFO, 0);
CREATE_TRACER(OS08a20_REG_INFO , "OS08a20: ", INFO, 1);
CREATE_TRACER(OS08a20_REG_DEBUG, "OS08a20: ", INFO, 1);

#define OS08a20_MIN_GAIN_STEP    (1.0f/128.0f)  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain) */

/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pos08a20_mode_info[] = {
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
        .fps       = 24 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
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
        .fps       = 12 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_HDR_STITCH,
        .stitchingMode = ISI_SENSOR_STITCHING_L_AND_S,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
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
        .fps       = 3.73 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 12,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
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
        .fps       = 2 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_HDR_STITCH,
        .stitchingMode = ISI_SENSOR_STITCHING_L_AND_S,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 4,
        .size      = {
            .boundsWidth  = 3200,
            .boundsHeight = 1800,
            .top           = 0,
            .left          = 0,
            .width         = 3200,
            .height        = 1800,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps       = 3.73 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 12,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 5,
        .size      = {
            .boundsWidth  = 2560,
            .boundsHeight = 1440,
            .top           = 0,
            .left          = 0,
            .width         = 2560,
            .height        = 1440,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps       = 3.73 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 12,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 6,//mcm
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
        .fps       = 12 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 7,//VGA 30fps
        .size      = {
            .boundsWidth  = 640,
            .boundsHeight = 480,
            .top           = 0,
            .left          = 0,
            .width         = 640,
            .height        = 480,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps       = 30 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 12,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 8,//VI200 4K
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
        .fps       = 1.84 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
};

static RESULT OS08a20_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s (enter)\n", __func__);

    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    if (pOS08a20Ctx == NULL || pOS08a20Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pOS08a20Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(OS08a20_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OS08a20_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OS08a20_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s (enter)\n", __func__);

    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    if (pOS08a20Ctx == NULL || pOS08a20Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(OS08a20_ERROR, "%s: NULL POINTER!\n", __func__);
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pOS08a20Ctx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(OS08a20_ERROR, "%s: hal write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OS08a20_INFO, "%s addr 0x%x, value 0x%x (exit) result = %d\n", __func__, result, addr, value);
    return (result);
}

static RESULT OS08a20_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(OS08a20_INFO, "%s (enter)\n", __func__);
    const OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    if (pOS08a20Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pOS08a20Ctx->sensorMode), sizeof(pOS08a20Ctx->sensorMode));

    TRACE(OS08a20_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT OS08a20_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(OS08a20_INFO, "%s (enter)\n", __func__);
    const OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    if (pOS08a20Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pos08a20_mode_info)/sizeof(pos08a20_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pos08a20_mode_info)/sizeof(pos08a20_mode_info[0])); i++) {
        if (pos08a20_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pos08a20_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(OS08a20_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT OS08a20_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(OS08a20_INFO, "%s (enter)\n", __func__);

    if (pOS08a20Ctx == NULL) return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pOS08a20Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pOS08a20Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pOS08a20Ctx->sensorMode.size.width;
    pCaps->resolution.height = pOS08a20Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(OS08a20_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS08a20_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s (enter)\n", __func__);

    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) malloc(sizeof(OS08a20_Context_t));
    if (!pOS08a20Ctx) {
        TRACE(OS08a20_ERROR, "%s: Can't allocate os08a20 context\n",__func__);
        return (RET_OUTOFMEM);
    }
    MEMSET(pOS08a20Ctx, 0, sizeof(OS08a20_Context_t));

    pOS08a20Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pOS08a20Ctx->groupHold          = BOOL_FALSE;
    pOS08a20Ctx->oldGain            = 0;
    pOS08a20Ctx->oldIntegrationTime = 0;
    pOS08a20Ctx->configured         = BOOL_FALSE;
    pOS08a20Ctx->streaming          = BOOL_FALSE;
    pOS08a20Ctx->testPattern        = BOOL_FALSE;
    pOS08a20Ctx->isAfpsRun          = BOOL_FALSE;
    pOS08a20Ctx->sensorMode.index   = 0;

    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x6c;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pOS08a20Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pOS08a20Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(OS08a20_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pOS08a20Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pOS08a20Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pOS08a20Ctx;

    TRACE(OS08a20_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS08a20_AecSetModeParameters(IsiSensorHandle_t handle, OS08a20_Context_t * pOS08a20Ctx)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0, regVal = 0;
    float again = 0, dgain = 0;
    TRACE(OS08a20_INFO, "%s%s: (enter)\n", __func__, pOS08a20Ctx->isAfpsRun ? "(AFPS)" : "");

    pOS08a20Ctx->aecIntegrationTimeIncrement = pOS08a20Ctx->oneLineExpTime;
    pOS08a20Ctx->aecMinIntegrationTime       = pOS08a20Ctx->oneLineExpTime * pOS08a20Ctx->minIntegrationLine;
    pOS08a20Ctx->aecMaxIntegrationTime       = pOS08a20Ctx->oneLineExpTime * pOS08a20Ctx->maxIntegrationLine;
    //pOS08a20Ctx->aecMaxIntegrationTime        = 0.04f;
    pOS08a20Ctx->aecMinVSIntegrationTime     = pOS08a20Ctx->oneLineExpTime * pOS08a20Ctx->minVSIntegrationLine;
    pOS08a20Ctx->aecMaxVSIntegrationTime     = pOS08a20Ctx->oneLineExpTime * pOS08a20Ctx->maxVSIntegrationLine;

    TRACE(OS08a20_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pOS08a20Ctx->isAfpsRun ? "(AFPS)" : "",pOS08a20Ctx->aecMaxIntegrationTime);

    pOS08a20Ctx->aecGainIncrement = OS08a20_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pOS08a20Ctx->oldGain               = 0;
    pOS08a20Ctx->oldIntegrationTime    = 0;

    OS08a20_IsiReadRegIss(handle, 0x3508, &value);
    regVal = (value & 0x3f) << 8;
    OS08a20_IsiReadRegIss(handle, 0x3509, &value);
    regVal = regVal | (value & 0xFF);
    again = regVal/128.0f;
    
    OS08a20_IsiReadRegIss(handle, 0x350a, &value);
    regVal = (value & 0x3f) << 8;
    OS08a20_IsiReadRegIss(handle, 0x350b, &value);
    regVal = regVal | (value & 0xFF);
    dgain = regVal/1024.0f;
    pOS08a20Ctx->aecCurGain            = again * dgain;

    OS08a20_IsiReadRegIss(handle, 0x3501, &value);
    regVal = value << 8;
    OS08a20_IsiReadRegIss(handle, 0x3502, &value);
    regVal = regVal | (value & 0xFF);
    pOS08a20Ctx->aecCurIntegrationTime = regVal * pOS08a20Ctx->oneLineExpTime;

    TRACE(OS08a20_INFO, "%s%s: (exit)\n", __func__,pOS08a20Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT OS08a20_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OS08a20_INFO, "%s (enter)\n", __func__);

    if (!pOS08a20Ctx) {
        TRACE(OS08a20_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOS08a20Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pOS08a20Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pos08a20_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (pos08a20_mode_info[i].index == pOS08a20Ctx->sensorMode.index) {
            SensorDefaultMode = &(pos08a20_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(OS08a20_mipi4lane_1080p_init) / sizeof(OS08a20_mipi4lane_1080p_init[0]); i++) {
                    OS08a20_IsiWriteRegIss(handle, OS08a20_mipi4lane_1080p_init[i][0], OS08a20_mipi4lane_1080p_init[i][1]);
                }
                break;
            case 1:
                for (int i = 0; i<sizeof(OS08a20_mipi4lane_1080p_2dol_init) / sizeof(OS08a20_mipi4lane_1080p_2dol_init[0]); i++) {
                    OS08a20_IsiWriteRegIss(handle, OS08a20_mipi4lane_1080p_2dol_init[i][0], OS08a20_mipi4lane_1080p_2dol_init[i][1]);
                }
                break;
            case 2:
                for (int i = 0; i<sizeof(OS08a20_mipi4lane_4k_init) / sizeof(OS08a20_mipi4lane_4k_init[0]); i++) {
                    OS08a20_IsiWriteRegIss(handle, OS08a20_mipi4lane_4k_init[i][0], OS08a20_mipi4lane_4k_init[i][1]);
                }
                break;
            case 3:
                for (int i = 0; i<sizeof(OS08a20_mipi4lane_4k_2dol_init) / sizeof(OS08a20_mipi4lane_4k_2dol_init[0]); i++) {
                    OS08a20_IsiWriteRegIss(handle, OS08a20_mipi4lane_4k_2dol_init[i][0], OS08a20_mipi4lane_4k_2dol_init[i][1]);
                }
                break;
            case 4:
                for (int i = 0; i<sizeof(OS08a20_mipi4lane_3200_1800_init) / sizeof(OS08a20_mipi4lane_3200_1800_init[0]); i++) {
                    OS08a20_IsiWriteRegIss(handle, OS08a20_mipi4lane_3200_1800_init[i][0], OS08a20_mipi4lane_3200_1800_init[i][1]);
                }
                break;
            case 5:
                for (int i = 0; i<sizeof(OS08a20_mipi4lane_2560_1440_init) / sizeof(OS08a20_mipi4lane_2560_1440_init[0]); i++) {
                    OS08a20_IsiWriteRegIss(handle, OS08a20_mipi4lane_2560_1440_init[i][0], OS08a20_mipi4lane_2560_1440_init[i][1]);
                }
                break;
            case 6:
                for (int i = 0; i<sizeof(OS08a20_mipi4lane_1080p_mcm_init) / sizeof(OS08a20_mipi4lane_1080p_mcm_init[0]); i++) {
                    OS08a20_IsiWriteRegIss(handle, OS08a20_mipi4lane_1080p_mcm_init[i][0], OS08a20_mipi4lane_1080p_mcm_init[i][1]);
                }
                break;
            case 7:
                for (int i = 0; i<sizeof(OS08a20_mipi4lane_640_480_init) / sizeof(OS08a20_mipi4lane_640_480_init[0]); i++) {
                    OS08a20_IsiWriteRegIss(handle, OS08a20_mipi4lane_640_480_init[i][0], OS08a20_mipi4lane_640_480_init[i][1]);
                }
                break;
            case 8:
                for (int i = 0; i<sizeof(OS08a20_mipi4lane_4K_VI200_init) / sizeof(OS08a20_mipi4lane_4K_VI200_init[0]); i++) {
                    OS08a20_IsiWriteRegIss(handle, OS08a20_mipi4lane_4K_VI200_init[i][0], OS08a20_mipi4lane_4K_VI200_init[i][1]);
                }
                break;
            default:
                TRACE(OS08a20_INFO, "%s:not support sensor mode %d\n", __func__,pOS08a20Ctx->sensorMode.index);
                return RET_NOTSUPP;
                break;
    }

        memcpy(&(pOS08a20Ctx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));

    } else {
        TRACE(OS08a20_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pOS08a20Ctx->sensorMode.index) {
        case 0:
            pOS08a20Ctx->oneLineExpTime      = 0.000036;
            pOS08a20Ctx->frameLengthLines    = 0x486;
            pOS08a20Ctx->curFrameLengthLines = pOS08a20Ctx->frameLengthLines;
            pOS08a20Ctx->maxIntegrationLine  = pOS08a20Ctx->frameLengthLines -8 ;
            pOS08a20Ctx->minIntegrationLine  = 8;
            pOS08a20Ctx->aecMaxGain          = 230;
            pOS08a20Ctx->aecMinGain          = 1.0;
            pOS08a20Ctx->aGain.min           = 1.0;
            pOS08a20Ctx->aGain.max           = 15.5;
            pOS08a20Ctx->aGain.step          = (1.0f/128.0f);
            pOS08a20Ctx->dGain.min           = 1.0;
            pOS08a20Ctx->dGain.max           = 15;
            pOS08a20Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 1:
            pOS08a20Ctx->oneLineExpTime      = 0.000036 ; 
            pOS08a20Ctx->frameLengthLines    = 0x486;
            pOS08a20Ctx->curFrameLengthLines = pOS08a20Ctx->frameLengthLines;
            pOS08a20Ctx->maxIntegrationLine  = pOS08a20Ctx->curFrameLengthLines - 8;
            pOS08a20Ctx->minIntegrationLine  = 8;
            pOS08a20Ctx->aecMaxGain          = 230;
            pOS08a20Ctx->aecMinGain          = 1.0;
            #ifdef ISP_MI_HDR
            pOS08a20Ctx->maxVSIntegrationLine  = 550/2;
            pOS08a20Ctx->minVSIntegrationLine  = 4;
            #else
            pOS08a20Ctx->maxVSIntegrationLine  = 51;
            pOS08a20Ctx->minVSIntegrationLine  = 8;
            #endif
            pOS08a20Ctx->aGain.min           = 1.0;
            pOS08a20Ctx->aGain.max           = 15.5;
            pOS08a20Ctx->aGain.step          = (1.0f/128.0f);
            pOS08a20Ctx->dGain.min           = 1.0;
            pOS08a20Ctx->dGain.max           = 15;
            pOS08a20Ctx->dGain.step          = (1.0f/1024.0f);
            pOS08a20Ctx->aVSGain.min         = 1.0;
            pOS08a20Ctx->aVSGain.max         = 15.5;
            pOS08a20Ctx->aVSGain.step        = (1.0f/128.0f);
            pOS08a20Ctx->dVSGain.min         = 1.0;
            pOS08a20Ctx->dVSGain.max         = 15;
            pOS08a20Ctx->dVSGain.step        = (1.0f/1024.0f);
            break;
        case 2:
            pOS08a20Ctx->oneLineExpTime      = 0.000110; 
            pOS08a20Ctx->frameLengthLines    = 0x984;
            pOS08a20Ctx->curFrameLengthLines = pOS08a20Ctx->frameLengthLines;
            pOS08a20Ctx->maxIntegrationLine  = pOS08a20Ctx->curFrameLengthLines - 8;
            pOS08a20Ctx->minIntegrationLine  = 8;
            pOS08a20Ctx->aecMaxGain          = 230;
            pOS08a20Ctx->aecMinGain          = 1.0;
            pOS08a20Ctx->aGain.min           = 1.0;
            pOS08a20Ctx->aGain.max           = 15.5;
            pOS08a20Ctx->aGain.step          = (1.0f/128.0f);
            pOS08a20Ctx->dGain.min           = 1.0;
            pOS08a20Ctx->dGain.max           = 15;
            pOS08a20Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 3:
            pOS08a20Ctx->oneLineExpTime       = 0.000107 ;
            pOS08a20Ctx->frameLengthLines     = 0x900;
            pOS08a20Ctx->curFrameLengthLines  = pOS08a20Ctx->frameLengthLines;
            pOS08a20Ctx->maxIntegrationLine   = pOS08a20Ctx->curFrameLengthLines - 8;
            pOS08a20Ctx->minIntegrationLine   = 8;
            pOS08a20Ctx->aecMaxGain           = 230;
            pOS08a20Ctx->aecMinGain           = 1.0;
            #ifdef ISP_MI_HDR
            pOS08a20Ctx->maxVSIntegrationLine  = 550/2;
            pOS08a20Ctx->minVSIntegrationLine  = 4;
            #else
            pOS08a20Ctx->maxVSIntegrationLine  = 51;
            pOS08a20Ctx->minVSIntegrationLine  = 1;
            #endif
            pOS08a20Ctx->aGain.min           = 1.0;
            pOS08a20Ctx->aGain.max           = 15.5;
            pOS08a20Ctx->aGain.step          = (1.0f/128.0f);
            pOS08a20Ctx->dGain.min           = 1.0;
            pOS08a20Ctx->dGain.max           = 15;
            pOS08a20Ctx->dGain.step          = (1.0f/1024.0f);
            pOS08a20Ctx->aVSGain.min         = 1.0;
            pOS08a20Ctx->aVSGain.max         = 15.5;
            pOS08a20Ctx->aVSGain.step        = (1.0f/128.0f);
            pOS08a20Ctx->dVSGain.min         = 1.0;
            pOS08a20Ctx->dVSGain.max         = 15;
            pOS08a20Ctx->dVSGain.step        = (1.0f/1024.0f);
            break;
        case 4:
            pOS08a20Ctx->oneLineExpTime      = 0.000110; 
            pOS08a20Ctx->frameLengthLines    = 0x984;
            pOS08a20Ctx->curFrameLengthLines = pOS08a20Ctx->frameLengthLines;
            pOS08a20Ctx->maxIntegrationLine  = pOS08a20Ctx->curFrameLengthLines - 8;
            pOS08a20Ctx->minIntegrationLine  = 8;
            pOS08a20Ctx->aecMaxGain          = 230;
            pOS08a20Ctx->aecMinGain          = 1.0;
            pOS08a20Ctx->aGain.min           = 1.0;
            pOS08a20Ctx->aGain.max           = 15.5;
            pOS08a20Ctx->aGain.step          = (1.0f/128.0f);
            pOS08a20Ctx->dGain.min           = 1.0;
            pOS08a20Ctx->dGain.max           = 15;
            pOS08a20Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 5:
            pOS08a20Ctx->oneLineExpTime      = 0.000110;
            pOS08a20Ctx->frameLengthLines    = 0x984;
            pOS08a20Ctx->curFrameLengthLines = pOS08a20Ctx->frameLengthLines;
            pOS08a20Ctx->maxIntegrationLine  = pOS08a20Ctx->curFrameLengthLines - 8;
            pOS08a20Ctx->minIntegrationLine  = 8;
            pOS08a20Ctx->aecMaxGain          = 230;
            pOS08a20Ctx->aecMinGain          = 1.0;
            pOS08a20Ctx->aGain.min           = 1.0;
            pOS08a20Ctx->aGain.max           = 15.5;
            pOS08a20Ctx->aGain.step          = (1.0f/128.0f);
            pOS08a20Ctx->dGain.min           = 1.0;
            pOS08a20Ctx->dGain.max           = 15;
            pOS08a20Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 6:
            pOS08a20Ctx->oneLineExpTime      = 0.000072;
            pOS08a20Ctx->frameLengthLines    = 0x486;
            pOS08a20Ctx->curFrameLengthLines = pOS08a20Ctx->frameLengthLines;
            pOS08a20Ctx->maxIntegrationLine  = pOS08a20Ctx->curFrameLengthLines - 8;
            pOS08a20Ctx->minIntegrationLine  = 8;
            pOS08a20Ctx->aecMaxGain          = 230;
            pOS08a20Ctx->aecMinGain          = 1.0;
            pOS08a20Ctx->aGain.min           = 1.0;
            pOS08a20Ctx->aGain.max           = 15.5;
            pOS08a20Ctx->aGain.step          = (1.0f/128.0f);
            pOS08a20Ctx->dGain.min           = 1.0;
            pOS08a20Ctx->dGain.max           = 15;
            pOS08a20Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 7:
            pOS08a20Ctx->oneLineExpTime      = 0.000030;
            pOS08a20Ctx->frameLengthLines    = 0x440;
            pOS08a20Ctx->curFrameLengthLines = pOS08a20Ctx->frameLengthLines;
            pOS08a20Ctx->maxIntegrationLine  = pOS08a20Ctx->frameLengthLines -8 ;
            pOS08a20Ctx->minIntegrationLine  = 8;
            pOS08a20Ctx->aecMaxGain          = 230;
            pOS08a20Ctx->aecMinGain          = 1.0;
            pOS08a20Ctx->aGain.min           = 1.0;
            pOS08a20Ctx->aGain.max           = 15.5;
            pOS08a20Ctx->aGain.step          = (1.0f/128.0f);
            pOS08a20Ctx->dGain.min           = 1.0;
            pOS08a20Ctx->dGain.max           = 15;
            pOS08a20Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 8:
            pOS08a20Ctx->oneLineExpTime      = 0.000128;
            pOS08a20Ctx->frameLengthLines    = 0x1092;
            pOS08a20Ctx->curFrameLengthLines = pOS08a20Ctx->frameLengthLines;
            pOS08a20Ctx->maxIntegrationLine  = pOS08a20Ctx->frameLengthLines -8 ;
            pOS08a20Ctx->minIntegrationLine  = 8;
            pOS08a20Ctx->aecMaxGain          = 230;
            pOS08a20Ctx->aecMinGain          = 1.0;
            pOS08a20Ctx->aGain.min           = 1.0;
            pOS08a20Ctx->aGain.max           = 15.5;
            pOS08a20Ctx->aGain.step          = (1.0f/128.0f);
            pOS08a20Ctx->dGain.min           = 1.0;
            pOS08a20Ctx->dGain.max           = 15;
            pOS08a20Ctx->dGain.step          = (1.0f/1024.0f);
            break;

        default:
            TRACE(OS08a20_INFO, "%s:not support sensor mode %d\n", __func__,pOS08a20Ctx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    pOS08a20Ctx->maxFps  = pOS08a20Ctx->sensorMode.fps;
    pOS08a20Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pOS08a20Ctx->currFps = pOS08a20Ctx->maxFps;

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(OS08a20_DEBUG, "%s: OS08a20 System-Reset executed\n", __func__);
    osSleep(100);

    result = OS08a20_AecSetModeParameters(handle, pOS08a20Ctx);
    if (result != RET_SUCCESS) {
        TRACE(OS08a20_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pOS08a20Ctx->configured = BOOL_TRUE;
    TRACE(OS08a20_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT OS08a20_IsiCloseIss(IsiSensorHandle_t handle)
{
    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s (enter)\n", __func__);

    if (pOS08a20Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)OS08a20_IsiSetStreamingIss(pOS08a20Ctx, BOOL_FALSE);

    TRACE(OS08a20_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS08a20_IsiReleaseIss(IsiSensorHandle_t handle)
{
    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s (enter)\n", __func__);

    if (pOS08a20Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pOS08a20Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pOS08a20Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pOS08a20Ctx, 0, sizeof(OS08a20_Context_t));
    free(pOS08a20Ctx);
    TRACE(OS08a20_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS08a20_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;
    uint32_t sensorId = 0;
    uint32_t correctId = 0x5308;

    TRACE(OS08a20_INFO, "%s (enter)\n", __func__);

    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    if (pOS08a20Ctx == NULL || pOS08a20Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = OS08a20_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(OS08a20_ERROR, "%s: Read Sensor ID Error! \n",__func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(OS08a20_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n", __func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(OS08a20_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__, correctId, sensorId);
    TRACE(OS08a20_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS08a20_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(OS08a20_INFO, "%s (enter)\n", __func__);

    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    if (pOS08a20Ctx == NULL || pOS08a20Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (!pValue) return (RET_NULL_POINTER);

    regVal   = 0;
    result    = OS08a20_IsiReadRegIss(handle, 0x300a, &regVal);
    sensorId = (regVal & 0xff) << 8;

    regVal   = 0;
    result   |= OS08a20_IsiReadRegIss(handle, 0x300b, &regVal);
    sensorId |= (regVal & 0xff);

    *pValue = sensorId;
    TRACE(OS08a20_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS08a20_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s (enter)\n", __func__);

    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    if (pOS08a20Ctx == NULL || pOS08a20Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOS08a20Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    result = OS08a20_IsiWriteRegIss(handle, 0x0100, on);
    if (result != RET_SUCCESS) {
        TRACE(OS08a20_ERROR, "%s: set sensor streaming error! \n",__func__);
        return (RET_FAILURE);
    }

    pOS08a20Ctx->streaming = on;

    TRACE(OS08a20_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS08a20_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OS08a20_INFO, "%s: (enter)\n", __func__);

    if (pOS08a20Ctx == NULL) {
        TRACE(OS08a20_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(OS08a20_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    if(pOS08a20Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        /* linear */
        pAeBaseInfo->gain.min        = pOS08a20Ctx->aecMinGain;//total gain
        pAeBaseInfo->gain.max        = pOS08a20Ctx->aecMaxGain;
        pAeBaseInfo->intTime.min     = pOS08a20Ctx->aecMinIntegrationTime;
        pAeBaseInfo->intTime.max     = pOS08a20Ctx->aecMaxIntegrationTime;

        pAeBaseInfo->aGain           = pOS08a20Ctx->aGain;//min, max, step
        pAeBaseInfo->dGain           = pOS08a20Ctx->dGain;

        pAeBaseInfo->aecCurGain      = pOS08a20Ctx->aecCurGain;
        pAeBaseInfo->aecCurIntTime   = pOS08a20Ctx->aecCurIntegrationTime;
    } else {
        /* stitching 2dol */
        pAeBaseInfo->longGain.min        = pOS08a20Ctx->aecMinGain;
        pAeBaseInfo->longGain.max        = pOS08a20Ctx->aecMaxGain;
        pAeBaseInfo->longIntTime.min     = pOS08a20Ctx->aecMinIntegrationTime;
        pAeBaseInfo->longIntTime.max     = pOS08a20Ctx->aecMaxIntegrationTime;

        pAeBaseInfo->shortGain.min       = pOS08a20Ctx->aecMinGain;
        pAeBaseInfo->shortGain.max       = pOS08a20Ctx->aecMaxGain;
        pAeBaseInfo->shortIntTime.min    = pOS08a20Ctx->aecMinVSIntegrationTime;
        pAeBaseInfo->shortIntTime.max    = pOS08a20Ctx->aecMaxVSIntegrationTime;

        pAeBaseInfo->aLongGain           = pOS08a20Ctx->aGain;
        pAeBaseInfo->dLongGain           = pOS08a20Ctx->dGain;
        pAeBaseInfo->aShortGain          = pOS08a20Ctx->aVSGain;
        pAeBaseInfo->dShortGain          = pOS08a20Ctx->dVSGain;

        pAeBaseInfo->curGain.gain[0] = pOS08a20Ctx->aecCurGain;
        pAeBaseInfo->curGain.gain[1] = pOS08a20Ctx->aecCurVSGain;
        pAeBaseInfo->curGain.gain[2] = 0;
        pAeBaseInfo->curGain.gain[3] = 0;
        pAeBaseInfo->curIntTime.intTime[0] = pOS08a20Ctx->aecCurIntegrationTime;
        pAeBaseInfo->curIntTime.intTime[1] = pOS08a20Ctx->aecCurVSIntegrationTime;
        pAeBaseInfo->curIntTime.intTime[2] = 0;
        pAeBaseInfo->curIntTime.intTime[3] = 0;
    }

    pAeBaseInfo->aecGainStep     = pOS08a20Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep  = pOS08a20Ctx->aecIntegrationTimeIncrement;
    pAeBaseInfo->stitchingMode   = pOS08a20Ctx->sensorMode.stitchingMode;

    TRACE(OS08a20_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT OS08a20_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s: (enter)\n", __func__);
    uint32_t again = 0, vsagain = 0;

    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    if (pOS08a20Ctx == NULL || pOS08a20Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOS08a20Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        again = (uint32_t)(pSensorAGain->gain[ISI_LINEAR_PARAS] * 128);
        result = OS08a20_IsiWriteRegIss(handle, 0x3508,(again & 0x3f00)>>8);
        result |= OS08a20_IsiWriteRegIss(handle, 0x3509,(again & 0xff));
        pOS08a20Ctx->curAgain = again/128.0f;

    } else if (pOS08a20Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        again = (uint32_t)(pSensorAGain->gain[ISI_DUAL_EXP_L_PARAS] * 128);
        result = OS08a20_IsiWriteRegIss(handle, 0x3508,(again & 0x3f00)>>8);
        result |= OS08a20_IsiWriteRegIss(handle, 0x3509,(again & 0xff));
        pOS08a20Ctx->curAgain = again/128.0f;

        vsagain = (uint32_t)(pSensorAGain->gain[ISI_DUAL_EXP_S_PARAS] * 128);
        result |=  OS08a20_IsiWriteRegIss(handle, 0x350c,(vsagain & 0x3f00)>>8);
        result |= OS08a20_IsiWriteRegIss(handle, 0x350d,(vsagain & 0x00ff));
        pOS08a20Ctx->curVSAgain = vsagain/128.0f;

    }  else {
        TRACE(OS08a20_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OS08a20_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS08a20_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s: (enter)\n", __func__);
    uint32_t dgain = 0, vsdgain = 0;

    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    if (pOS08a20Ctx == NULL || pOS08a20Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOS08a20Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        dgain = (uint32_t)(pSensorDGain->gain[ISI_LINEAR_PARAS] * 1024);
        result = OS08a20_IsiWriteRegIss(handle, 0x350a, (dgain >> 8) & 0x3f);
        result |= OS08a20_IsiWriteRegIss(handle,0x350b, (dgain & 0xff));
        pOS08a20Ctx->curDgain = dgain/1024.0f;
        pOS08a20Ctx->aecCurGain = pOS08a20Ctx->curAgain * pOS08a20Ctx->curDgain;

    } else if (pOS08a20Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        dgain = (uint32_t)(pSensorDGain->gain[ISI_DUAL_EXP_L_PARAS] * 1024);
        result = OS08a20_IsiWriteRegIss(handle, 0x350a, (dgain >> 8) & 0x3f);
        result |= OS08a20_IsiWriteRegIss(handle,0x350b, (dgain & 0xff));
        pOS08a20Ctx->curDgain = dgain/1024.0f;
        pOS08a20Ctx->aecCurGain = pOS08a20Ctx->curAgain * pOS08a20Ctx->curDgain;

        vsdgain = (uint32_t)(pSensorDGain->gain[ISI_DUAL_EXP_S_PARAS] * 1024);
        result |=  OS08a20_IsiWriteRegIss(handle, 0x350e, (vsdgain >> 8) & 0x3f);
        result |=  OS08a20_IsiWriteRegIss(handle, 0x350f, (vsdgain & 0xff));
        pOS08a20Ctx->curVSDgain = vsdgain/1024.0f;
        pOS08a20Ctx->aecCurVSGain = pOS08a20Ctx->curVSAgain * pOS08a20Ctx->curVSDgain;

    }  else {
        TRACE(OS08a20_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OS08a20_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS08a20_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s: (enter)\n", __func__);

    if (pOS08a20Ctx == NULL) {
        TRACE(OS08a20_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    if(pOS08a20Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        pSensorAGain->gain[ISI_LINEAR_PARAS]       = pOS08a20Ctx->curAgain;

    } else if (pOS08a20Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        pSensorAGain->gain[ISI_DUAL_EXP_L_PARAS]   = pOS08a20Ctx->curAgain;
        pSensorAGain->gain[ISI_DUAL_EXP_S_PARAS]   = pOS08a20Ctx->curVSAgain;

    } else {
        TRACE(OS08a20_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OS08a20_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS08a20_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s: (enter)\n", __func__);

    if (pOS08a20Ctx == NULL) {
        TRACE(OS08a20_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    if (pOS08a20Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        pSensorDGain->gain[ISI_LINEAR_PARAS] = pOS08a20Ctx->curDgain;

    } else if (pOS08a20Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        pSensorDGain->gain[ISI_DUAL_EXP_L_PARAS]  = pOS08a20Ctx->curDgain;
        pSensorDGain->gain[ISI_DUAL_EXP_S_PARAS]  = pOS08a20Ctx->curVSDgain;

    } else {
        TRACE(OS08a20_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OS08a20_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS08a20_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s: (enter)\n", __func__);
    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;

    if (!pOS08a20Ctx) {
        TRACE(OS08a20_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOS08a20Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        result = OS08a20_SetIntTime(handle, pSensorIntTime->intTime[ISI_LINEAR_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(OS08a20_INFO, "%s: set sensor IntTime[ISI_LINEAR_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

    } else if (pOS08a20Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        result = OS08a20_SetIntTime(handle, pSensorIntTime->intTime[ISI_DUAL_EXP_L_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(OS08a20_INFO, "%s: set sensor IntTime[ISI_DUAL_EXP_L_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

        result = OS08a20_SetVSIntTime(handle, pSensorIntTime->intTime[ISI_DUAL_EXP_S_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(OS08a20_INFO, "%s: set sensor IntTime[ISI_DUAL_EXP_S_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

    }  else {
        TRACE(OS08a20_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OS08a20_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OS08a20_SetIntTime(IsiSensorHandle_t handle, float newIntegrationTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s: (enter)\n", __func__);
    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pOS08a20Ctx) {
        TRACE(OS08a20_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = newIntegrationTime / pOS08a20Ctx->oneLineExpTime;
    expLine = MIN(pOS08a20Ctx->maxIntegrationLine, MAX(pOS08a20Ctx->minIntegrationLine, expLine));
    TRACE(OS08a20_DEBUG, "%s: set expLine = 0x%04x\n", __func__, expLine);

    result =  OS08a20_IsiWriteRegIss(handle, 0x3501, (expLine >>8) & 0xff);
    result |= OS08a20_IsiWriteRegIss(handle, 0x3502, (expLine & 0xff));

    pOS08a20Ctx->aecCurIntegrationTime = expLine * pOS08a20Ctx->oneLineExpTime;

    TRACE(OS08a20_DEBUG, "%s: set IntTime = %f\n", __func__, pOS08a20Ctx->aecCurIntegrationTime);
    TRACE(OS08a20_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OS08a20_SetVSIntTime(IsiSensorHandle_t handle, float newIntegrationTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s: (enter)\n", __func__);
    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pOS08a20Ctx) {
        TRACE(OS08a20_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = newIntegrationTime / pOS08a20Ctx->oneLineExpTime;
#ifdef ISP_MI_HDR
    expLine = MIN(550/2, MAX(8, expLine)); // 40ms
#else
    expLine = MIN(51, MAX(8, expLine));   //61
#endif

    result =  OS08a20_IsiWriteRegIss(handle, 0x3511, (expLine >> 8) & 0xff);
    result |= OS08a20_IsiWriteRegIss(handle, 0x3512, (expLine & 0xff));

    pOS08a20Ctx->aecCurVSIntegrationTime = expLine * pOS08a20Ctx->oneLineExpTime;

    TRACE(OS08a20_DEBUG, "%s: set VSIntTime = %f\n", __func__, pOS08a20Ctx->aecCurVSIntegrationTime);
    TRACE(OS08a20_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS08a20_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s: (enter)\n", __func__);

    if (!pOS08a20Ctx) {
        TRACE(OS08a20_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    if (pOS08a20Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        pSensorIntTime->intTime[ISI_LINEAR_PARAS] = pOS08a20Ctx->aecCurIntegrationTime;

    } else if (pOS08a20Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        pSensorIntTime->intTime[ISI_DUAL_EXP_L_PARAS]   = pOS08a20Ctx->aecCurIntegrationTime;
        pSensorIntTime->intTime[ISI_DUAL_EXP_S_PARAS]   = pOS08a20Ctx->aecCurVSIntegrationTime;

    } else {
        TRACE(OS08a20_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OS08a20_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS08a20_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t * pFps)
{
    const OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s: (enter)\n", __func__);

    if (pOS08a20Ctx == NULL) {
        TRACE(OS08a20_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pOS08a20Ctx->currFps;

    TRACE(OS08a20_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS08a20_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    RESULT result = RET_SUCCESS;
    int32_t NewVts = 0;

    TRACE(OS08a20_INFO, "%s: (enter)\n", __func__);

    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    if (pOS08a20Ctx == NULL) {
        TRACE(OS08a20_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

   if (fps > pOS08a20Ctx->maxFps) {
        TRACE(OS08a20_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, fps, pOS08a20Ctx->maxFps, pOS08a20Ctx->minFps,pOS08a20Ctx->maxFps);
        fps = pOS08a20Ctx->maxFps;
    }
    if (fps < pOS08a20Ctx->minFps) {
        TRACE(OS08a20_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, fps, pOS08a20Ctx->minFps, pOS08a20Ctx->minFps,pOS08a20Ctx->maxFps);
        fps = pOS08a20Ctx->minFps;
    }
     
    NewVts = pOS08a20Ctx->frameLengthLines*pOS08a20Ctx->sensorMode.fps / fps;
    result =  OS08a20_IsiWriteRegIss(handle, 0x380e, NewVts >> 8);
    result |=  OS08a20_IsiWriteRegIss(handle, 0x380f, NewVts & 0xff);
    pOS08a20Ctx->currFps              = fps;
    pOS08a20Ctx->curFrameLengthLines  = NewVts;
    pOS08a20Ctx->maxIntegrationLine   = pOS08a20Ctx->curFrameLengthLines - 8;
    //pOS08a20Ctx->aecMaxIntegrationTime = pOS08a20Ctx->maxIntegrationLine * pOS08a20Ctx->oneLineExpTime;

    TRACE(OS08a20_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS08a20_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    const OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    if (pOS08a20Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(OS08a20_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    pIspStatus->useSensorAWB = false;

    TRACE(OS08a20_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT OS08a20_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS08a20_INFO, "%s: (enter)\n", __func__);

    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    if (pOS08a20Ctx == NULL || pOS08a20Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOS08a20Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = OS08a20_IsiWriteRegIss(handle, 0x5081, 0x00);
    } else {
        result = OS08a20_IsiWriteRegIss(handle, 0x5081, 0x80);
    }

    pOS08a20Ctx->testPattern = tpg.enable;

    TRACE(OS08a20_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS08a20_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(OS08a20_INFO, "%s: (enter)\n", __func__);

    OS08a20_Context_t *pOS08a20Ctx = (OS08a20_Context_t *) handle;
    if (pOS08a20Ctx == NULL || pOS08a20Ctx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOS08a20Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!OS08a20_IsiReadRegIss(handle, 0x5081,&value)) {
        pTpg->enable = ((value & 0x80) != 0) ? 1 : 0;
        if(pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pOS08a20Ctx->testPattern = pTpg->enable;
    }

    TRACE(OS08a20_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS08a20_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "OS08a20";
    TRACE(OS08a20_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = OS08a20_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = OS08a20_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = OS08a20_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = OS08a20_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = OS08a20_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = OS08a20_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = OS08a20_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = OS08a20_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = OS08a20_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = OS08a20_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = OS08a20_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = OS08a20_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = OS08a20_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = OS08a20_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = OS08a20_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = OS08a20_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = OS08a20_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = OS08a20_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = OS08a20_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = OS08a20_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = OS08a20_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = OS08a20_IsiGetIspStatusIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = OS08a20_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = OS08a20_IsiGetTpgIss;

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

    TRACE(OS08a20_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t OS08a20_IsiCamDrvConfig = {
    .cameraDriverID      = 0x5308,
    .pIsiGetSensorIss    = OS08a20_IsiGetSensorIss,
};
