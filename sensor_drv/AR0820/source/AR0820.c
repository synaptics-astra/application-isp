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
#include "AR0820_priv.h"

CREATE_TRACER(AR0820_INFO , "AR0820: ", INFO,    1);
CREATE_TRACER(AR0820_WARN , "AR0820: ", WARNING, 1);
CREATE_TRACER(AR0820_ERROR, "AR0820: ", ERROR,   1);
CREATE_TRACER(AR0820_DEBUG,     "AR0820: ", INFO, 1);
CREATE_TRACER(AR0820_REG_INFO , "AR0820: ", INFO, 1);
CREATE_TRACER(AR0820_REG_DEBUG, "AR0820: ", INFO, 1);

#define AR0820_MIN_GAIN_STEP    ( 1.0f/256.0f )  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain ) */
#define AR0820_METADATA_ONELINE_PIXEL 1949
/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t par0820_mode_info[] = {
    {
        .index     = 0,//1080p linear
        .size      = {
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
        .fps       = 15 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 12,
        .bayerPattern = ISI_BPAT_GRBG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 1,//4k linear
        .size      = {
            .boundsWidth  = 3840,
            .boundsHeight = 2160,
            .top           = 0,
            .left          = 0,
            .width         = 3840,
            .height        = 2160,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps       = 6.7 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 12,
        .bayerPattern = ISI_BPAT_GRBG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 2,//1080p(binning) native 3dol
        .size      = {
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
        .fps       = 7 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_HDR_NATIVE,
        .nativeMode = ISI_SENSOR_NATIVE_3DOL,
        .bitWidth = 12,
        .compress.enable = 1,
        .compress.xBit  = 20,
        .compress.yBit  = 12,
        .bayerPattern = ISI_BPAT_GRBG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 3,//1080p(crop) native 3dol
        .size      = {
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
        .fps       = 7.6 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_HDR_NATIVE,
        .nativeMode = ISI_SENSOR_NATIVE_3DOL,
        .bitWidth = 12,
        .compress.enable = 1,
        .compress.xBit  = 20,
        .compress.yBit  = 12,
        .bayerPattern = ISI_BPAT_GRBG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 4,//4k native 3dol
        .size      = {
            .boundsWidth  = 3840,
            .boundsHeight = 2160,
            .top           = 0,
            .left          = 0,
            .width         = 3840,
            .height        = 2160,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps       = 6.7 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_HDR_NATIVE,
        .nativeMode = ISI_SENSOR_NATIVE_3DOL,
        .bitWidth = 12,
        .compress.enable = 1,
        .compress.xBit  = 20,
        .compress.yBit  = 12,
        .bayerPattern = ISI_BPAT_GRBG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 5,//1080p native 4dol
        .size      = {
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
        .fps       = 5.4 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_HDR_NATIVE,
        .nativeMode = ISI_SENSOR_NATIVE_4DOL,
        .bitWidth = 12,
        .compress.enable = 1,
        .compress.xBit  = 24,
        .compress.yBit  = 12,
        .bayerPattern = ISI_BPAT_GRBG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 6,//1080p LIM 4dol
        .size      = {
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
        .fps       = 5 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_HDR_STITCH,
        .stitchingMode = ISI_SENSOR_STITCHING_4DOL,
        .bitWidth = 12,
        .bayerPattern = ISI_BPAT_GRBG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 7,//1080p native4dol binning demo mode
        .size      = {
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
        .fps       = 17.2 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_HDR_NATIVE,
        .nativeMode = ISI_SENSOR_NATIVE_4DOL,
        .bitWidth = 12,
        .compress.enable = 1,
        .compress.xBit  = 24,
        .compress.yBit  = 12,
        .bayerPattern = ISI_BPAT_GRBG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 8,//4K native4dol VI200 metadata mode
        .size      = {
            .boundsWidth  = 3840,
            .boundsHeight = 2160,
            .top           = 0,
            .left          = 0,
            .width         = 3840,
            .height        = 2160,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps       = 5.7 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_HDR_NATIVE,
        .nativeMode = ISI_SENSOR_NATIVE_4DOL,
        .bitWidth = 12,
        .compress.enable = 1,
        .compress.xBit  = 24,
        .compress.yBit  = 12,
        .bayerPattern = ISI_BPAT_GRBG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
};

const IsiMetadataAttr_t AR0820_Metadata_Attr = {

    .subAttr = {
       .support  = 1,
       .regInfo  = 1,
       .expTime  = 1,
       .again    = 1,
       .dgain    = 1,
       .bls      = 0,
       .hist     = 0,
       .meanLuma = 0,
       .reservedEnable = 0
    }
};


static RESULT AR0820_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s (enter)\n", __func__);

    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL || pAR0820Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pAR0820Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(AR0820_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }
    TRACE(AR0820_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT AR0820_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s (enter)\n", __func__);

    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL || pAR0820Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pAR0820Ctx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(AR0820_ERROR, "%s: hal write sensor register error!\n",__func__);
        return (RET_FAILURE);
    }

    TRACE(AR0820_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT AR0820_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(AR0820_INFO, "%s (enter)\n", __func__);
    const AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pAR0820Ctx->sensorMode), sizeof(pAR0820Ctx->sensorMode));

    TRACE(AR0820_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT AR0820_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(AR0820_INFO, "%s (enter)\n", __func__);
    const AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(par0820_mode_info)/sizeof(par0820_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(par0820_mode_info)/sizeof(par0820_mode_info[0])); i++) {
        if (par0820_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &par0820_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(AR0820_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT AR0820_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps) {
    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(AR0820_INFO, "%s (enter)\n", __func__);

    if (pAR0820Ctx == NULL) return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pAR0820Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pAR0820Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pAR0820Ctx->sensorMode.size.width;
    pCaps->resolution.height = pAR0820Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_2LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(AR0820_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT AR0820_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle) 
{
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s (enter)\n", __func__);

    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) malloc(sizeof(AR0820_Context_t));
    if (!pAR0820Ctx) {
        TRACE(AR0820_ERROR, "%s: Can't allocate ar0820 context\n", __func__);
        return (RET_OUTOFMEM);
    }
    MEMSET(pAR0820Ctx, 0, sizeof(AR0820_Context_t));

    pAR0820Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pAR0820Ctx->groupHold          = BOOL_FALSE;
    pAR0820Ctx->oldGain            = 0;
    pAR0820Ctx->oldIntegrationTime = 0;
    pAR0820Ctx->configured         = BOOL_FALSE;
    pAR0820Ctx->streaming          = BOOL_FALSE;
    pAR0820Ctx->testPattern        = BOOL_FALSE;
    pAR0820Ctx->isAfpsRun          = BOOL_FALSE;
    pAR0820Ctx->sensorMode.index   = 0;
    pAR0820Ctx->metaEnable.mainAttr  = 0;

    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x20;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 2;

    pAR0820Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pAR0820Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(AR0820_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pAR0820Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pAR0820Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pAR0820Ctx;

    TRACE(AR0820_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT AR0820_AecSetModeParameters(IsiSensorHandle_t handle, AR0820_Context_t * pAR0820Ctx) 
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0, cgain = 0, fgain = 0;
    TRACE(AR0820_INFO, "%s%s: (enter)\n", __func__, pAR0820Ctx->isAfpsRun ? "(AFPS)" : "");

    pAR0820Ctx->aecIntegrationTimeIncrement = pAR0820Ctx->oneLineExpTime;
    pAR0820Ctx->aecMinIntegrationTime = pAR0820Ctx->oneLineExpTime * pAR0820Ctx->minIntegrationLine;
    //pAR0820Ctx->aecMaxIntegrationTime = pAR0820Ctx->oneLineExpTime * pAR0820Ctx->maxIntegrationLine;
    pAR0820Ctx->aecMaxIntegrationTime = 0.04;

    TRACE(AR0820_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pAR0820Ctx->isAfpsRun ? "(AFPS)" : "", pAR0820Ctx->aecMaxIntegrationTime);

    pAR0820Ctx->aecGainIncrement = AR0820_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pAR0820Ctx->oldGain = 0;
    pAR0820Ctx->oldIntegrationTime = 0;

    AR0820_IsiReadRegIss(handle, 0x3366, &value);
    cgain = value & 0x07;
    AR0820_IsiReadRegIss(handle, 0x336a, &value);
    fgain = value & 0x0f;
    pAR0820Ctx->aecCurGain = (float)((1 << cgain) * (1 + (float)(fgain/16)));
    pAR0820Ctx->aecCurLongGain = (float)((1 << cgain) * (1 + (float)(fgain/16)));

    AR0820_IsiReadRegIss(handle, 0x3012, &value);
    pAR0820Ctx->aecCurIntegrationTime = value * pAR0820Ctx->oneLineExpTime;
    pAR0820Ctx->aecCurLongIntegrationTime = value * pAR0820Ctx->oneLineExpTime;

    TRACE(AR0820_INFO, "%s%s: (exit)\n", __func__, pAR0820Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT AR0820_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode) 
{
    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(AR0820_INFO, "%s (enter)\n", __func__);

    if (!pAR0820Ctx) {
        TRACE(AR0820_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAR0820Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pAR0820Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(par0820_mode_info) / sizeof(IsiSensorMode_t); i++) {
        if (par0820_mode_info[i].index == pAR0820Ctx->sensorMode.index) {
            SensorDefaultMode = &(par0820_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(AR0820_mipi2lane_1080p_linear_init) / sizeof(AR0820_mipi2lane_1080p_linear_init[0]); i++) {
                    AR0820_IsiWriteRegIss(handle, AR0820_mipi2lane_1080p_linear_init[i][0], AR0820_mipi2lane_1080p_linear_init[i][1]);
                }
                break;
            case 1:
                for (int i = 0; i<sizeof(AR0820_mipi2lane_4k_linear_init) / sizeof(AR0820_mipi2lane_4k_linear_init[0]); i++) {
                    AR0820_IsiWriteRegIss(handle, AR0820_mipi2lane_4k_linear_init[i][0], AR0820_mipi2lane_4k_linear_init[i][1]);
                }
                break;
            case 2: 
                for (int i = 0; i<sizeof(AR0820_mipi2lane_1080p_native3dol_binning_init) / sizeof(AR0820_mipi2lane_1080p_native3dol_binning_init[0]); i++) {
                    AR0820_IsiWriteRegIss(handle, AR0820_mipi2lane_1080p_native3dol_binning_init[i][0], AR0820_mipi2lane_1080p_native3dol_binning_init[i][1]);
                }
                break;
            case 3: 
                for (int i = 0; i<sizeof(AR0820_mipi2lane_1080p_native3dol_crop_init) / sizeof(AR0820_mipi2lane_1080p_native3dol_crop_init[0]); i++) {
                    AR0820_IsiWriteRegIss(handle, AR0820_mipi2lane_1080p_native3dol_crop_init[i][0], AR0820_mipi2lane_1080p_native3dol_crop_init[i][1]);
                }
                break;
            case 4: 
                for (int i = 0; i<sizeof(AR0820_mipi2lane_4k_native3dol_init) / sizeof(AR0820_mipi2lane_4k_native3dol_init[0]); i++) {
                    AR0820_IsiWriteRegIss(handle, AR0820_mipi2lane_4k_native3dol_init[i][0], AR0820_mipi2lane_4k_native3dol_init[i][1]);
                }
                break;
            case 5: 
                for (int i = 0; i<sizeof(AR0820_mipi2lane_1080p_native4dol_init) / sizeof(AR0820_mipi2lane_1080p_native4dol_init[0]); i++) {
                    AR0820_IsiWriteRegIss(handle, AR0820_mipi2lane_1080p_native4dol_init[i][0], AR0820_mipi2lane_1080p_native4dol_init[i][1]);
                }
                break;
            case 6: 
                for (int i = 0; i<sizeof(AR0820_mipi2lane_1080p_lim4dol_init) / sizeof(AR0820_mipi2lane_1080p_lim4dol_init[0]); i++) {
                    AR0820_IsiWriteRegIss(handle, AR0820_mipi2lane_1080p_lim4dol_init[i][0], AR0820_mipi2lane_1080p_lim4dol_init[i][1]);
                }
                break;
            case 7: 
                for (int i = 0; i<sizeof(AR0820_mipi2lane_1080p_native4dol_demo_init) / sizeof(AR0820_mipi2lane_1080p_native4dol_demo_init[0]); i++) {
                    AR0820_IsiWriteRegIss(handle, AR0820_mipi2lane_1080p_native4dol_demo_init[i][0], AR0820_mipi2lane_1080p_native4dol_demo_init[i][1]);
                }
                break;
            case 8: 
                for (int i = 0; i<sizeof(AR0820_mipi2lane_4K_native4dol_VI200_metadata_init) / sizeof(AR0820_mipi2lane_4K_native4dol_VI200_metadata_init[0]); i++) {
                    AR0820_IsiWriteRegIss(handle, AR0820_mipi2lane_4K_native4dol_VI200_metadata_init[i][0], AR0820_mipi2lane_4K_native4dol_VI200_metadata_init[i][1]);
                }
                break;
            default:
                TRACE(AR0820_INFO, "%s:not support sensor mode %d\n", __func__, pAR0820Ctx->sensorMode.index);
                return RET_NOTSUPP;
                break;
        }

        memcpy(&(pAR0820Ctx->sensorMode), SensorDefaultMode, sizeof(IsiSensorMode_t));
    } else {
        TRACE(AR0820_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pAR0820Ctx->sensorMode.index)
    {
        case 0://1080p linear
            pAR0820Ctx->oneLineExpTime   = 0.000029;
            pAR0820Ctx->frameLengthLines    = 0x0910;
            pAR0820Ctx->curFrameLengthLines = pAR0820Ctx->frameLengthLines;
            pAR0820Ctx->maxIntegrationLine  = pAR0820Ctx->curFrameLengthLines - 3;
            pAR0820Ctx->minIntegrationLine  = 1;
            pAR0820Ctx->aecMaxGain          = 8.0;
            pAR0820Ctx->aecMinGain          = 1.0;
            pAR0820Ctx->aGain.min           = 1.0;
            pAR0820Ctx->aGain.max           = 8.0;
            pAR0820Ctx->aGain.step          = (1.0f/256.0f);
            pAR0820Ctx->dGain.min           = 1.0;
            pAR0820Ctx->dGain.max           = 1.0;
            pAR0820Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 1://4k linear
            pAR0820Ctx->oneLineExpTime   = 0.000064;
            pAR0820Ctx->frameLengthLines    = 0x0920;
            pAR0820Ctx->curFrameLengthLines = pAR0820Ctx->frameLengthLines;
            pAR0820Ctx->maxIntegrationLine  = pAR0820Ctx->curFrameLengthLines - 3;
            pAR0820Ctx->minIntegrationLine  = 1;
            pAR0820Ctx->aecMaxGain          = 8.0;
            pAR0820Ctx->aecMinGain          = 1.0;
            pAR0820Ctx->aGain.min           = 1.0;
            pAR0820Ctx->aGain.max           = 8.0;
            pAR0820Ctx->aGain.step          = (1.0f/256.0f);
            pAR0820Ctx->dGain.min           = 1.0;
            pAR0820Ctx->dGain.max           = 1.0;
            pAR0820Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 2://1080p(binning) native 3dol
            pAR0820Ctx->oneLineExpTime   = 0.000045;
            pAR0820Ctx->frameLengthLines    = 0x0c18;
            pAR0820Ctx->curFrameLengthLines = pAR0820Ctx->frameLengthLines;
            pAR0820Ctx->maxIntegrationLine  = pAR0820Ctx->curFrameLengthLines - 3;
            pAR0820Ctx->minIntegrationLine  = 1;
            pAR0820Ctx->aecMaxGain          = 8.0;
            pAR0820Ctx->aecMinGain          = 1.0;
            pAR0820Ctx->aGain.min           = 1.0;
            pAR0820Ctx->aGain.max           = 8.0;
            pAR0820Ctx->aGain.step          = (1.0f/256.0f);
            pAR0820Ctx->dGain.min           = 1.0;
            pAR0820Ctx->dGain.max           = 1.0;
            pAR0820Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 3://1080p(crop) native 3dol
            pAR0820Ctx->oneLineExpTime   = 0.000057;
            pAR0820Ctx->frameLengthLines    = 0x0920;
            pAR0820Ctx->curFrameLengthLines = pAR0820Ctx->frameLengthLines;
            pAR0820Ctx->maxIntegrationLine  = pAR0820Ctx->curFrameLengthLines - 3;
            pAR0820Ctx->minIntegrationLine  = 1;
            pAR0820Ctx->aecMaxGain          = 8.0;
            pAR0820Ctx->aecMinGain          = 1.0;
            pAR0820Ctx->aGain.min           = 1.0;
            pAR0820Ctx->aGain.max           = 8.0;
            pAR0820Ctx->aGain.step          = (1.0f/256.0f);
            pAR0820Ctx->dGain.min           = 1.0;
            pAR0820Ctx->dGain.max           = 1.0;
            pAR0820Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 4://4k native 3dol
            pAR0820Ctx->oneLineExpTime   = 0.000064;
            pAR0820Ctx->frameLengthLines    = 0x0920;
            pAR0820Ctx->curFrameLengthLines = pAR0820Ctx->frameLengthLines;
            pAR0820Ctx->maxIntegrationLine  = pAR0820Ctx->curFrameLengthLines - 3;
            pAR0820Ctx->minIntegrationLine  = 1;
            pAR0820Ctx->aecMaxGain          = 8.0;
            pAR0820Ctx->aecMinGain          = 1.0;
            pAR0820Ctx->aGain.min           = 1.0;
            pAR0820Ctx->aGain.max           = 8.0;
            pAR0820Ctx->aGain.step          = (1.0f/256.0f);
            pAR0820Ctx->dGain.min           = 1.0;
            pAR0820Ctx->dGain.max           = 1.0;
            pAR0820Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 5://1080p native 4dol
            pAR0820Ctx->oneLineExpTime   = 0.000059;
            pAR0820Ctx->frameLengthLines    = 0x0c18;
            pAR0820Ctx->curFrameLengthLines = pAR0820Ctx->frameLengthLines;
            pAR0820Ctx->maxIntegrationLine  = pAR0820Ctx->curFrameLengthLines - 3;
            pAR0820Ctx->minIntegrationLine  = 1;
            pAR0820Ctx->aecMaxGain          = 8.0;
            pAR0820Ctx->aecMinGain          = 1.0;
            pAR0820Ctx->aGain.min           = 1.0;
            pAR0820Ctx->aGain.max           = 8.0;
            pAR0820Ctx->aGain.step          = (1.0f/256.0f);
            pAR0820Ctx->dGain.min           = 1.0;
            pAR0820Ctx->dGain.max           = 1.0;
            pAR0820Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 6://1080p LIM 4dol
            pAR0820Ctx->oneLineExpTime   = 0.000110;
            pAR0820Ctx->frameLengthLines    = 0x06e8;
            pAR0820Ctx->curFrameLengthLines = pAR0820Ctx->frameLengthLines;
            pAR0820Ctx->maxIntegrationLine  = pAR0820Ctx->curFrameLengthLines - 3;
            pAR0820Ctx->minIntegrationLine  = 1;
            pAR0820Ctx->aecMaxGain          = 8.0;
            pAR0820Ctx->aecMinGain          = 1.0;
            pAR0820Ctx->aGain.min           = 1.0;
            pAR0820Ctx->aGain.max           = 8.0;
            pAR0820Ctx->aGain.step          = (1.0f/256.0f);
            pAR0820Ctx->dGain.min           = 1.0;
            pAR0820Ctx->dGain.max           = 1.0;
            pAR0820Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 7://1080p native4dol binning demo mode
            pAR0820Ctx->oneLineExpTime   = 0.000026;
            pAR0820Ctx->frameLengthLines    = 0x08ba;//0x0a18
            pAR0820Ctx->curFrameLengthLines = pAR0820Ctx->frameLengthLines;
            pAR0820Ctx->maxIntegrationLine  = pAR0820Ctx->curFrameLengthLines - 3;
            pAR0820Ctx->minIntegrationLine  = 1;
            pAR0820Ctx->aecMaxGain          = 8.0;
            pAR0820Ctx->aecMinGain          = 1.0;
            pAR0820Ctx->aGain.min           = 1.0;
            pAR0820Ctx->aGain.max           = 8.0;
            pAR0820Ctx->aGain.step          = (1.0f/256.0f);
            pAR0820Ctx->dGain.min           = 1.0;
            pAR0820Ctx->dGain.max           = 1.0;
            pAR0820Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 8://4K native4dol VI200 metadata mode
            pAR0820Ctx->oneLineExpTime   = 0.000075;
            pAR0820Ctx->frameLengthLines    = 0x0920;
            pAR0820Ctx->curFrameLengthLines = pAR0820Ctx->frameLengthLines;
            pAR0820Ctx->maxIntegrationLine  = pAR0820Ctx->curFrameLengthLines - 3;
            pAR0820Ctx->minIntegrationLine  = 1;
            pAR0820Ctx->aecMaxGain          = 8.0;
            pAR0820Ctx->aecMinGain          = 1.0;
            pAR0820Ctx->aGain.min           = 1.0;
            pAR0820Ctx->aGain.max           = 8.0;
            pAR0820Ctx->aGain.step          = (1.0f/256.0f);
            pAR0820Ctx->dGain.min           = 1.0;
            pAR0820Ctx->dGain.max           = 1.0;
            pAR0820Ctx->dGain.step          = (1.0f/256.0f);
            break;
        default:
            TRACE(AR0820_INFO, "%s:not support sensor mode %d\n", __func__, pAR0820Ctx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    pAR0820Ctx->maxFps  = pAR0820Ctx->sensorMode.fps;
    pAR0820Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pAR0820Ctx->currFps = pAR0820Ctx->maxFps;

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(AR0820_DEBUG, "%s: AR0820 System-Reset executed\n", __func__);
    osSleep(100);

    result = AR0820_AecSetModeParameters(handle, pAR0820Ctx);
    if (result != RET_SUCCESS) {
        TRACE(AR0820_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pAR0820Ctx->configured = BOOL_TRUE;
    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT AR0820_IsiCloseIss(IsiSensorHandle_t handle)
{
    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s (enter)\n", __func__);

    if (pAR0820Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)AR0820_IsiSetStreamingIss(pAR0820Ctx, BOOL_FALSE);

    TRACE(AR0820_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT AR0820_IsiReleaseIss(IsiSensorHandle_t handle) 
{
    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s (enter)\n", __func__);

    if (pAR0820Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pAR0820Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pAR0820Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pAR0820Ctx, 0, sizeof(AR0820_Context_t));
    free(pAR0820Ctx);

    TRACE(AR0820_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT AR0820_IsiCheckConnectionIss(IsiSensorHandle_t handle) 
{
    RESULT result = RET_SUCCESS;
    uint32_t correctId = 0x0557;
    uint32_t sensorId = 0;

    TRACE(AR0820_INFO, "%s (enter)\n", __func__);

    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL || pAR0820Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = AR0820_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(AR0820_ERROR, "%s: Read Sensor ID Error! \n", __func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(AR0820_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n", __func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(AR0820_INFO, "%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__, correctId, sensorId);
    TRACE(AR0820_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT AR0820_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue) 
{
    RESULT result = RET_SUCCESS;
    uint16_t sensorId;
    TRACE(AR0820_INFO, "%s (enter)\n", __func__);

    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL || pAR0820Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (!pValue)
        return (RET_NULL_POINTER);

    result = AR0820_IsiReadRegIss(handle, 0x3000, &sensorId);

    *pValue = sensorId;
    TRACE(AR0820_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT AR0820_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on) 
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(AR0820_INFO, "%s (enter)\n", __func__);

    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL || pAR0820Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pAR0820Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    if(on){
        result = AR0820_IsiReadRegIss(handle, 0x301a, &value);
        if (result != RET_SUCCESS) {
            return (RET_FAILURE);
        }

        result = AR0820_IsiWriteRegIss(handle, 0x301a, (value|0x04));
        if (result != RET_SUCCESS) {
            return (RET_FAILURE);
        }
    } else {
        result = AR0820_IsiReadRegIss(handle, 0x301a, &value);
        if (result != RET_SUCCESS) {
            return (RET_FAILURE);
        }

        result = AR0820_IsiWriteRegIss(handle, 0x301a, (value&0xfb));
        if (result != RET_SUCCESS) {
            return (RET_FAILURE);
        }

        result = AR0820_IsiWriteRegIss(handle, 0x3020, 0x01);//software reset
        if (result != RET_SUCCESS) {
            return (RET_FAILURE);
        }
    }

    pAR0820Ctx->streaming = on;

    TRACE(AR0820_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT AR0820_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    if (pAR0820Ctx == NULL) {
        TRACE(AR0820_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(AR0820_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    //get time limit and total gain limit
    //Linear mode
    pAeBaseInfo->gain.min        = pAR0820Ctx->aecMinGain;
    pAeBaseInfo->gain.max        = pAR0820Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pAR0820Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pAR0820Ctx->aecMaxIntegrationTime;
    pAeBaseInfo->aecCurGain     = pAR0820Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime  = pAR0820Ctx->aecCurIntegrationTime;

    //get again/dgain info
    pAeBaseInfo->aGain           = pAR0820Ctx->aGain;
    pAeBaseInfo->dGain           = pAR0820Ctx->dGain;
    
    //native 3dol
    pAeBaseInfo->longGain.min        = pAR0820Ctx->aecMinGain;
    pAeBaseInfo->longGain.max        = pAR0820Ctx->aecMaxGain;
    pAeBaseInfo->longIntTime.min     = pAR0820Ctx->aecMinIntegrationTime;
    pAeBaseInfo->longIntTime.max     = pAR0820Ctx->aecMaxIntegrationTime;

    pAeBaseInfo->curGain.gain[0]       = pAR0820Ctx->aecCurLongGain;
    pAeBaseInfo->curGain.gain[1]       = 0;
    pAeBaseInfo->curGain.gain[2]       = 0;
    pAeBaseInfo->curGain.gain[3]       = 0;
    pAeBaseInfo->curIntTime.intTime[0] = pAR0820Ctx->aecCurLongIntegrationTime;
    pAeBaseInfo->curIntTime.intTime[1] = 0;
    pAeBaseInfo->curIntTime.intTime[2] = 0;
    pAeBaseInfo->curIntTime.intTime[3] = 0;

    pAeBaseInfo->aLongGain             = pAR0820Ctx->aGain;
    pAeBaseInfo->dLongGain             = pAR0820Ctx->dGain;
    pAeBaseInfo->aShortGain            = pAR0820Ctx->aGain;
    pAeBaseInfo->dShortGain            = pAR0820Ctx->dGain;
    pAeBaseInfo->aVSGain               = pAR0820Ctx->aVSGain;
    pAeBaseInfo->dVSGain               = pAR0820Ctx->dVSGain;
    
    pAeBaseInfo->nativeHdrRatio[0] = 16.0;
    pAeBaseInfo->nativeHdrRatio[1] = 16.0;
    pAeBaseInfo->nativeHdrRatio[2] = 16.0;
    
    //Linear and hdr mode common parameters
    pAeBaseInfo->aecGainStep    = pAR0820Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep = pAR0820Ctx->aecIntegrationTimeIncrement;
    pAeBaseInfo->stitchingMode  = pAR0820Ctx->sensorMode.stitchingMode;
    pAeBaseInfo->nativeMode     = pAR0820Ctx->sensorMode.nativeMode;

    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT AR0820_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);
    float NewGain = 0;

    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL || pAR0820Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    NewGain = pSensorAGain->gain[ISI_LINEAR_PARAS];
    NewGain = MAX(pAR0820Ctx->aecMinGain, MIN(NewGain, pAR0820Ctx->aecMaxGain));

    uint8_t  cgain = 2;
    uint8_t  fgain_div = 16;
    uint16_t cgain_pow = 0;
    uint16_t fgain = 0;
    float SensorGain = 0;

    /*step1. set analog coarse gain*/
    if (NewGain < 2) {
        cgain_pow = 0;
        SensorGain = NewGain;
    } else {
        for (cgain_pow=1; cgain_pow<4; cgain_pow++) {
            if (NewGain/cgain >= 2) {
                cgain *= 2;
                continue;
            } else {
                break;
            }
        }
    }

    /*step2. set analog fine gain*/
    if (cgain_pow) {
        SensorGain = NewGain/cgain; //SensorGain /= cgain;
    }
    if (SensorGain >= 1) {
        SensorGain -= 1;
    }
    fgain = (uint8_t)(SensorGain * fgain_div);

    /*step3. set the same value of gain for t1/t2/t3/t4 */
    cgain_pow = 0x7 & cgain_pow;
    cgain_pow = cgain_pow | (cgain_pow << 4) | (cgain_pow << 8) | (cgain_pow << 12);
    result = AR0820_IsiWriteRegIss(handle, 0x3366, cgain_pow);
    fgain = 0xf & fgain;
    fgain = fgain | (fgain << 4) | (fgain << 8) | (fgain << 12);
    result |= AR0820_IsiWriteRegIss(handle, 0x336a, fgain);

    pAR0820Ctx->aecCurGain = (float)((1 << (cgain_pow & 0x7)) * (1 + (float)(fgain & 0xf)/16));
    pAR0820Ctx->aecCurLongGain = (float)((1 << (cgain_pow & 0x7)) * (1 + (float)(fgain & 0xf)/16));
    
    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT AR0820_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT AR0820_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    const AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    if (pAR0820Ctx == NULL) {
        TRACE(AR0820_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorAGain->gain[ISI_LINEAR_PARAS]       = pAR0820Ctx->aecCurGain;

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT AR0820_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    const AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    if (pAR0820Ctx == NULL) {
        TRACE(AR0820_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorDGain->gain[ISI_LINEAR_PARAS] = 1;

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT AR0820_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);
    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    uint32_t exp_line = 0;

    if (!pAR0820Ctx) {
        TRACE(AR0820_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAR0820Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        exp_line = pSensorIntTime->intTime[ISI_LINEAR_PARAS] / pAR0820Ctx->oneLineExpTime;
        exp_line = MIN(pAR0820Ctx->maxIntegrationLine, MAX(pAR0820Ctx->minIntegrationLine, exp_line));
        TRACE(AR0820_DEBUG, "%s: set exp_line=0x%04x\n", __func__, exp_line);

        result = AR0820_IsiWriteRegIss(handle, 0x3012, exp_line);//coarse integration time for T1
        pAR0820Ctx->aecCurIntegrationTime = exp_line * pAR0820Ctx->oneLineExpTime;

    } else if (pAR0820Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE || pAR0820Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_STITCH) {
        exp_line = pSensorIntTime->intTime[ISI_LINEAR_PARAS] / pAR0820Ctx->oneLineExpTime;
        exp_line = MIN(pAR0820Ctx->maxIntegrationLine, MAX(pAR0820Ctx->minIntegrationLine, exp_line));  
        // float  hdr_ratio = 16;
        // exp_ratio = (uint32_t) (((int)((hdr_ratio)*10) % 10) >= 5 ? (hdr_ratio+1) : (hdr_ratio));

        uint32_t middle_exp = 0;
        uint16_t coarse_exp_t1 = 0;
        uint16_t ratio_input = 0;
        //uint8_t  ratio = 2;

        // AR0820_IsiReadRegIss(handle, 0x32f6, &middle_exp);
        // middle_exp = middle_exp & 0xf; //middle integration time
        coarse_exp_t1 = (uint16_t)exp_line - middle_exp;

        // if (exp_ratio >= 16) {
        //     ratio_input = 4;
        // } else if(exp_ratio <= 1) {
        //     ratio_input = 2;//1
        // } else {
        //     for (ratio_input = 1; ratio_input < 4 ; ratio_input++) {
        //         if (exp_ratio > ratio) {
        //             ratio *= 2;
        //         } else {
        //             break;
        //         }
        //     }
        // }
        ratio_input = 4; //actual native hdr ratio = 2^4=16
        ratio_input = ratio_input & 0x7;
        ratio_input = ratio_input|(ratio_input<<4)|(ratio_input<<8);

        result = AR0820_IsiWriteRegIss(handle, 0x3012, coarse_exp_t1);//exp_line
        result |= AR0820_IsiWriteRegIss(handle, 0x3238, ratio_input);

        pAR0820Ctx->aecCurIntegrationTime = exp_line * pAR0820Ctx->oneLineExpTime;

        pAR0820Ctx->aecCurLongIntegrationTime = exp_line * pAR0820Ctx->oneLineExpTime;

    }  else {
        TRACE(AR0820_INFO, "%s:not support this hdr_mode.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT AR0820_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    const AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    if (!pAR0820Ctx) {
        TRACE(AR0820_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    pSensorIntTime->intTime[ISI_LINEAR_PARAS] = pAR0820Ctx->aecCurIntegrationTime;

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT AR0820_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t *pFps)
{
    const AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    if (pAR0820Ctx == NULL) {
        TRACE(AR0820_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pAR0820Ctx->currFps;

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT AR0820_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    if (pAR0820Ctx == NULL) {
        TRACE(AR0820_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (fps > pAR0820Ctx->maxFps) {
        TRACE(AR0820_ERROR, "%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, fps, pAR0820Ctx->maxFps, pAR0820Ctx->minFps, pAR0820Ctx->maxFps);
        fps = pAR0820Ctx->maxFps;
    }
    if (fps < pAR0820Ctx->minFps) {
        TRACE(AR0820_ERROR, "%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, fps, pAR0820Ctx->minFps, pAR0820Ctx->minFps, pAR0820Ctx->maxFps);
        fps = pAR0820Ctx->minFps;
    }

    uint16_t FrameLengthLines;
    FrameLengthLines = pAR0820Ctx->frameLengthLines * pAR0820Ctx->maxFps / fps;
    result = AR0820_IsiWriteRegIss(handle, 0x300a, FrameLengthLines);

    if (result != RET_SUCCESS) {
        TRACE(AR0820_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_FAILURE);
    }
    pAR0820Ctx->currFps = fps;
    pAR0820Ctx->curFrameLengthLines = FrameLengthLines;
    pAR0820Ctx->maxIntegrationLine = pAR0820Ctx->curFrameLengthLines - 3;
    pAR0820Ctx->aecMaxIntegrationTime = pAR0820Ctx->maxIntegrationLine * pAR0820Ctx->oneLineExpTime;

    TRACE(AR0820_INFO, "%s: set sensor fps = %d\n", __func__, pAR0820Ctx->currFps);

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT AR0820_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    if (pAR0820Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    if (pAR0820Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pIspStatus->useSensorAWB = true;
    } else {
        pIspStatus->useSensorAWB = false;
    }

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return result;
}

static RESULT AR0820_IsiSetWBIss(IsiSensorHandle_t handle, const IsiSensorWb_t *pWb)
{
    uint32_t rGain, grGain, gbGain, bGain;
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL || pAR0820Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)  return RET_NULL_POINTER;

//for isp awb gain, 256 means gain 1.0x ;ar0820 awb gain, 128 means gain 1.0x
    rGain  = (uint32_t)(pWb->rGain*128); 
    grGain = (uint32_t)(pWb->grGain*128);
    gbGain = (uint32_t)(pWb->gbGain*128);
    bGain  = (uint32_t)(pWb->bGain*128);

    if (pAR0820Ctx->sensorWb.grGain != pWb->grGain) {
//first statge, xxxx.yyyyyyy, 0x80 means 1.0x

        result = AR0820_IsiWriteRegIss(handle, 0x3056, grGain);//for exp t1
        result |= AR0820_IsiWriteRegIss(handle, 0x35a0, grGain);//for exp t2
        result |= AR0820_IsiWriteRegIss(handle, 0x35a8, grGain);//for exp t3
        result |= AR0820_IsiWriteRegIss(handle, 0x35b0, grGain);//for exp t4

//second statge , xx.yyyyyyyyy, 0x200 means 1.0x
        result |= AR0820_IsiWriteRegIss(handle, 0x3300, 0x200);
    }

    if (pAR0820Ctx->sensorWb.bGain != pWb->bGain) {
        result = AR0820_IsiWriteRegIss(handle,  0x3058, bGain);//for exp t1
        result |= AR0820_IsiWriteRegIss(handle, 0x35a2, bGain);//for exp t2
        result |= AR0820_IsiWriteRegIss(handle, 0x35aa, bGain);//for exp t3
        result |= AR0820_IsiWriteRegIss(handle, 0x35b2, bGain);//for exp t4

        result |= AR0820_IsiWriteRegIss(handle, 0x3302, 0x200);
    }

    if (pAR0820Ctx->sensorWb.rGain != pWb->rGain) {
        result = AR0820_IsiWriteRegIss(handle,  0x305a, rGain);//for exp t1
        result |= AR0820_IsiWriteRegIss(handle, 0x35a4, rGain);//for exp t2
        result |= AR0820_IsiWriteRegIss(handle, 0x35ac, rGain);//for exp t3
        result |= AR0820_IsiWriteRegIss(handle, 0x35b4, rGain);//for exp t4

        result |= AR0820_IsiWriteRegIss(handle, 0x3304, 0x200);
    }

    if (pAR0820Ctx->sensorWb.gbGain != pWb->gbGain) {
        result = AR0820_IsiWriteRegIss(handle,  0x305c, gbGain);//for exp t1
        result |= AR0820_IsiWriteRegIss(handle, 0x35a6, gbGain);//for exp t2
        result |= AR0820_IsiWriteRegIss(handle, 0x35ae, gbGain);//for exp t3
        result |= AR0820_IsiWriteRegIss(handle, 0x35b6, gbGain);//for exp t4

        result |= AR0820_IsiWriteRegIss(handle, 0x3306, 0x200);
    }

    memcpy(&pAR0820Ctx->sensorWb, pWb, sizeof(IsiSensorWb_t));

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT AR0820_IsiGetWBIss(IsiSensorHandle_t handle, IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    const AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)  return RET_NULL_POINTER;

    memcpy(pWb, &pAR0820Ctx->sensorWb, sizeof(IsiSensorWb_t));

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT AR0820_IsiSetBlcIss(IsiSensorHandle_t handle, const IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    const AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)  return RET_NULL_POINTER;

    result = AR0820_IsiWriteRegIss(handle, 0x3370, 0x0111);
    result |= AR0820_IsiWriteRegIss(handle, 0x301e, pBlc->red);

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT AR0820_IsiGetBlcIss(IsiSensorHandle_t handle, IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);
    uint16_t regValue;

    const AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)  return RET_NULL_POINTER;

    result = AR0820_IsiReadRegIss(handle, 0x301e, &regValue);

    pBlc->red  = regValue;
    pBlc->gr   = regValue;
    pBlc->gb   = regValue;
    pBlc->blue = regValue;

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT AR0820_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL || pAR0820Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pAR0820Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = AR0820_IsiWriteRegIss(handle, 0x3070, 0x0000);
    } else {
        result = AR0820_IsiWriteRegIss(handle, 0x3070, 0x0002);
    }

    pAR0820Ctx->testPattern = tpg.enable;

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT AR0820_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL || pAR0820Ctx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pAR0820Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!AR0820_IsiReadRegIss(handle, 0x3070, &value)) {
        pTpg->enable = ((value & 0x0002) != 0) ? 1 : 0;
        if(pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pAR0820Ctx->testPattern = pTpg->enable;
    }

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT AR0820_IsiGetExpandCurveIss(IsiSensorHandle_t handle, IsiSensorCompandCurve_t *pCurve)
{
    RESULT result = RET_SUCCESS;
    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL || pAR0820Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }
    
    if ((pCurve->xBit) == 12 && (pCurve->yBit == 20)) {
        uint8_t compand_px[64] = {7,6,6,7,6,6, 7,6,6,7,6,6, 8,7,7,8,7,7, 3,3,3,2,2, 4,4,4,3,3, 5,5,5,4,4, 6,6,6,5,5, 7,6,6,7,6,6, 4,4,4,3,3, 5,5,5,4,4, 6,6,6,5,5, 7,7,7,6,6};
        uint16_t curve[12] = {0};
        const uint32_t expandXDataExist[13] = {0, 0x200, 0x400, 0x800, 0x820, 0x860, 0x8e0, 0x9e0, 0xbe0, 0xc20, 0xca0, 0xda0, 0xfa0};
        const uint32_t expandYDataExist[13] = {0, 1<<9, 1<<10, 1<<11, 1<<12, 1<<13, 1<<14, 1<<15, 1<<16, 1<<17, 1<<18, 1<<19, 1<<20};

        for (int i=0; i<12; i++) {
            curve[i] = (expandYDataExist[i+1]-expandYDataExist[i]) / (expandXDataExist[i+1]-expandXDataExist[i]);
        }
        memcpy(pCurve->compandPx, compand_px, sizeof(compand_px));

        pCurve->compandXData[0] = 0;
        pCurve->compandYData[0] = 0;
        for (int i=1; i<65; i++) {
            pCurve->compandXData[i] = (1 << pCurve->compandPx[i-1]) + pCurve->compandXData[i-1];//get x-coordinate

            if (pCurve->compandXData[i] < expandXDataExist[1]) {
                pCurve->compandYData[i] = curve[0] * pCurve->compandXData[i] + expandYDataExist[0];
            } else if (pCurve->compandXData[i] < expandXDataExist[2]) {
                pCurve->compandYData[i] = curve[1] * (pCurve->compandXData[i] - expandXDataExist[1]) + expandYDataExist[1];
            } else if (pCurve->compandXData[i] < expandXDataExist[3]) {
                pCurve->compandYData[i] = curve[2] * (pCurve->compandXData[i] - expandXDataExist[2]) + expandYDataExist[2];
            } else if (pCurve->compandXData[i] < expandXDataExist[4]) {
                pCurve->compandYData[i] = curve[3] * (pCurve->compandXData[i] - expandXDataExist[3]) + expandYDataExist[3];
            } else if (pCurve->compandXData[i] < expandXDataExist[5]) {
                pCurve->compandYData[i] = curve[4] * (pCurve->compandXData[i] - expandXDataExist[4]) + expandYDataExist[4];
            } else if (pCurve->compandXData[i] < expandXDataExist[6]) {
                pCurve->compandYData[i] = curve[5] * (pCurve->compandXData[i] - expandXDataExist[5]) + expandYDataExist[5];
            } else if (pCurve->compandXData[i] < expandXDataExist[7]) {
                pCurve->compandYData[i] = curve[6] * (pCurve->compandXData[i] - expandXDataExist[6]) + expandYDataExist[6];
            } else if (pCurve->compandXData[i] < expandXDataExist[8]) {
                pCurve->compandYData[i] = curve[7] * (pCurve->compandXData[i] - expandXDataExist[7]) + expandYDataExist[7];
            } else if (pCurve->compandXData[i] < expandXDataExist[9]) {
                pCurve->compandYData[i] = curve[8] * (pCurve->compandXData[i] - expandXDataExist[8]) + expandYDataExist[8];
            } else if (pCurve->compandXData[i] < expandXDataExist[10]) {
                pCurve->compandYData[i] = curve[9] * (pCurve->compandXData[i] - expandXDataExist[9]) + expandYDataExist[9];
            } else if (pCurve->compandXData[i] < expandXDataExist[11]) {
                pCurve->compandYData[i] = curve[10] * (pCurve->compandXData[i] - expandXDataExist[10]) + expandYDataExist[10];
            } else {
                pCurve->compandYData[i] = curve[11] * (pCurve->compandXData[i] - expandXDataExist[11]) + expandYDataExist[11];
            }
        }
        return result;
    }

    return result;
}

static RESULT AR0820_IsiQueryMetadataAttr( IsiSensorHandle_t handle, IsiMetadataAttr_t *pAttr )
{
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    const AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pAttr == NULL)  return RET_NULL_POINTER;

    pAttr->mainAttr = AR0820_Metadata_Attr.mainAttr;
    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

static RESULT AR0820_IsiSetMetadataAttrEnable( IsiSensorHandle_t handle, IsiMetadataAttr_t attr )
{
    IsiMetadataAttr_t cond;
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    cond.mainAttr = (~AR0820_Metadata_Attr.mainAttr & attr.mainAttr);
    if ( cond.mainAttr != 0) {
        TRACE(AR0820_ERROR, "%s: the enabit attribute bit is not support!!\n", __func__, cond.mainAttr);
        return RET_WRONG_CONFIG;
    }

    pAR0820Ctx->metaEnable.mainAttr = attr.mainAttr;
    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

static RESULT AR0820_IsiGetMetadataAttrEnable( IsiSensorHandle_t handle, IsiMetadataAttr_t *pAttr )
{
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    pAttr->mainAttr = pAR0820Ctx->metaEnable.mainAttr;
    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

static RESULT AR0820_IsiGetMetadataWin          ( IsiSensorHandle_t handle, IsiMetadataWinInfo_t *pMetaWin )
{
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);
    const AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    IsiMetadataAttr_t  metaAttr = {0};
    //uint32_t imageWidth = 0;
    uint32_t imageHeight = 0;
    uint32_t vOffset = 0;
    uint8_t winId = 0;

    if (pAR0820Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if ( pMetaWin == NULL) {
        return RET_NULL_POINTER;
    }

    //imageWidth = pAR0820Ctx->sensorMode.size.width;
    imageHeight = pAR0820Ctx->sensorMode.size.height;
    metaAttr.mainAttr = pAR0820Ctx->metaEnable.mainAttr;

    MEMSET(pMetaWin, 0 , sizeof(IsiMetadataWinInfo_t));
    pMetaWin->winNum = 0;

    if (metaAttr.subAttr.expTime ==1
        || metaAttr.subAttr.again == 1
        || metaAttr.subAttr.dgain == 1) {

        pMetaWin->metaWin[winId].hStart = 0;
        pMetaWin->metaWin[winId].vStart = 0;
        pMetaWin->metaWin[winId].hSize = AR0820_METADATA_ONELINE_PIXEL;
        pMetaWin->metaWin[winId].vSize = 2;
        pMetaWin->winNum += 1;
        vOffset = pMetaWin->metaWin[winId].vStart + pMetaWin->metaWin[winId].vSize;
    }
    winId ++;
    if (metaAttr.subAttr.bls == 1
        ||metaAttr.subAttr.hist == 1
        || metaAttr.subAttr.meanLuma == 1) {

        pMetaWin->winNum +=1;
        pMetaWin->metaWin[winId].hStart = 0;
        pMetaWin->metaWin[winId].vStart = imageHeight + vOffset;
        pMetaWin->metaWin[winId].hSize = AR0820_METADATA_ONELINE_PIXEL;
        pMetaWin->metaWin[winId].vSize = 2;
    }

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

static RESULT AR0820_IsiParserMetadata(IsiSensorHandle_t handle, const MetadataBufInfo_t *pMetaBuf, IsiSensorMetadata_t *pMetaInfo)
{
    TRACE(AR0820_INFO, "%s: (enter)\n", __func__);

    const AR0820_Context_t *pAR0820Ctx = (AR0820_Context_t *) handle;
    if (pAR0820Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

//    typedef struct MetadataBufInfo_s
//    {
//        uint8_t bufNum;
//        uint8_t *pBuffer[METADATA_WIN_BUFFER_NUM];
//        uint32_t address[METADATA_WIN_BUFFER_NUM];
//        uint32_t bufSize;
//    } MetadataBufInfo_t;

    TRACE(AR0820_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}


RESULT AR0820_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "AR0820";
    TRACE(AR0820_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = AR0820_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = AR0820_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = AR0820_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = AR0820_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = AR0820_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = AR0820_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = AR0820_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = AR0820_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = AR0820_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = AR0820_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = AR0820_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = AR0820_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = AR0820_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = AR0820_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = AR0820_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = AR0820_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = AR0820_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = AR0820_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = AR0820_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = AR0820_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = AR0820_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = AR0820_IsiGetIspStatusIss;
        pIsiSensor->pIsiSetWBIss                        = AR0820_IsiSetWBIss;
        pIsiSensor->pIsiGetWBIss                        = AR0820_IsiGetWBIss;
        pIsiSensor->pIsiSetBlcIss                       = AR0820_IsiSetBlcIss;
        pIsiSensor->pIsiGetBlcIss                       = AR0820_IsiGetBlcIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = AR0820_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = AR0820_IsiGetTpgIss;
        pIsiSensor->pIsiGetExpandCurveIss               = AR0820_IsiGetExpandCurveIss;

        /* AF */
        pIsiSensor->pIsiFocusCreateIss                  = NULL;
        pIsiSensor->pIsiFocusReleaseIss                 = NULL;
        pIsiSensor->pIsiFocusGetCalibrateIss            = NULL;
        pIsiSensor->pIsiFocusSetIss                     = NULL;
        pIsiSensor->pIsiFocusGetIss                     = NULL;

        /* metadata*/
        pIsiSensor->pIsiQueryMetadataAttrIss            = AR0820_IsiQueryMetadataAttr;
        pIsiSensor->pIsiSetMetadataAttrEnableIss        = AR0820_IsiSetMetadataAttrEnable;
        pIsiSensor->pIsiGetMetadataAttrEnableIss        = AR0820_IsiGetMetadataAttrEnable;
        pIsiSensor->pIsiGetMetadataWinIss               = AR0820_IsiGetMetadataWin;
        pIsiSensor->pIsiParserMetadataIss               = AR0820_IsiParserMetadata;

    } else {
        result = RET_NULL_POINTER;
    }

    TRACE(AR0820_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t AR0820_IsiCamDrvConfig = {
    .cameraDriverID      = 0x0557,
    .pIsiGetSensorIss    = AR0820_IsiGetSensorIss,
};
