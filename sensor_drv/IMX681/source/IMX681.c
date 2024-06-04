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
#include "isi_otp.h"
#include "IMX681_priv.h"

CREATE_TRACER( IMX681_INFO , "IMX681: ", INFO,    1);
CREATE_TRACER( IMX681_WARN , "IMX681: ", WARNING, 1);
CREATE_TRACER( IMX681_ERROR, "IMX681: ", ERROR,   1);
CREATE_TRACER( IMX681_DEBUG,     "IMX681: ", INFO, 1);
CREATE_TRACER( IMX681_REG_INFO , "IMX681: ", INFO, 1);
CREATE_TRACER( IMX681_REG_DEBUG, "IMX681: ", INFO, 1);

#define H8V8
#define H1V1
//#define VSI_OTP_TEST
//#define OTP_ENABLE
/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pimx681_mode_info[] = {
    {
        .index         = 0,
        .size      = {
            .boundsWidth  = 4032,
            .boundsHeight = 3024,
            .top           = 0,
            .left          = 0,
            .width         = 4032,
            .height        = 3024,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps           = 5 * ISI_FPS_QUANTIZE,
        .hdrMode      = ISI_SENSOR_MODE_LINEAR,
        .bitWidth     = 10,
        .bayerPattern = ISI_BPAT_RGGB,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index         = 1,
        .size      = {
            .boundsWidth  = 2016,
            .boundsHeight = 1512,
            .top           = 0,
            .left          = 0,
            .width         = 2016,
            .height        = 1512,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps           = 17 * ISI_FPS_QUANTIZE,
        .hdrMode      = ISI_SENSOR_MODE_LINEAR,
        .bitWidth     = 10,
        .bayerPattern = ISI_BPAT_RGGB,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index         = 2,
        .size      = {
            .boundsWidth  = 1008,
            .boundsHeight = 756,
            .top           = 0,
            .left          = 0,
            .width         = 1008,
            .height        = 756,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps           = 30 * ISI_FPS_QUANTIZE,
        .hdrMode      = ISI_SENSOR_MODE_LINEAR,
        .bitWidth     = 10,
        .bayerPattern = ISI_BPAT_RGGB,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index         = 3,
        .size      = {
            .boundsWidth  = 1008,
            .boundsHeight = 756,
            .top           = 0,
            .left          = 0,
            .width         = 1008,
            .height        = 756,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps           = 30 * ISI_FPS_QUANTIZE,
        .hdrMode      = ISI_SENSOR_MODE_LINEAR,
        .bitWidth     = 8,
        .bayerPattern = ISI_BPAT_RGGB,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index         = 4,
        .size      = {
            .boundsWidth  = 496,
            .boundsHeight = 368,
            .top           = 0,
            .left          = 0,
            .width         = 496,
            .height        = 368,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps           = 30 * ISI_FPS_QUANTIZE,
        .hdrMode      = ISI_SENSOR_MODE_LINEAR,
        .bitWidth     = 10,
        .bayerPattern = ISI_BPAT_RGGB,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index         = 5,
        .size      = {
            .boundsWidth  = 496,
            .boundsHeight = 368,
            .top           = 0,
            .left          = 0,
            .width         = 496,
            .height        = 368,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps           = 30 * ISI_FPS_QUANTIZE,
        .hdrMode      = ISI_SENSOR_MODE_LINEAR,
        .bitWidth     = 8,
        .bayerPattern = ISI_BPAT_RGGB,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index         = 6,
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
        .fps           = 4 * ISI_FPS_QUANTIZE,
        .hdrMode      = ISI_SENSOR_MODE_LINEAR,
        .bitWidth     = 8,
        .bayerPattern = ISI_BPAT_RGGB,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index         = 7,
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
        .fps           = 7 * ISI_FPS_QUANTIZE,
        .hdrMode      = ISI_SENSOR_MODE_LINEAR,
        .bitWidth     = 8,
        .bayerPattern = ISI_BPAT_RGGB,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index         = 8,//vi200
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
        .fps           = 5 * ISI_FPS_QUANTIZE,
        .hdrMode      = ISI_SENSOR_MODE_LINEAR,
        .bitWidth     = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
};

static RESULT IMX681_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s (enter)\n", __func__);

    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    if (pIMX681Ctx == NULL || pIMX681Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pIMX681Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(IMX681_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(IMX681_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT IMX681_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s (enter)\n", __func__);

    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    if (pIMX681Ctx == NULL || pIMX681Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pIMX681Ctx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(IMX681_ERROR, "%s: hal write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(IMX681_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT IMX681_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(IMX681_INFO, "%s (enter)\n", __func__);
    const IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    if (pIMX681Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pIMX681Ctx->sensorMode), sizeof(pIMX681Ctx->sensorMode));

    TRACE(IMX681_INFO, "%s (exit)\n", __func__);
    return ( RET_SUCCESS );
}

static  RESULT IMX681_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(IMX681_INFO, "%s (enter)\n", __func__);
    const IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    if (pIMX681Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pimx681_mode_info)/sizeof(pimx681_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pimx681_mode_info)/sizeof(pimx681_mode_info[0])); i++) {
        if (pimx681_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pimx681_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(IMX681_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT IMX681_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(IMX681_INFO, "%s (enter)\n", __func__);

    if (pIMX681Ctx == NULL) return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pIMX681Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pIMX681Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pIMX681Ctx->sensorMode.size.width;
    pCaps->resolution.height = pIMX681Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_2LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else if (pCaps->bitWidth == 8) {
         pCaps->mipiMode      = ISI_FORMAT_RAW_8;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(IMX681_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX681_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle) {
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s (enter)\n", __func__);

    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) malloc(sizeof(IMX681_Context_t));
    if (!pIMX681Ctx) {
        TRACE(IMX681_ERROR, "%s: Can't allocate imx681 context\n",__func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pIMX681Ctx, 0, sizeof(IMX681_Context_t));

    pIMX681Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pIMX681Ctx->groupHold          = BOOL_FALSE;
    pIMX681Ctx->oldGain            = 0;
    pIMX681Ctx->oldIntegrationTime = 0;
    pIMX681Ctx->configured         = BOOL_FALSE;
    pIMX681Ctx->streaming          = BOOL_FALSE;
    pIMX681Ctx->testPattern        = BOOL_FALSE;
    pIMX681Ctx->isAfpsRun          = BOOL_FALSE;
    pIMX681Ctx->sensorMode.index   = 0;
    
    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x34;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;


    pIMX681Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pIMX681Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(IMX681_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pIMX681Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pIMX681Ctx);
        return (result);
    }

#ifdef OTP_ENABLE
    IsiSensorSccbCfg_t eepromSccbConfig;
    sccbConfig.slaveAddr = 0x50 << 1;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pIMX681Ctx->halEEPROMI2cHandle = HalI2cOpen(busId, eepromSccbConfig.slaveAddr, eepromSccbConfig.addrByte, eepromSccbConfig.dataByte);
    if (pIMX681Ctx->halEEPROMI2cHandle == NULL) {
        TRACE(IMX681_ERROR, "%s: hal I2c open EEPROM error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pIMX681Ctx->halEEPROMI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pIMX681Ctx);
        return (result);
    }

#endif

    *pHandle = (IsiSensorHandle_t) pIMX681Ctx;

    TRACE(IMX681_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX681_AecSetModeParameters(IsiSensorHandle_t handle, IMX681_Context_t *pIMX681Ctx)
{
    uint32_t regVal = 0;
    uint16_t value = 0;
    float fgain = 0;
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s%s: (enter)\n", __func__,pIMX681Ctx->isAfpsRun ? "(AFPS)" : "");

    pIMX681Ctx->aecIntegrationTimeIncrement = pIMX681Ctx->oneLineExpTime;
    pIMX681Ctx->aecMinIntegrationTime       = pIMX681Ctx->oneLineExpTime * pIMX681Ctx->minIntegrationLine;
    pIMX681Ctx->aecMaxIntegrationTime       = pIMX681Ctx->oneLineExpTime * pIMX681Ctx->maxIntegrationLine;

    TRACE(IMX681_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pIMX681Ctx->isAfpsRun ? "(AFPS)" : "",pIMX681Ctx->aecMaxIntegrationTime);

    pIMX681Ctx->aecGainIncrement = 1/256.0;

    //reflects the state of the sensor registers, must equal default settings
#if 0
    pIMX681Ctx->aecCurGain            = pIMX681Ctx->aecMinGain;
    pIMX681Ctx->aecCurIntegrationTime = 0.0f;
#else
    IMX681_IsiReadRegIss(handle, 0x0204, &value);
    regVal = (value << 8);
    IMX681_IsiReadRegIss(handle, 0x0205, &value);
    regVal = regVal | (value & 0xFF);
    fgain = 1024/(1024-regVal);
    if (!(fgain < 16)) {
        IMX681_IsiReadRegIss(handle, 0x020e, &value);
        regVal = (value << 8);
        IMX681_IsiReadRegIss(handle, 0x020f, &value);
        regVal = regVal | (value & 0xFF);
        fgain = (float)regVal/16;
    }
    pIMX681Ctx->aecCurGain            = fgain;

    IMX681_IsiReadRegIss(handle, 0x0229, &value);
    regVal = value << 16;
    IMX681_IsiReadRegIss(handle, 0x022a, &value);
    regVal = regVal | (value << 8);
    IMX681_IsiReadRegIss(handle, 0x022b, &value);
    regVal = regVal | (value & 0xFF);
    pIMX681Ctx->aecCurIntegrationTime = regVal * pIMX681Ctx->oneLineExpTime;
#endif
    pIMX681Ctx->oldGain               = 0;
    pIMX681Ctx->oldIntegrationTime    = 0;

    TRACE(IMX681_DEBUG, "%s%s: (exit) CurGain=%f  CurInt=%f  \n", __func__, pIMX681Ctx->isAfpsRun ? "(AFPS)" : "",
            pIMX681Ctx->aecCurGain, pIMX681Ctx->aecCurIntegrationTime);

    return (result);
}

static RESULT IMX681_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX681_INFO, "%s (enter)\n", __func__);

    if (!pIMX681Ctx) {
        TRACE(IMX681_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pIMX681Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pIMX681Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pimx681_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (pimx681_mode_info[i].index == pIMX681Ctx->sensorMode.index) {
            SensorDefaultMode = &(pimx681_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
        case 0:
            for (int i = 0; i<sizeof(IMX681_mipi2lane_H1V1_init) / sizeof(IMX681_mipi2lane_H1V1_init[0]); i++) {
                    IMX681_IsiWriteRegIss(handle, IMX681_mipi2lane_H1V1_init[i][0], IMX681_mipi2lane_H1V1_init[i][1]);
                }
            break;
        case 1:
            for (int i = 0; i<sizeof(IMX681_mipi2lane_H2V2_init) / sizeof(IMX681_mipi2lane_H2V2_init[0]); i++) {
                    IMX681_IsiWriteRegIss(handle, IMX681_mipi2lane_H2V2_init[i][0], IMX681_mipi2lane_H2V2_init[i][1]);
                }
            break;
        case 2:
            for (int i = 0; i<sizeof(IMX681_mipi2lane_H4V4_init) / sizeof(IMX681_mipi2lane_H4V4_init[0]); i++) {
                    IMX681_IsiWriteRegIss(handle, IMX681_mipi2lane_H4V4_init[i][0], IMX681_mipi2lane_H4V4_init[i][1]);
                }
            break;
        case 3:
            for (int i = 0; i<sizeof(IMX681_mipi2lane_H4V4_raw8_init) / sizeof(IMX681_mipi2lane_H4V4_raw8_init[0]); i++) {
                    IMX681_IsiWriteRegIss(handle, IMX681_mipi2lane_H4V4_raw8_init[i][0], IMX681_mipi2lane_H4V4_raw8_init[i][1]);
                }
            break;
        case 4:
            for (int i = 0; i<sizeof(IMX681_mipi2lane_H8V8_init) / sizeof(IMX681_mipi2lane_H8V8_init[0]); i++) {
                    IMX681_IsiWriteRegIss(handle, IMX681_mipi2lane_H8V8_init[i][0], IMX681_mipi2lane_H8V8_init[i][1]);
                }
            break;
        case 5:
            for (int i = 0; i<sizeof(IMX681_mipi2lane_H8V8_raw8_init) / sizeof(IMX681_mipi2lane_H8V8_raw8_init[0]); i++) {
                    IMX681_IsiWriteRegIss(handle, IMX681_mipi2lane_H8V8_raw8_init[i][0], IMX681_mipi2lane_H8V8_raw8_init[i][1]);
                }
            break;
        case 6:
            for (int i = 0; i<sizeof(IMX681_mipi2lane_H1V1_H8V8_init) / sizeof(IMX681_mipi2lane_H1V1_H8V8_init[0]); i++) {
                    IMX681_IsiWriteRegIss(handle, IMX681_mipi2lane_H1V1_H8V8_init[i][0], IMX681_mipi2lane_H1V1_H8V8_init[i][1]);
                }
            break;
        case 7:
            for (int i = 0; i<sizeof(IMX681_mipi2lane_H2V2_H8V8_init) / sizeof(IMX681_mipi2lane_H2V2_H8V8_init[0]); i++) {
                    IMX681_IsiWriteRegIss(handle, IMX681_mipi2lane_H2V2_H8V8_init[i][0], IMX681_mipi2lane_H2V2_H8V8_init[i][1]);
                }
            break;
        case 8:
            for (int i = 0; i<sizeof(IMX681_mipi2lane_4K_VI200_init) / sizeof(IMX681_mipi2lane_4K_VI200_init[0]); i++) {
                    IMX681_IsiWriteRegIss(handle, IMX681_mipi2lane_4K_VI200_init[i][0], IMX681_mipi2lane_4K_VI200_init[i][1]);
                }
            break;
        default:
            TRACE(IMX681_INFO, "%s:not support sensor mode %d\n", __func__,pIMX681Ctx->sensorMode.index);
            return RET_NOTSUPP;
       }

        memcpy(&(pIMX681Ctx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));
    } else {
        TRACE(IMX681_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pIMX681Ctx->sensorMode.index) {
        case 0:
            pIMX681Ctx->oneLineExpTime      = 0.00006916;
            pIMX681Ctx->frameLengthLines    = 0xC0B;
            pIMX681Ctx->curFrameLengthLines = pIMX681Ctx->frameLengthLines;
            pIMX681Ctx->maxIntegrationLine  = pIMX681Ctx->frameLengthLines -8 ;
            pIMX681Ctx->minIntegrationLine  = 4;
            pIMX681Ctx->aecMaxGain          = 128;
            pIMX681Ctx->aecMinGain          = 1.0;
            pIMX681Ctx->aGain.min           = 1.0;
            pIMX681Ctx->aGain.max           = 16.0;
            pIMX681Ctx->aGain.step          = (1.0f/256.0f);
            pIMX681Ctx->dGain.min           = 1.0;
            pIMX681Ctx->dGain.max           = 8;
            pIMX681Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 1:
            pIMX681Ctx->oneLineExpTime      = 0.00003556;
            pIMX681Ctx->frameLengthLines    = 0x65A;
            pIMX681Ctx->curFrameLengthLines = pIMX681Ctx->frameLengthLines;
            pIMX681Ctx->maxIntegrationLine  = pIMX681Ctx->curFrameLengthLines - 8;
            pIMX681Ctx->minIntegrationLine  = 4;
            pIMX681Ctx->aecMaxGain          = 128;
            pIMX681Ctx->aecMinGain          = 1.0;
            pIMX681Ctx->aGain.min           = 1.0;
            pIMX681Ctx->aGain.max           = 16.0;
            pIMX681Ctx->aGain.step          = (1.0f/256.0f);
            pIMX681Ctx->dGain.min           = 1.0;
            pIMX681Ctx->dGain.max           = 8;
            pIMX681Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 2:
            pIMX681Ctx->oneLineExpTime      = 0.00001876;
            pIMX681Ctx->frameLengthLines    = 0x6F0;
            pIMX681Ctx->curFrameLengthLines = pIMX681Ctx->frameLengthLines;
            pIMX681Ctx->maxIntegrationLine  = pIMX681Ctx->frameLengthLines -8 ;
            pIMX681Ctx->minIntegrationLine  = 4;
            pIMX681Ctx->aecMaxGain          = 128;
            pIMX681Ctx->aecMinGain          = 1.0;
            pIMX681Ctx->aGain.min           = 1.0;
            pIMX681Ctx->aGain.max           = 16.0;
            pIMX681Ctx->aGain.step          = (1.0f/256.0f);
            pIMX681Ctx->dGain.min           = 1.0;
            pIMX681Ctx->dGain.max           = 8;
            pIMX681Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 3:
            pIMX681Ctx->oneLineExpTime      = 0.00001876;
            pIMX681Ctx->frameLengthLines    = 0x6F0;
            pIMX681Ctx->curFrameLengthLines = pIMX681Ctx->frameLengthLines;
            pIMX681Ctx->maxIntegrationLine  = pIMX681Ctx->frameLengthLines -8 ;
            pIMX681Ctx->minIntegrationLine  = 4;
            pIMX681Ctx->aecMaxGain          = 128;
            pIMX681Ctx->aecMinGain          = 1.0;
            pIMX681Ctx->aGain.min           = 1.0;
            pIMX681Ctx->aGain.max           = 16.0;
            pIMX681Ctx->aGain.step          = (1.0f/256.0f);
            pIMX681Ctx->dGain.min           = 1.0;
            pIMX681Ctx->dGain.max           = 8;
            pIMX681Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 4:
            pIMX681Ctx->oneLineExpTime      = 0.00001136;
            pIMX681Ctx->frameLengthLines    = 0xB76;
            pIMX681Ctx->curFrameLengthLines = pIMX681Ctx->frameLengthLines;
            pIMX681Ctx->maxIntegrationLine  = pIMX681Ctx->frameLengthLines -8 ;
            pIMX681Ctx->minIntegrationLine  = 4;
            pIMX681Ctx->aecMaxGain          = 128;
            pIMX681Ctx->aecMinGain          = 1.0;
            pIMX681Ctx->aGain.min           = 1.0;
            pIMX681Ctx->aGain.max           = 16.0;
            pIMX681Ctx->aGain.step          = (1.0f/256.0f);
            pIMX681Ctx->dGain.min           = 1.0;
            pIMX681Ctx->dGain.max           = 8;
            pIMX681Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 5:
            pIMX681Ctx->oneLineExpTime      = 0.00001136;
            pIMX681Ctx->frameLengthLines    = 0xB76;
            pIMX681Ctx->curFrameLengthLines = pIMX681Ctx->frameLengthLines;
            pIMX681Ctx->maxIntegrationLine  = pIMX681Ctx->frameLengthLines -8 ;
            pIMX681Ctx->minIntegrationLine  = 4;
            pIMX681Ctx->aecMaxGain          = 128;
            pIMX681Ctx->aecMinGain          = 1.0;
            pIMX681Ctx->aGain.min           = 1.0;
            pIMX681Ctx->aGain.max           = 16.0;
            pIMX681Ctx->aGain.step          = (1.0f/256.0f);
            pIMX681Ctx->dGain.min           = 1.0;
            pIMX681Ctx->dGain.max           = 8;
            pIMX681Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 6:
            pIMX681Ctx->oneLineExpTime      = 0.0000774;
            pIMX681Ctx->frameLengthLines    = 0xC0B;
            pIMX681Ctx->curFrameLengthLines = pIMX681Ctx->frameLengthLines;
            pIMX681Ctx->maxIntegrationLine  = pIMX681Ctx->frameLengthLines -8 ;
            pIMX681Ctx->minIntegrationLine  = 4;
            pIMX681Ctx->aecMaxGain          = 128;
            pIMX681Ctx->aecMinGain          = 1.0;
            pIMX681Ctx->aGain.min           = 1.0;
            pIMX681Ctx->aGain.max           = 16.0;
            pIMX681Ctx->aGain.step          = (1.0f/256.0f);
            pIMX681Ctx->dGain.min           = 1.0;
            pIMX681Ctx->dGain.max           = 8;
            pIMX681Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 7:
            pIMX681Ctx->oneLineExpTime      = 0.000091;
            pIMX681Ctx->frameLengthLines    = 0x658;
            pIMX681Ctx->curFrameLengthLines = pIMX681Ctx->frameLengthLines;
            pIMX681Ctx->maxIntegrationLine  = pIMX681Ctx->frameLengthLines -8 ;
            pIMX681Ctx->minIntegrationLine  = 4;
            pIMX681Ctx->aecMaxGain          = 128;
            pIMX681Ctx->aecMinGain          = 1.0;
            pIMX681Ctx->aGain.min           = 1.0;
            pIMX681Ctx->aGain.max           = 16.0;
            pIMX681Ctx->aGain.step          = (1.0f/256.0f);
            pIMX681Ctx->dGain.min           = 1.0;
            pIMX681Ctx->dGain.max           = 8;
            pIMX681Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 8:
            pIMX681Ctx->oneLineExpTime      = 0.000091;
            pIMX681Ctx->frameLengthLines    = 0x8AB;
            pIMX681Ctx->curFrameLengthLines = pIMX681Ctx->frameLengthLines;
            pIMX681Ctx->maxIntegrationLine  = pIMX681Ctx->frameLengthLines -8 ;
            pIMX681Ctx->minIntegrationLine  = 4;
            pIMX681Ctx->aecMaxGain          = 128;
            pIMX681Ctx->aecMinGain          = 1.0;
            pIMX681Ctx->aGain.min           = 1.0;
            pIMX681Ctx->aGain.max           = 16.0;
            pIMX681Ctx->aGain.step          = (1.0f/256.0f);
            pIMX681Ctx->dGain.min           = 1.0;
            pIMX681Ctx->dGain.max           = 8;
            pIMX681Ctx->dGain.step          = (1.0f/256.0f);
            break;
        default:
            TRACE(IMX681_INFO, "%s:not support sensor mode %d\n", __func__,pIMX681Ctx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    pIMX681Ctx->maxFps  = pIMX681Ctx->sensorMode.fps;
    pIMX681Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pIMX681Ctx->currFps = pIMX681Ctx->maxFps;

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(IMX681_DEBUG, "%s: IMX681 System-Reset executed\n", __func__);
    osSleep(100);

    result = IMX681_AecSetModeParameters(handle, pIMX681Ctx);
    if (result != RET_SUCCESS) {
        TRACE(IMX681_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pIMX681Ctx->configured = BOOL_TRUE;
    TRACE(IMX681_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT IMX681_IsiCloseIss(IsiSensorHandle_t handle)
{
    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s (enter)\n", __func__);

    if (pIMX681Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)IMX681_IsiSetStreamingIss(pIMX681Ctx, BOOL_FALSE);

    TRACE(IMX681_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX681_IsiReleaseIss(IsiSensorHandle_t handle)
{
    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s (enter)\n", __func__);

    if (pIMX681Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pIMX681Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pIMX681Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

#ifdef OTP_ENABLE
    HalI2cClose(pIMX681Ctx->halEEPROMI2cHandle);
    (void)HalDelRef(pIMX681Ctx->halEEPROMI2cHandle, HAL_DEV_I2C);
#endif
    MEMSET(pIMX681Ctx, 0, sizeof(IMX681_Context_t));
    free(pIMX681Ctx);
    TRACE(IMX681_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX681_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;

    uint32_t sensorId = 0;
    uint32_t correctId = 0x0000;

    TRACE(IMX681_INFO, "%s (enter)\n", __func__);

    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    if (pIMX681Ctx == NULL || pIMX681Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = IMX681_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(IMX681_ERROR, "%s: Read Sensor ID Error! \n", __func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(IMX681_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n",__func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(IMX681_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__,correctId, sensorId);
    TRACE(IMX681_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX681_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(IMX681_INFO, "%s (enter)\n", __func__);

    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    if (pIMX681Ctx == NULL || pIMX681Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    regVal = 0;
    result = IMX681_IsiReadRegIss(handle, 0x002a, &regVal);
    sensorId = (regVal & 0xff) << 8;

    regVal = 0;
    result |= IMX681_IsiReadRegIss(handle, 0x002b, &regVal);
    sensorId |= (regVal & 0xff);

    *pValue = sensorId;
    TRACE(IMX681_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX681_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on) {
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s (enter)\n", __func__);

    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    if (pIMX681Ctx == NULL || pIMX681Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pIMX681Ctx->configured != BOOL_TRUE) return RET_WRONG_STATE;

    if(!on)  IMX681_IsiWriteRegIss(handle, 0x0106, 0);
    result = IMX681_IsiWriteRegIss(handle, 0x0100, on);
    if (result != RET_SUCCESS) {
        TRACE(IMX681_ERROR, "%s: set sensor streaming error! \n",__func__);
        return (RET_FAILURE);
    }

    pIMX681Ctx->streaming = on;

    TRACE(IMX681_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX681_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    const IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX681_INFO, "%s: (enter)\n", __func__);

    if (pIMX681Ctx == NULL) {
        TRACE(IMX681_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(IMX681_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    //get time limit and total gain limit
    pAeBaseInfo->gain.min        = pIMX681Ctx->aecMinGain;
    pAeBaseInfo->gain.max        = pIMX681Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pIMX681Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pIMX681Ctx->aecMaxIntegrationTime;

    //get again/dgain info
    pAeBaseInfo->aGain           = pIMX681Ctx->aGain;
    pAeBaseInfo->dGain           = pIMX681Ctx->dGain;
    
    pAeBaseInfo->aecCurGain     = pIMX681Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime  = pIMX681Ctx->aecCurIntegrationTime;
    pAeBaseInfo->aecGainStep    = pIMX681Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep = pIMX681Ctx->aecIntegrationTimeIncrement;

    TRACE(IMX681_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT IMX681_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s: (enter)\n", __func__);
    uint32_t again = 0, dgain = 0; 

    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    if (pIMX681Ctx == NULL || pIMX681Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    again = 1024-1024/pSensorAGain->gain[ISI_LINEAR_PARAS];
    if(again >= 960) {
        again = 960;
        dgain = pSensorAGain->gain[ISI_LINEAR_PARAS] * 16;
        if( dgain <= 256) dgain = 256;
    }

#ifdef  H1V1
        result |= IMX681_IsiWriteRegIss(handle, 0x0204, (again >> 8) & 0xff);
        result |=  IMX681_IsiWriteRegIss(handle, 0x0205, again & 0xff );
#endif
#ifdef H8V8
        result |= IMX681_IsiWriteRegIss(handle, 0xe80a, (again >> 8) & 0xff);
        result |=  IMX681_IsiWriteRegIss(handle, 0xe80b, again & 0xff );
#endif
         if(dgain > 1) {
#ifdef H1V1
        result |= IMX681_IsiWriteRegIss(handle, 0x020e, (dgain >> 8) & 0xff);
        result |=  IMX681_IsiWriteRegIss(handle, 0x020f, dgain & 0xff );
        result |= IMX681_IsiWriteRegIss(handle, 0x0210, (dgain >> 8) & 0xff);
        result |=  IMX681_IsiWriteRegIss(handle, 0x0211, dgain & 0xff );

        result |= IMX681_IsiWriteRegIss(handle, 0x0212, (dgain >> 8) & 0xff);
        result |=  IMX681_IsiWriteRegIss(handle, 0x0213, dgain & 0xff );
        result |= IMX681_IsiWriteRegIss(handle, 0x0214, (dgain >> 8) & 0xff);
        result |=  IMX681_IsiWriteRegIss(handle, 0x0215, dgain & 0xff );
#endif
#ifdef H8V8
        result |= IMX681_IsiWriteRegIss(handle, 0xe80c, (dgain >> 8) & 0xff);
        result |= IMX681_IsiWriteRegIss(handle, 0xe80d, dgain & 0xff);
        result |= IMX681_IsiWriteRegIss(handle, 0xe80e, (dgain >> 8) & 0xff);
        result |= IMX681_IsiWriteRegIss(handle, 0xe80f, dgain & 0xff);

        result |= IMX681_IsiWriteRegIss(handle, 0xe810, (dgain >> 8) & 0xff);
        result |= IMX681_IsiWriteRegIss(handle, 0xe811, dgain & 0xff);
        result |= IMX681_IsiWriteRegIss(handle, 0xe812, (dgain >> 8) & 0xff);
        result |= IMX681_IsiWriteRegIss(handle, 0xe813, dgain & 0xff);
#endif
        }

    TRACE(IMX681_DEBUG, " pIMX681Ctx Set Gain again %d dgain %d NewGain  %f\n",again,dgain,pSensorAGain->gain[ISI_LINEAR_PARAS]);
    pIMX681Ctx->curAgain = pSensorAGain->gain[ISI_LINEAR_PARAS];
    pIMX681Ctx->curDgain = dgain/256.0f;
    pIMX681Ctx->aecCurGain = pSensorAGain->gain[ISI_LINEAR_PARAS];

    TRACE(IMX681_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX681_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s: (enter)\n", __func__);

    TRACE(IMX681_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX681_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    const IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s: (enter)\n", __func__);

    if (pIMX681Ctx == NULL) {
        TRACE(IMX681_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorAGain->gain[ISI_LINEAR_PARAS] = pIMX681Ctx->curAgain;

    TRACE(IMX681_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX681_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    const IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s: (enter)\n", __func__);

    if (pIMX681Ctx == NULL) {
        TRACE(IMX681_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorDGain->gain[ISI_LINEAR_PARAS] = pIMX681Ctx->curDgain;

    TRACE(IMX681_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX681_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s: (enter)\n", __func__);
    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pIMX681Ctx) {
        TRACE(IMX681_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = pSensorIntTime->intTime[ISI_LINEAR_PARAS] / pIMX681Ctx->oneLineExpTime;
    expLine = MIN(pIMX681Ctx->maxIntegrationLine, MAX(1, expLine));
    TRACE(IMX681_DEBUG, "%s: set expLine=0x%04x\n", __func__, expLine);

#ifdef H1V1
    result &=  IMX681_IsiWriteRegIss(handle, 0x0229, (expLine >>16) & 0xff);
    result &= IMX681_IsiWriteRegIss(handle,  0x022A, (expLine >> 8& 0xff));
    result &= IMX681_IsiWriteRegIss(handle,  0x022B, (expLine & 0xff));
#endif
#ifdef H8V8
    result &=  IMX681_IsiWriteRegIss(handle, 0xE815, (expLine >>16) & 0xff);
    result &= IMX681_IsiWriteRegIss(handle,  0xE816, (expLine >> 8& 0xff));
    result &= IMX681_IsiWriteRegIss(handle,  0xE817, (expLine & 0xff));
#endif

    pIMX681Ctx->aecCurIntegrationTime = expLine * pIMX681Ctx->oneLineExpTime;

    TRACE(IMX681_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX681_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    const IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s: (enter)\n", __func__);

    if (!pIMX681Ctx) {
        TRACE(IMX681_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    pSensorIntTime->intTime[ISI_LINEAR_PARAS] = pIMX681Ctx->aecCurIntegrationTime;

    TRACE(IMX681_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX681_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t *pFps)
{
    const IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s: (enter)\n", __func__);

    if (pIMX681Ctx == NULL) {
        TRACE(IMX681_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pIMX681Ctx->currFps;

    TRACE(IMX681_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX681_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    RESULT result = RET_SUCCESS;
    int32_t NewVts = 0;
    TRACE(IMX681_INFO, "%s: (enter)\n", __func__);

    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    if (pIMX681Ctx == NULL) {
        TRACE(IMX681_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (fps > pIMX681Ctx->maxFps) {
        TRACE(IMX681_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__,
                             fps, pIMX681Ctx->maxFps, pIMX681Ctx->minFps,pIMX681Ctx->maxFps);
        fps = pIMX681Ctx->maxFps;
    }
    if (fps < pIMX681Ctx->minFps) {
        TRACE(IMX681_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__,
                            fps, pIMX681Ctx->minFps, pIMX681Ctx->minFps,pIMX681Ctx->maxFps);
        fps = pIMX681Ctx->minFps;
    }

    NewVts = pIMX681Ctx->frameLengthLines*pIMX681Ctx->sensorMode.fps / fps;

    #ifdef  H1V1
    result  =  IMX681_IsiWriteRegIss(handle, 0x033d, NewVts >> 16);
    result |=  IMX681_IsiWriteRegIss(handle, 0x033e, ((NewVts >> 8) & 0xff));
    result |=  IMX681_IsiWriteRegIss(handle, 0x033f, NewVts & 0xff);
    #endif

    #ifdef  H8V8
    result |=  IMX681_IsiWriteRegIss(handle, 0xe819, NewVts >> 16);
    result |=  IMX681_IsiWriteRegIss(handle, 0xe81a, ((NewVts >> 8) & 0xff));
    result |=  IMX681_IsiWriteRegIss(handle, 0xe81b, NewVts & 0xff);
    #endif

    pIMX681Ctx->currFps              = fps;
    pIMX681Ctx->curFrameLengthLines  = NewVts;
    pIMX681Ctx->maxIntegrationLine   = pIMX681Ctx->curFrameLengthLines - 8;
    //pIMX681Ctx->aecMaxIntegrationTime = pIMX681Ctx->maxIntegrationLine * pIMX681Ctx->oneLineExpTime;

    TRACE(IMX681_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX681_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    const IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    if (pIMX681Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(IMX681_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    pIspStatus->useSensorAWB = false;

    TRACE(IMX681_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT IMX681_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s: (enter)\n", __func__);

    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    if (pIMX681Ctx == NULL || pIMX681Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pIMX681Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = IMX681_IsiWriteRegIss(handle, 0x0600, 0x00);
        result |= IMX681_IsiWriteRegIss(handle, 0x0601, 0x00);
    } else {
        result = IMX681_IsiWriteRegIss(handle, 0x0600, 0x00);
        result |= IMX681_IsiWriteRegIss(handle, 0x0601, 0x02);
    }

    pIMX681Ctx->testPattern = tpg.enable;

    TRACE(IMX681_INFO, "%s: (exit)\n", __func__);
    return (result);
}
RESULT IMX681_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(IMX681_INFO, "%s: (enter)\n", __func__);

    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    if (pIMX681Ctx == NULL || pIMX681Ctx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pIMX681Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!IMX681_IsiReadRegIss(handle, 0x6001,&value)) {
        pTpg->enable = ((value & 0x02) != 0) ? 1 : 0;
        if(pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pIMX681Ctx->testPattern = pTpg->enable;
    }

    TRACE(IMX681_INFO, "%s: (exit)\n", __func__);
    return (result);
}

#ifdef OTP_ENABLE
uint8_t IMX681_ReadOTPIss(IsiSensorHandle_t handle, uint16_t regAddr )
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX681_INFO, "%s: (enter)\n", __func__);
    IMX681_Context_t *pIMX681Ctx = (IMX681_Context_t *) handle;
    uint16_t value = 0;

    result = HalReadI2CReg(pIMX681Ctx->halEEPROMI2cHandle, regAddr, &value);
    if (result != RET_SUCCESS) {
        TRACE(IMX681_ERROR, "%s: hal read sensor register error!\n", __func__);
        return -1;
    }

    TRACE(IMX681_INFO, "%s: reg_addr = 0x%04x value = %02x \n", __func__, regAddr, value);
    TRACE(IMX681_INFO, "%s: (exit)\n", __func__);
    return value;
}
#endif

#ifdef VSI_OTP_TEST
uint8_t IMX681_ReadOTPIss(IsiSensorHandle_t handle, uint16_t regAddr )
{
    TRACE(IMX681_INFO, "%s: (enter)\n", __func__);
    uint16_t value = 0;

    for(int i=0; i<5070; i++) {
        if(regAddr == IMX681_OTP_data[i][0]) {
            value = IMX681_OTP_data[i][1];
            break;
        }
    }

    TRACE(IMX681_INFO, "%s: reg_addr = 0x%04x value = %02x \n", __func__, regAddr, value);
    TRACE(IMX681_INFO, "%s: (exit)\n", __func__);
    return value;
}

RESULT IMX681_IsiGetOtpDataIss(IsiSensorHandle_t handle, uint32_t *length, void *pOtpData)
{
  RESULT result = RET_SUCCESS;
  TRACE(IMX681_INFO, "%s: (enter)\n", __func__);
  IsiOTPV1_t *OtpData = (IsiOTPV1_t *) pOtpData;
  int row = 0, col = 0;
  uint16_t SensorIDSum = 0, ModuleInfoSum = 0, ReservedDataSum = 0, LSC4000KSum = 0, Awb5800KSum = 0, 
  Awb4000KSum = 0, Awb3100KSum = 0, AwbOverallSum = 0, GoldenAwb5800KSum = 0,
  GoldenAwb4000KSum = 0, GoldenAwb3100KSum = 0, GoldenOverallAwbSum = 0,
  LightSource5800KSum = 0,  LightSource4000KSum = 0, LightSource3100KSum = 0,
  LightSourceOverallSum = 0, offset = 0;
  OtpData->otpLSCEnable         = true;
  OtpData->otpAwbEnable         = true;
  OtpData->otpLightSourceEnable = true;
  OtpData->otpFocusEnable       = false;
  OtpData->dataCheckResult      = true;
  OtpData->otpInformation.hwVersion      = IMX681_ReadOTPIss(handle, 0x0000);
  OtpData->otpInformation.eepromRevision = IMX681_ReadOTPIss(handle, 0x0001);
  OtpData->otpInformation.sensorRevision = IMX681_ReadOTPIss(handle, 0x0002);
  OtpData->otpInformation.tlensRevision  = IMX681_ReadOTPIss(handle, 0x0003);
  OtpData->otpInformation.ircfRevision   = IMX681_ReadOTPIss(handle, 0x0004);
  OtpData->otpInformation.lensRevision   = IMX681_ReadOTPIss(handle, 0x0005);
  OtpData->otpInformation.caRevision     = IMX681_ReadOTPIss(handle, 0x0006);
  OtpData->otpInformation.moduleInteID   = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0007)<<8) | (IMX681_ReadOTPIss(handle, 0x0008)));
  OtpData->otpInformation.factoryID      = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0009)<<8) | (IMX681_ReadOTPIss(handle, 0x000a)));
  OtpData->otpInformation.mirrorFlip     = IMX681_ReadOTPIss(handle, 0x000b);
  OtpData->otpInformation.tlensSlaveID   = IMX681_ReadOTPIss(handle, 0x000c);
  OtpData->otpInformation.eepromSlaveID  = IMX681_ReadOTPIss(handle, 0x000d);
  OtpData->otpInformation.sensorSlaveID  = IMX681_ReadOTPIss(handle, 0x000e);
  for (int i=0; i<11; i++) {
    OtpData->otpInformation.sensorID[i]  = IMX681_ReadOTPIss(handle, 0x000f+i);
    SensorIDSum += OtpData->otpInformation.sensorID[i];
  }
  uint16_t SensorIDCheckSum =  (((uint16_t)IMX681_ReadOTPIss(handle, 0x001a)<<8) | (IMX681_ReadOTPIss(handle, 0x001b)));
  if (SensorIDSum != SensorIDCheckSum) {
    OtpData->dataCheckResult = false;
    TRACE(IMX681_ERROR, "%s: SensorID data error!\n", __func__);
  }

  OtpData->otpInformation.manuDateYear   =  (((uint16_t)IMX681_ReadOTPIss(handle, 0x001c)<<8) | (IMX681_ReadOTPIss(handle, 0x001d)));
  OtpData->otpInformation.manuDateMonth  =  (((uint16_t)IMX681_ReadOTPIss(handle, 0x001e)<<8) | (IMX681_ReadOTPIss(handle, 0x001f)));
  OtpData->otpInformation.manuDateDay    =  (((uint16_t)IMX681_ReadOTPIss(handle, 0x0020)<<8) | (IMX681_ReadOTPIss(handle, 0x0021)));
  for (int i=0; i<12; i++) {
     OtpData->otpInformation.barcodeModuleSN[i] = IMX681_ReadOTPIss(handle, 0x0022+i);
  }
  OtpData->otpInformation.mapTotalSize   =  (((uint16_t)IMX681_ReadOTPIss(handle, 0x002e)<<8) | (IMX681_ReadOTPIss(handle, 0x002f)));
  uint16_t ModuleinfoCheckSum =  (((uint16_t)IMX681_ReadOTPIss(handle, 0x0030)<<8) | (IMX681_ReadOTPIss(handle, 0x0031)));
  for (int i=0; i<48; i++) {
    ModuleInfoSum += IMX681_ReadOTPIss(handle, 0x0000+i);
  }
  if (ModuleInfoSum != ModuleinfoCheckSum) {
    OtpData->dataCheckResult = false;
    TRACE(IMX681_ERROR, "%s: Module info data error!\n", __func__);
  }
  ModuleInfoSum += (IMX681_ReadOTPIss(handle, 0x0030) + IMX681_ReadOTPIss(handle, 0x0031));
  
  for (int i=0; i<21; i++) {
    ReservedDataSum += IMX681_ReadOTPIss(handle, 0x0032+i);
  }

  for(row = 0; row<ISI_OTP_LSC_TABLE_NUM; row++) {
    for (col = 0; col<ISI_OTP_LSC_TABLE_NUM; col++) {
       offset = ISI_OTP_LSC_TABLE_NUM*2*row+col*2;
       OtpData->lsc4000K.r[row][col]   = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0047+offset)<<8) | (IMX681_ReadOTPIss(handle, 0x0047+offset+1)));
       OtpData->lsc4000K.gr[row][col]  = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0289+offset)<<8) | (IMX681_ReadOTPIss(handle, 0x0289+offset+1)));
       OtpData->lsc4000K.gb[row][col]  = (((uint16_t)IMX681_ReadOTPIss(handle, 0x04cb+offset)<<8) | (IMX681_ReadOTPIss(handle, 0x04cb+offset+1)));
       OtpData->lsc4000K.b[row][col]   = (((uint16_t)IMX681_ReadOTPIss(handle, 0x070d+offset)<<8) | (IMX681_ReadOTPIss(handle, 0x070d+offset+1)));
   }
 }
  uint16_t LSCCheckSum_4000K  = (((uint16_t)IMX681_ReadOTPIss(handle, 0x094f)<<8) | (IMX681_ReadOTPIss(handle, 0x0950)));
  for (int i=0; i<2312; i++) {
    LSC4000KSum += IMX681_ReadOTPIss(handle, 0x0047+i);
  }
  if (LSC4000KSum != LSCCheckSum_4000K) {
    OtpData->dataCheckResult = false;
    TRACE(IMX681_ERROR, "%s: LSC_4000K data error!\n", __func__);
  }
  LSC4000KSum += (IMX681_ReadOTPIss(handle, 0x094f) + IMX681_ReadOTPIss(handle, 0x0950));

   OtpData->awb5800K.r        = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0951)<<8) | (IMX681_ReadOTPIss(handle, 0x0952)));
   OtpData->awb5800K.gb       = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0955)<<8) | (IMX681_ReadOTPIss(handle, 0x0956)));
   OtpData->awb5800K.b        = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0957)<<8) | (IMX681_ReadOTPIss(handle, 0x0958)));
   OtpData->awb5800K.rgRatio  = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0959)<<8) | (IMX681_ReadOTPIss(handle, 0x095a)));
   OtpData->awb5800K.bgRatio  = (((uint16_t)IMX681_ReadOTPIss(handle, 0x095b)<<8) | (IMX681_ReadOTPIss(handle, 0x095c)));
   uint16_t AwbCheckSum_5800K  = (((uint16_t)IMX681_ReadOTPIss(handle, 0x095d)<<8) | (IMX681_ReadOTPIss(handle, 0x095e)));
   for (int i=0; i<12; i++) {
     Awb5800KSum += IMX681_ReadOTPIss(handle, 0x0951+i);
   }
   if (Awb5800KSum != AwbCheckSum_5800K) {
     OtpData->dataCheckResult = false;
     TRACE(IMX681_ERROR, "%s: Awb_5800K data error!\n", __func__);
   }

   OtpData->awb4000K.r        = (((uint16_t)IMX681_ReadOTPIss(handle, 0x095f)<<8) | (IMX681_ReadOTPIss(handle, 0x0960)));
   OtpData->awb4000K.gr       = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0961)<<8) | (IMX681_ReadOTPIss(handle, 0x0962)));
   OtpData->awb4000K.gb       = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0963)<<8) | (IMX681_ReadOTPIss(handle, 0x0964)));
   OtpData->awb4000K.b        = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0965)<<8) | (IMX681_ReadOTPIss(handle, 0x0966)));
   OtpData->awb4000K.rgRatio = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0967)<<8) | (IMX681_ReadOTPIss(handle, 0x0968)));
   OtpData->awb4000K.bgRatio = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0969)<<8) | (IMX681_ReadOTPIss(handle, 0x096a)));
   uint16_t AwbCheckSum_4000K  = (((uint16_t)IMX681_ReadOTPIss(handle, 0x096b)<<8) | (IMX681_ReadOTPIss(handle, 0x096c)));
   for (int i=0; i<12; i++) {
     Awb4000KSum += IMX681_ReadOTPIss(handle, 0x095f+i);
   }
   if (Awb4000KSum != AwbCheckSum_4000K) {
     OtpData->dataCheckResult = false;
     TRACE(IMX681_ERROR, "%s: Awb_4000K data error!\n", __func__);
   }

   OtpData->awb3100K.r        = (((uint16_t)IMX681_ReadOTPIss(handle, 0x096d)<<8) | (IMX681_ReadOTPIss(handle, 0x096e)));
   OtpData->awb3100K.gr       = (((uint16_t)IMX681_ReadOTPIss(handle, 0x096f)<<8) | (IMX681_ReadOTPIss(handle, 0x0970)));
   OtpData->awb3100K.gb       = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0971)<<8) | (IMX681_ReadOTPIss(handle, 0x0972)));
   OtpData->awb3100K.b        = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0973)<<8) | (IMX681_ReadOTPIss(handle, 0x0974)));
   OtpData->awb3100K.rgRatio = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0975)<<8) | (IMX681_ReadOTPIss(handle, 0x0976)));
   OtpData->awb3100K.bgRatio = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0977)<<8) | (IMX681_ReadOTPIss(handle, 0x0978)));
   uint16_t AwbCheckSum_3100K         = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0979)<<8) | (IMX681_ReadOTPIss(handle, 0x097a)));
   for (int i=0; i<12; i++) {
     Awb3100KSum += IMX681_ReadOTPIss(handle, 0x096d+i);
   }
   if (Awb3100KSum != AwbCheckSum_3100K) {
     OtpData->dataCheckResult = false;
     TRACE(IMX681_ERROR, "%s: Awb_3100K data error!\n", __func__);
   }
   uint16_t AwbOverallCheckSum        = (((uint16_t)IMX681_ReadOTPIss(handle, 0x097b)<<8) | (IMX681_ReadOTPIss(handle, 0x097c)));
   for (int i=0; i<42; i++) {
     AwbOverallSum += IMX681_ReadOTPIss(handle, 0x0951+i);
   }
   if (AwbOverallSum != AwbOverallCheckSum) {
     OtpData->dataCheckResult = false;
     TRACE(IMX681_ERROR, "%s: AwbOverallSum data error!\n", __func__);
   }
   AwbOverallSum += (IMX681_ReadOTPIss(handle, 0x097b) + IMX681_ReadOTPIss(handle, 0x097c));

   OtpData->goldenAwb5800K.r         = (((uint16_t)IMX681_ReadOTPIss(handle, 0x097d)<<8) | (IMX681_ReadOTPIss(handle, 0x097e)));
   OtpData->goldenAwb5800K.gr        = (((uint16_t)IMX681_ReadOTPIss(handle, 0x097f)<<8) | (IMX681_ReadOTPIss(handle, 0x0980)));
   OtpData->goldenAwb5800K.gb        = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0981)<<8) | (IMX681_ReadOTPIss(handle, 0x0982)));
   OtpData->goldenAwb5800K.b         = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0983)<<8) | (IMX681_ReadOTPIss(handle, 0x0984)));
   OtpData->goldenAwb5800K.rgRatio   = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0985)<<8) | (IMX681_ReadOTPIss(handle, 0x0986)));
   OtpData->goldenAwb5800K.bgRatio   = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0987)<<8) | (IMX681_ReadOTPIss(handle, 0x0988)));
   uint16_t GoldenAwbCheckSum_5800K  = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0989)<<8) | (IMX681_ReadOTPIss(handle, 0x098a)));
   for (int i=0; i<12; i++) {
     GoldenAwb5800KSum += IMX681_ReadOTPIss(handle, 0x097d+i);
   }
   if (GoldenAwb5800KSum != GoldenAwbCheckSum_5800K) {
     OtpData->dataCheckResult = false;
     TRACE(IMX681_ERROR, "%s: goldenAwb5800K data error!\n", __func__);
   }

   OtpData->goldenAwb4000K.r         = (((uint16_t)IMX681_ReadOTPIss(handle, 0x098b)<<8) | (IMX681_ReadOTPIss(handle, 0x098c)));
   OtpData->goldenAwb4000K.gr        = (((uint16_t)IMX681_ReadOTPIss(handle, 0x098d)<<8) | (IMX681_ReadOTPIss(handle, 0x098e)));
   OtpData->goldenAwb4000K.gb        = (((uint16_t)IMX681_ReadOTPIss(handle, 0x098f)<<8) | (IMX681_ReadOTPIss(handle, 0x0990)));
   OtpData->goldenAwb4000K.b         = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0991)<<8) | (IMX681_ReadOTPIss(handle, 0x0992)));
   OtpData->goldenAwb4000K.rgRatio   = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0993)<<8) | (IMX681_ReadOTPIss(handle, 0x0994)));
   OtpData->goldenAwb4000K.bgRatio   = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0995)<<8) | (IMX681_ReadOTPIss(handle, 0x0996)));
   uint16_t GoldenAwbCheckSum_4000K  = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0997)<<8) | (IMX681_ReadOTPIss(handle, 0x0998)));
   for (int i=0; i<12; i++) {
     GoldenAwb4000KSum += IMX681_ReadOTPIss(handle, 0x098b+i);
   }
   if (GoldenAwb4000KSum != GoldenAwbCheckSum_4000K) {
     OtpData->dataCheckResult = false;
     TRACE(IMX681_ERROR, "%s: goldenAwb4000K data error!\n", __func__);
   }

   OtpData->goldenAwb3100K.r         = (((uint16_t)IMX681_ReadOTPIss(handle, 0x0999)<<8) | (IMX681_ReadOTPIss(handle, 0x099a)));
   OtpData->goldenAwb3100K.gr        = (((uint16_t)IMX681_ReadOTPIss(handle, 0x099b)<<8) | (IMX681_ReadOTPIss(handle, 0x099c)));
   OtpData->goldenAwb3100K.gb        = (((uint16_t)IMX681_ReadOTPIss(handle, 0x099d)<<8) | (IMX681_ReadOTPIss(handle, 0x099e)));
   OtpData->goldenAwb3100K.b         = (((uint16_t)IMX681_ReadOTPIss(handle, 0x099f)<<8) | (IMX681_ReadOTPIss(handle, 0x09a0)));
   OtpData->goldenAwb3100K.rgRatio  = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09a1)<<8) | (IMX681_ReadOTPIss(handle, 0x09a2)));
   OtpData->goldenAwb3100K.bgRatio  = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09a3)<<8) | (IMX681_ReadOTPIss(handle, 0x09a4)));
   uint16_t GoldenAwbCheckSum_3100K   = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09a5)<<8) | (IMX681_ReadOTPIss(handle, 0x09a6)));
   for (int i=0; i<12; i++) {
     GoldenAwb3100KSum += IMX681_ReadOTPIss(handle, 0x0999+i);
   }
   if (GoldenAwb3100KSum != GoldenAwbCheckSum_3100K) {
     OtpData->dataCheckResult = false;
     TRACE(IMX681_ERROR, "%s: goldenAwb3100K data error!\n", __func__);
   }
   uint16_t GoldenOverallAwbCheckSum  = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09a7)<<8) | (IMX681_ReadOTPIss(handle, 0x09a8)));
   for (int i=0; i<42; i++) {
     GoldenOverallAwbSum += IMX681_ReadOTPIss(handle, 0x097d+i);
   }
   if (GoldenOverallAwbSum != GoldenOverallAwbCheckSum) {
     OtpData->dataCheckResult = false;
     TRACE(IMX681_ERROR, "%s: GoldenOverallAwbSum data error!\n", __func__);
   }
   GoldenOverallAwbSum += (IMX681_ReadOTPIss(handle, 0x09a7) + IMX681_ReadOTPIss(handle, 0x09a8));

   OtpData->lightSource5800K.xCIE      = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09a9)<<8) | (IMX681_ReadOTPIss(handle, 0x09aa)));
   OtpData->lightSource5800K.yCIE      = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09ab)<<8) | (IMX681_ReadOTPIss(handle, 0x09ac)));
   OtpData->lightSource5800K.intensity = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09ad)<<8) | (IMX681_ReadOTPIss(handle, 0x09ae)));
   uint16_t LightSourceCheckSum_5800K   = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09af)<<8) | (IMX681_ReadOTPIss(handle, 0x09b0)));
   for (int i=0; i<6; i++) {
     LightSource5800KSum += IMX681_ReadOTPIss(handle, 0x09a9+i);
   }
   if (LightSource5800KSum != LightSourceCheckSum_5800K) {
     OtpData->dataCheckResult = false;
     TRACE(IMX681_ERROR, "%s: LightSource_5800K data error!\n", __func__);
   }

   OtpData->lightSource4000K.xCIE      = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09b1)<<8) | (IMX681_ReadOTPIss(handle, 0x09b2)));
   OtpData->lightSource4000K.yCIE      = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09b3)<<8) | (IMX681_ReadOTPIss(handle, 0x09b4)));
   OtpData->lightSource4000K.intensity = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09b5)<<8) | (IMX681_ReadOTPIss(handle, 0x09b6)));
   uint16_t LightSourceCheckSum_4000K   = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09b7)<<8) | (IMX681_ReadOTPIss(handle, 0x09b8)));
   for (int i=0; i<6; i++) {
     LightSource4000KSum += IMX681_ReadOTPIss(handle, 0x09b1+i);
   }
   if (LightSource4000KSum != LightSourceCheckSum_4000K) {
     OtpData->dataCheckResult = false;
     TRACE(IMX681_ERROR, "%s: LightSource_4000K data error!\n", __func__);
   }

   OtpData->lightSource3100K.xCIE      = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09b9)<<8) | (IMX681_ReadOTPIss(handle, 0x09ba)));
   OtpData->lightSource3100K.yCIE      = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09bb)<<8) | (IMX681_ReadOTPIss(handle, 0x09bc)));
   OtpData->lightSource3100K.intensity = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09bd)<<8) | (IMX681_ReadOTPIss(handle, 0x09be)));
   uint16_t LightSourceCheckSum_3100K   = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09bf)<<8) | (IMX681_ReadOTPIss(handle, 0x09c0)));
   for (int i=0; i<6; i++) {
     LightSource3100KSum += IMX681_ReadOTPIss(handle, 0x09b9+i);
   }
   if (LightSource3100KSum != LightSourceCheckSum_3100K) {
     OtpData->dataCheckResult = false;
     TRACE(IMX681_ERROR, "%s: LightSource_3100K data error!\n", __func__);
   }
   uint16_t LightSourceOverallCheckSum  = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09c1)<<8) | (IMX681_ReadOTPIss(handle, 0x09c2)));
   for (int i=0; i<24; i++) {
     LightSourceOverallSum += IMX681_ReadOTPIss(handle, 0x09a9+i);
   }
   if (LightSourceOverallSum != LightSourceOverallCheckSum) {
     OtpData->dataCheckResult = false;
     TRACE(IMX681_ERROR, "%s: LightSourceOverallSum data error!\n", __func__);
   }
   LightSourceOverallSum += (IMX681_ReadOTPIss(handle, 0x09c1) + IMX681_ReadOTPIss(handle, 0x09c2));
   
   uint16_t OverallCheckSum = (((uint16_t)IMX681_ReadOTPIss(handle, 0x09c3)<<8) | (IMX681_ReadOTPIss(handle, 0x09c4)));
   uint16_t OverallSum = ModuleInfoSum + ReservedDataSum + LSC4000KSum + AwbOverallSum + GoldenOverallAwbSum + LightSourceOverallSum;
   if (OverallSum != OverallCheckSum) {
     OtpData->dataCheckResult = false;
     TRACE(IMX681_ERROR, "%s: OverallSum data error!\n", __func__);
   }

   if (OtpData->dataCheckResult == false) {
    return (RET_INVALID_PARM);
   }

   TRACE(IMX681_INFO, "%s: (exit)\n", __func__);
   return (result);
}
#endif

RESULT IMX681_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "IMX681";
    TRACE( IMX681_INFO, "%s (enter)\n", __func__);

    if ( pIsiSensor != NULL ) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = IMX681_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = IMX681_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = IMX681_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = IMX681_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = IMX681_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = IMX681_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = IMX681_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = IMX681_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = IMX681_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = IMX681_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = IMX681_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = IMX681_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = IMX681_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = IMX681_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = IMX681_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = IMX681_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = IMX681_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = IMX681_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = IMX681_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = IMX681_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = IMX681_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = IMX681_IsiGetIspStatusIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = IMX681_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = IMX681_IsiGetTpgIss;
#ifdef VSI_OTP_TEST
        pIsiSensor->pIsiGetOtpDataIss                   = IMX681_IsiGetOtpDataIss;
#endif

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

    TRACE( IMX681_INFO, "%s (exit)\n", __func__);
    return ( result );
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t IMX681_IsiCamDrvConfig = {
    .cameraDriverID      = 0x0000,
    .pIsiGetSensorIss    = IMX681_IsiGetSensorIss,
};
