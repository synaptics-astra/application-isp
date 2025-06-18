/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2014 - 2024 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/

#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <common/misc.h>
#include "isi.h"
#include "isi_iss.h"
#include "OV50a40_priv.h"

CREATE_TRACER(OV50a40_INFO , "OV50a40: ", INFO,    1);
CREATE_TRACER(OV50a40_WARN , "OV50a40: ", WARNING, 1);
CREATE_TRACER(OV50a40_ERROR, "OV50a40: ", ERROR,   1);
CREATE_TRACER(OV50a40_DEBUG,     "OV50a40: ", INFO, 0);
CREATE_TRACER(OV50a40_REG_INFO , "OV50a40: ", INFO, 1);
CREATE_TRACER(OV50a40_REG_DEBUG, "OV50a40: ", INFO, 1);

#define OV50a40_MIN_GAIN_STEP    (1.0f/128.0f)  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain) */

/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pov50a40_mode_info[] = {
    {
        .index     = 0,
        .size      = {
            .boundsWidth  = 1280,
            .boundsHeight = 720,
            .top           = 0,
            .left          = 0,
            .width         = 1280,
            .height        = 720,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps       = 7.8 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 1,
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
        .fps       = 4.25 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_RGGB,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 2,
        .size      = {
            .boundsWidth  = 7680,
            .boundsHeight = 4320,
            .top           = 0,
            .left          = 0,
            .width         = 7680,
            .height        = 4320,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps       = 1.8 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_RGGB,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 3, // 8k 2lane
        .size      = {
            .boundsWidth  = 7680,
            .boundsHeight = 4320,
            .top           = 0,
            .left          = 0,
            .width         = 7680,
            .height        = 4320,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps       = 2.5 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_RGGB,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    }
};

static RESULT OV50a40_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pOV50a40Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(OV50a40_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV50a40_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV50a40_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(OV50a40_ERROR, "%s: NULL POINTER!\n", __func__);
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pOV50a40Ctx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(OV50a40_ERROR, "%s: hal write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV50a40_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV50a40_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);
    const OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pOV50a40Ctx->sensorMode), sizeof(pOV50a40Ctx->sensorMode));

    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT OV50a40_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);
    const OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pov50a40_mode_info)/sizeof(pov50a40_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pov50a40_mode_info)/sizeof(pov50a40_mode_info[0])); i++) {
        if (pov50a40_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pov50a40_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT OV50a40_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    if (pOV50a40Ctx == NULL) return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pOV50a40Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pOV50a40Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pOV50a40Ctx->sensorMode.size.width;
    pCaps->resolution.height = pOV50a40Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;
    if (3 == pOV50a40Ctx->sensorMode.index) {
        pCaps->mipiLanes         = ISI_MIPI_2LANES;
    }

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) osMalloc(sizeof(OV50a40_Context_t));
    if (!pOV50a40Ctx) {
        TRACE(OV50a40_ERROR, "%s: Can't allocate ov50a40 context\n",__func__);
        return (RET_OUTOFMEM);
    }
    MEMSET(pOV50a40Ctx, 0, sizeof(OV50a40_Context_t));

    pOV50a40Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pOV50a40Ctx->groupHold          = BOOL_FALSE;
    pOV50a40Ctx->oldGain            = 0;
    pOV50a40Ctx->oldIntegrationTime = 0;
    pOV50a40Ctx->configured         = BOOL_FALSE;
    pOV50a40Ctx->streaming          = BOOL_FALSE;
    pOV50a40Ctx->testPattern        = BOOL_FALSE;
    pOV50a40Ctx->isAfpsRun          = BOOL_FALSE;
    pOV50a40Ctx->sensorMode.index   = 0;

    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x6c;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pOV50a40Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(OV50a40_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pOV50a40Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        osFree(pOV50a40Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pOV50a40Ctx;

    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_AecSetModeParameters(IsiSensorHandle_t handle, OV50a40_Context_t * pOV50a40Ctx)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    uint32_t regVal = 0;
    float32_t again = 0, dgain = 0;
    TRACE(OV50a40_INFO, "%s%s: (enter)\n", __func__, pOV50a40Ctx->isAfpsRun ? "(AFPS)" : "");

    pOV50a40Ctx->aecIntegrationTimeIncrement = pOV50a40Ctx->oneLineExpTime;
    pOV50a40Ctx->aecMinIntegrationTime       = pOV50a40Ctx->oneLineExpTime * pOV50a40Ctx->minIntegrationLine;
    pOV50a40Ctx->aecMaxIntegrationTime       = pOV50a40Ctx->oneLineExpTime * pOV50a40Ctx->maxIntegrationLine;

    TRACE(OV50a40_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pOV50a40Ctx->isAfpsRun ? "(AFPS)" : "",pOV50a40Ctx->aecMaxIntegrationTime);

    pOV50a40Ctx->aecGainIncrement = OV50a40_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pOV50a40Ctx->oldGain               = 0;
    pOV50a40Ctx->oldIntegrationTime    = 0;

    OV50a40_IsiReadRegIss(handle, 0x3508, &value);
    regVal = (value & 0x7f) << 7;
    OV50a40_IsiReadRegIss(handle, 0x3509, &value);
    regVal = regVal | ((value >> 1) & 0x7f);
    again = regVal/128.0f;
    dgain = 1.0f;
    pOV50a40Ctx->aecCurGain = again * dgain;

    
    OV50a40_IsiReadRegIss(handle, 0x3500, &value);
    regVal = value << 16;
    OV50a40_IsiReadRegIss(handle, 0x3501, &value);
    regVal = regVal | (value << 8);
    OV50a40_IsiReadRegIss(handle, 0x3502, &value);
    regVal = regVal | (value & 0xFF);
    pOV50a40Ctx->aecCurIntegrationTime = regVal * pOV50a40Ctx->oneLineExpTime;

    TRACE(OV50a40_INFO, "%s%s: (exit)\n", __func__,pOV50a40Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT OV50a40_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    if (!pOV50a40Ctx) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOV50a40Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pOV50a40Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pov50a40_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (pov50a40_mode_info[i].index == pOV50a40Ctx->sensorMode.index) {
            SensorDefaultMode = &(pov50a40_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(OV50a40_mipi4lane_720p_init) / sizeof(OV50a40_mipi4lane_720p_init[0]); i++) {
                    OV50a40_IsiWriteRegIss(handle, OV50a40_mipi4lane_720p_init[i][0], OV50a40_mipi4lane_720p_init[i][1]);
                }
                break;
            case 1:
                for (int i = 0; i<sizeof(OV50a40_mipi4lane_4k_init) / sizeof(OV50a40_mipi4lane_4k_init[0]); i++) {
                    OV50a40_IsiWriteRegIss(handle, OV50a40_mipi4lane_4k_init[i][0], OV50a40_mipi4lane_4k_init[i][1]);
                }
                break;
            case 2:
                for (int i = 0; i<sizeof(OV50a40_mipi4lane_8k_init) / sizeof(OV50a40_mipi4lane_8k_init[0]); i++) {
                    OV50a40_IsiWriteRegIss(handle, OV50a40_mipi4lane_8k_init[i][0], OV50a40_mipi4lane_8k_init[i][1]);
                }
                break;
            case 3:
                for (int i = 0; i<sizeof(OV50a40_mipi4lane_8k_2lane_init) / sizeof(OV50a40_mipi4lane_8k_2lane_init[0]); i++) {
                    OV50a40_IsiWriteRegIss(handle, OV50a40_mipi4lane_8k_2lane_init[i][0], OV50a40_mipi4lane_8k_2lane_init[i][1]);
                }
                break;
            default:
                TRACE(OV50a40_INFO, "%s:not support sensor mode %d\n", __func__,pOV50a40Ctx->sensorMode.index);
                return RET_NOTSUPP;
                break;
    }

        memcpy(&(pOV50a40Ctx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));

    } else {
        TRACE(OV50a40_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pOV50a40Ctx->sensorMode.index) {
        case 0:
            pOV50a40Ctx->oneLineExpTime   = 0.000048;
            pOV50a40Ctx->frameLengthLines    = 0xa64;
            pOV50a40Ctx->curFrameLengthLines = pOV50a40Ctx->frameLengthLines;
            pOV50a40Ctx->maxIntegrationLine  = pOV50a40Ctx->frameLengthLines -32 ;
            pOV50a40Ctx->minIntegrationLine  = 4;
            pOV50a40Ctx->aecMaxGain          = 127.99;
            pOV50a40Ctx->aecMinGain          = 1.0;
            pOV50a40Ctx->aGain.min           = 1.0;
            pOV50a40Ctx->aGain.max           = 127.99;
            pOV50a40Ctx->aGain.step          = (1.0f/128.0f);
            pOV50a40Ctx->dGain.min           = 1.0;
            pOV50a40Ctx->dGain.max           = 1.0;
            pOV50a40Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 1:
            pOV50a40Ctx->oneLineExpTime   = 0.000066;
            pOV50a40Ctx->frameLengthLines    = 0xe00;
            pOV50a40Ctx->curFrameLengthLines = pOV50a40Ctx->frameLengthLines;
            pOV50a40Ctx->maxIntegrationLine  = pOV50a40Ctx->frameLengthLines -32 ;
            pOV50a40Ctx->minIntegrationLine  = 4;
            pOV50a40Ctx->aecMaxGain          = 127.99;
            pOV50a40Ctx->aecMinGain          = 1.0;
            pOV50a40Ctx->aGain.min           = 1.0;
            pOV50a40Ctx->aGain.max           = 127.99;
            pOV50a40Ctx->aGain.step          = (1.0f/128.0f);
            pOV50a40Ctx->dGain.min           = 1.0;
            pOV50a40Ctx->dGain.max           = 1.0;
            pOV50a40Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 2:
            pOV50a40Ctx->oneLineExpTime      = 0.000121;
            pOV50a40Ctx->frameLengthLines    = 0x116c;
            pOV50a40Ctx->curFrameLengthLines = pOV50a40Ctx->frameLengthLines;
            pOV50a40Ctx->maxIntegrationLine  = pOV50a40Ctx->frameLengthLines -32 ;
            pOV50a40Ctx->minIntegrationLine  = 4;
            pOV50a40Ctx->aecMaxGain          = 127.99;
            pOV50a40Ctx->aecMinGain          = 1.0;
            pOV50a40Ctx->aGain.min           = 1.0;
            pOV50a40Ctx->aGain.max           = 127.99;
            pOV50a40Ctx->aGain.step          = (1.0f/128.0f);
            pOV50a40Ctx->dGain.min           = 1.0;
            pOV50a40Ctx->dGain.max           = 1.0;
            pOV50a40Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 3:
            pOV50a40Ctx->oneLineExpTime      = 0.000064;
            pOV50a40Ctx->frameLengthLines    = 0x186c;
            pOV50a40Ctx->curFrameLengthLines = pOV50a40Ctx->frameLengthLines;
            pOV50a40Ctx->maxIntegrationLine  = pOV50a40Ctx->frameLengthLines -32 ;
            pOV50a40Ctx->minIntegrationLine  = 4;
            pOV50a40Ctx->aecMaxGain          = 127.99;
            pOV50a40Ctx->aecMinGain          = 1.0;
            pOV50a40Ctx->aGain.min           = 1.0;
            pOV50a40Ctx->aGain.max           = 127.99;
            pOV50a40Ctx->aGain.step          = (1.0f/128.0f);
            pOV50a40Ctx->dGain.min           = 1.0;
            pOV50a40Ctx->dGain.max           = 1.0;
            pOV50a40Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        default:
            TRACE(OV50a40_INFO, "%s:not support sensor mode %d\n", __func__,pOV50a40Ctx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    pOV50a40Ctx->maxFps  = pOV50a40Ctx->sensorMode.fps;
    pOV50a40Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pOV50a40Ctx->currFps = pOV50a40Ctx->maxFps;

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(OV50a40_DEBUG, "%s: OV50a40 System-Reset executed\n", __func__);
    osSleep(100);

    result = OV50a40_AecSetModeParameters(handle, pOV50a40Ctx);
    if (result != RET_SUCCESS) {
        TRACE(OV50a40_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pOV50a40Ctx->configured = BOOL_TRUE;
    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT OV50a40_IsiCloseIss(IsiSensorHandle_t handle)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    if (pOV50a40Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)OV50a40_IsiSetStreamingIss(pOV50a40Ctx, BOOL_FALSE);

    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_IsiReleaseIss(IsiSensorHandle_t handle)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    if (pOV50a40Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)HalDelRef(pOV50a40Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    HalI2cClose(pOV50a40Ctx->isiCtx.halI2cHandle);

    MEMSET(pOV50a40Ctx, 0, sizeof(OV50a40_Context_t));
    osFree(pOV50a40Ctx);
    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;

    uint32_t sensorId = 0;
    uint32_t correctId = 0x5650;

    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = OV50a40_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(OV50a40_ERROR, "%s: Read Sensor ID Error! \n",__func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(OV50a40_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n", __func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(OV50a40_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__, correctId, sensorId);
    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (!pValue) return (RET_NULL_POINTER);

    regVal   = 0;
    result    = OV50a40_IsiReadRegIss(handle, 0x300a, &regVal);
    sensorId = (regVal & 0xff) << 8;

    regVal   = 0;
    result   |= OV50a40_IsiReadRegIss(handle, 0x300b, &regVal);
    sensorId |= (regVal & 0xff);

    *pValue = sensorId;
    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV50a40Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    result = OV50a40_IsiWriteRegIss(handle, 0x0100, on);
    if (result != RET_SUCCESS) {
        TRACE(OV50a40_ERROR, "%s: set sensor streaming error! \n",__func__);
        return (RET_FAILURE);
    }

    pOV50a40Ctx->streaming = on;

    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV50a40_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    if (pOV50a40Ctx == NULL) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(OV50a40_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    /* linear mode*/
    pAeBaseInfo->gain.min        = pOV50a40Ctx->aecMinGain;//total gain
    pAeBaseInfo->gain.max        = pOV50a40Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pOV50a40Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pOV50a40Ctx->aecMaxIntegrationTime;

    pAeBaseInfo->aGain           = pOV50a40Ctx->aGain;//min, max, step
    pAeBaseInfo->dGain           = pOV50a40Ctx->dGain;

    pAeBaseInfo->aecCurGain      = pOV50a40Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime   = pOV50a40Ctx->aecCurIntegrationTime;

    pAeBaseInfo->aecGainStep     = pOV50a40Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep  = pOV50a40Ctx->aecIntegrationTimeIncrement;

    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);
    uint32_t again = 0;

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    again = (uint32_t)(pSensorAGain->gain[ISI_LINEAR_PARAS] * 128);
    result = OV50a40_IsiWriteRegIss(handle, 0x3508, (again >> 7) & 0x7f);
    result |= OV50a40_IsiWriteRegIss(handle, 0x3509,(again & 0x7f) << 1);
    pOV50a40Ctx->curAgain = again/128.0f;

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    pOV50a40Ctx->curDgain = 1.0;
    pOV50a40Ctx->aecCurGain = pOV50a40Ctx->curAgain * pOV50a40Ctx->curDgain;

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    if (pOV50a40Ctx == NULL) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorAGain->gain[ISI_LINEAR_PARAS]       = pOV50a40Ctx->curAgain;

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    if (pOV50a40Ctx == NULL) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorDGain->gain[ISI_LINEAR_PARAS] = pOV50a40Ctx->curDgain;

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);
    uint32_t expLine = 0;
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;

    if (!pOV50a40Ctx) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = pSensorIntTime->intTime[ISI_LINEAR_PARAS] / pOV50a40Ctx->oneLineExpTime;
    expLine = MIN(pOV50a40Ctx->maxIntegrationLine, MAX(pOV50a40Ctx->minIntegrationLine, expLine));
    TRACE(OV50a40_DEBUG, "%s: set expLine = 0x%04x\n", __func__, expLine);

    result  = OV50a40_IsiWriteRegIss(handle, 0x3500, (expLine >> 16) & 0xff);
    result |= OV50a40_IsiWriteRegIss(handle, 0x3501, (expLine >> 8) & 0xff);
    result |= OV50a40_IsiWriteRegIss(handle, 0x3502, (expLine & 0xff));

    pOV50a40Ctx->aecCurIntegrationTime = expLine * pOV50a40Ctx->oneLineExpTime;

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    if (!pOV50a40Ctx) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    pSensorIntTime->intTime[ISI_LINEAR_PARAS] = pOV50a40Ctx->aecCurIntegrationTime;

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t * pFps)
{
    const OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    if (pOV50a40Ctx == NULL) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pOV50a40Ctx->currFps;

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    RESULT result = RET_SUCCESS;
    int32_t NewVts = 0;

    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL) {
        TRACE(OV50a40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

   if (fps > pOV50a40Ctx->maxFps) {
        TRACE(OV50a40_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, fps, pOV50a40Ctx->maxFps, pOV50a40Ctx->minFps,pOV50a40Ctx->maxFps);
        fps = pOV50a40Ctx->maxFps;
    }
    if (fps < pOV50a40Ctx->minFps) {
        TRACE(OV50a40_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, fps, pOV50a40Ctx->minFps, pOV50a40Ctx->minFps,pOV50a40Ctx->maxFps);
        fps = pOV50a40Ctx->minFps;
    }

    NewVts = pOV50a40Ctx->frameLengthLines*pOV50a40Ctx->sensorMode.fps / fps;
    result =  OV50a40_IsiWriteRegIss(handle, 0x380e, NewVts >> 8);
    result |=  OV50a40_IsiWriteRegIss(handle, 0x380f, NewVts & 0xff);
    pOV50a40Ctx->currFps              = fps;
    pOV50a40Ctx->curFrameLengthLines  = NewVts;
    pOV50a40Ctx->maxIntegrationLine   = pOV50a40Ctx->curFrameLengthLines - 8;
    pOV50a40Ctx->aecMaxIntegrationTime = pOV50a40Ctx->maxIntegrationLine * pOV50a40Ctx->oneLineExpTime;

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    const OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    pIspStatus->useSensorAWB = false;

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT OV50a40_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV50a40Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = OV50a40_IsiWriteRegIss(handle, 0x50c1, 0x00);
    } else {
        result = OV50a40_IsiWriteRegIss(handle, 0x50c1, 0x01);
    }

    pOV50a40Ctx->testPattern = tpg.enable;

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(OV50a40_INFO, "%s: (enter)\n", __func__);

    OV50a40_Context_t *pOV50a40Ctx = (OV50a40_Context_t *) handle;
    if (pOV50a40Ctx == NULL || pOV50a40Ctx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV50a40Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!OV50a40_IsiReadRegIss(handle, 0x50c1,&value)) {
        pTpg->enable = ((value & 0x01) != 0) ? 1 : 0;
        if(pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pOV50a40Ctx->testPattern = pTpg->enable;
    }

    TRACE(OV50a40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV50a40_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "OV50a40";
    TRACE(OV50a40_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = OV50a40_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = OV50a40_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = OV50a40_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = OV50a40_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = OV50a40_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = OV50a40_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = OV50a40_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = OV50a40_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = OV50a40_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = OV50a40_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = OV50a40_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = OV50a40_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = OV50a40_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = OV50a40_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = OV50a40_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = OV50a40_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = OV50a40_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = OV50a40_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = OV50a40_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = OV50a40_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = OV50a40_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = OV50a40_IsiGetIspStatusIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = OV50a40_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = OV50a40_IsiGetTpgIss;

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

    TRACE(OV50a40_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t OV50a40_IsiCamDrvConfig = {
    .cameraDriverID      = 0x5650,
    .pIsiGetSensorIss    = OV50a40_IsiGetSensorIss,
};
