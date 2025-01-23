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
#include "OS04a10_priv.h"

CREATE_TRACER(OS04a10_INFO , "OS04a10: ", INFO,    1);
CREATE_TRACER(OS04a10_WARN , "OS04a10: ", WARNING, 1);
CREATE_TRACER(OS04a10_ERROR, "OS04a10: ", ERROR,   1);
CREATE_TRACER(OS04a10_DEBUG,     "OS04a10: ", INFO, 0);
CREATE_TRACER(OS04a10_REG_INFO , "OS04a10: ", INFO, 1);
CREATE_TRACER(OS04a10_REG_DEBUG, "OS04a10: ", INFO, 1);

#define OS04a10_MIN_GAIN_STEP    (1.0f/16.0f)  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain) */

/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pos04a10_mode_info[] = {
    {
        .index     = 0,
        .size      = {
            .boundsWidth  = 2688,
            .boundsHeight = 1520,
            .top           = 0,
            .left          = 0,
            .width         = 2688,
            .height        = 1520,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps       = 12 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 12,
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
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps       = 22 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 12,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 2,
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
        .fps       = 13.6 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_HDR_NATIVE,
        .nativeMode = ISI_SENSOR_NATIVE_DCG,
        .bitWidth = 16,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
    {
        .index     = 3,
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
        .fps       = 25 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_HDR_NATIVE,
        .nativeMode = ISI_SENSOR_NATIVE_DCG,
        .bitWidth = 12,
        .compress.enable = 1,
        .compress.xBit  = 16,
        .compress.yBit  = 12,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
};

static RESULT OS04a10_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s (enter)\n", __func__);

    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL || pOS04a10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pOS04a10Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(OS04a10_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OS04a10_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OS04a10_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s (enter)\n", __func__);

    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL || pOS04a10Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(OS04a10_ERROR, "%s: NULL POINTER!\n", __func__);
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pOS04a10Ctx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(OS04a10_ERROR, "%s: hal write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OS04a10_INFO, "%s addr 0x%04x, value 0x%02x (exit) result = %d\n", __func__, addr, value, result);
    return (result);
}

static RESULT OS04a10_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(OS04a10_INFO, "%s (enter)\n", __func__);
    const OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pOS04a10Ctx->sensorMode), sizeof(pOS04a10Ctx->sensorMode));

    TRACE(OS04a10_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT OS04a10_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(OS04a10_INFO, "%s (enter)\n", __func__);
    const OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pos04a10_mode_info)/sizeof(pos04a10_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pos04a10_mode_info)/sizeof(pos04a10_mode_info[0])); i++) {
        if (pos04a10_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pos04a10_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(OS04a10_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT OS04a10_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(OS04a10_INFO, "%s (enter)\n", __func__);

    if (pOS04a10Ctx == NULL) return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pOS04a10Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pOS04a10Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pOS04a10Ctx->sensorMode.size.width;
    pCaps->resolution.height = pOS04a10Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else if (pCaps->bitWidth == 16) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_16;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(OS04a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS04a10_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s (enter)\n", __func__);

    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) osMalloc(sizeof(OS04a10_Context_t));
    if (!pOS04a10Ctx) {
        TRACE(OS04a10_ERROR, "%s: Can't allocate os04a10 context\n",__func__);
        return (RET_OUTOFMEM);
    }
    MEMSET(pOS04a10Ctx, 0, sizeof(OS04a10_Context_t));

    pOS04a10Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pOS04a10Ctx->groupHold          = BOOL_FALSE;
    pOS04a10Ctx->oldGain            = 0;
    pOS04a10Ctx->oldIntegrationTime = 0;
    pOS04a10Ctx->configured         = BOOL_FALSE;
    pOS04a10Ctx->streaming          = BOOL_FALSE;
    pOS04a10Ctx->testPattern        = BOOL_FALSE;
    pOS04a10Ctx->isAfpsRun          = BOOL_FALSE;
    pOS04a10Ctx->sensorMode.index   = 0;

    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x6c;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pOS04a10Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pOS04a10Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(OS04a10_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pOS04a10Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        osFree(pOS04a10Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pOS04a10Ctx;

    TRACE(OS04a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS04a10_AecSetModeParameters(IsiSensorHandle_t handle, OS04a10_Context_t * pOS04a10Ctx)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0, regVal = 0;
    float32_t again = 0, dgain = 0;
    TRACE(OS04a10_INFO, "%s%s: (enter)\n", __func__, pOS04a10Ctx->isAfpsRun ? "(AFPS)" : "");

    pOS04a10Ctx->aecIntegrationTimeIncrement = pOS04a10Ctx->oneLineExpTime;
    pOS04a10Ctx->aecMinIntegrationTime       = pOS04a10Ctx->oneLineExpTime * pOS04a10Ctx->minIntegrationLine;
    pOS04a10Ctx->aecMaxIntegrationTime       = pOS04a10Ctx->oneLineExpTime * pOS04a10Ctx->maxIntegrationLine;

    TRACE(OS04a10_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pOS04a10Ctx->isAfpsRun ? "(AFPS)" : "",pOS04a10Ctx->aecMaxIntegrationTime);

    pOS04a10Ctx->aecGainIncrement = OS04a10_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pOS04a10Ctx->oldGain               = 0;
    pOS04a10Ctx->oldIntegrationTime    = 0;

    //HCG linear, DCG based on HCG
    OS04a10_IsiReadRegIss(handle, 0x3501, &value);
    regVal = value << 8;
    OS04a10_IsiReadRegIss(handle, 0x3502, &value);
    regVal = regVal | (value & 0xff);
    pOS04a10Ctx->aecCurIntegrationTime = regVal * pOS04a10Ctx->oneLineExpTime;
    pOS04a10Ctx->aecCurLongIntegrationTime = regVal * pOS04a10Ctx->oneLineExpTime;

    OS04a10_IsiReadRegIss(handle, 0x3508, &value);
    regVal = (value & 0x1f) << 4;
    OS04a10_IsiReadRegIss(handle, 0x3509, &value);
    regVal = regVal | ((value & 0xF0) >> 4);
    again = regVal/16.0f;
    
    OS04a10_IsiReadRegIss(handle, 0x350a, &value);
    regVal = (value & 0x0f) << 10;
    OS04a10_IsiReadRegIss(handle, 0x350b, &value);
    regVal = regVal | ((value & 0xff) << 2);
    OS04a10_IsiReadRegIss(handle, 0x350c, &value);
    regVal = regVal | ((value & 0xc0) >> 6);
    dgain = regVal/1024.0f;
    pOS04a10Ctx->aecCurGain            = again * dgain;
    pOS04a10Ctx->aecCurLongGain        = again * dgain;

    TRACE(OS04a10_INFO, "%s%s: (exit)\n", __func__,pOS04a10Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT OS04a10_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OS04a10_INFO, "%s (enter)\n", __func__);

    if (!pOS04a10Ctx) {
        TRACE(OS04a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOS04a10Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pOS04a10Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pos04a10_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (pos04a10_mode_info[i].index == pOS04a10Ctx->sensorMode.index) {
            SensorDefaultMode = &(pos04a10_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(OS04a10_mipi4lane_2688_1520_init) / sizeof(OS04a10_mipi4lane_2688_1520_init[0]); i++) {
                    OS04a10_IsiWriteRegIss(handle, OS04a10_mipi4lane_2688_1520_init[i][0], OS04a10_mipi4lane_2688_1520_init[i][1]);
                }
                break;
            case 1:
                for (int i = 0; i<sizeof(OS04a10_mipi4lane_1080p_init) / sizeof(OS04a10_mipi4lane_1080p_init[0]); i++) {
                    OS04a10_IsiWriteRegIss(handle, OS04a10_mipi4lane_1080p_init[i][0], OS04a10_mipi4lane_1080p_init[i][1]);
                }
                break;
            case 2:
                for (int i = 0; i<sizeof(OS04a10_mipi4lane_1080p_DCG_combine_16b_init) / sizeof(OS04a10_mipi4lane_1080p_DCG_combine_16b_init[0]); i++) {
                    OS04a10_IsiWriteRegIss(handle, OS04a10_mipi4lane_1080p_DCG_combine_16b_init[i][0], OS04a10_mipi4lane_1080p_DCG_combine_16b_init[i][1]);
                }
                break;
            case 3:
                for (int i = 0; i<sizeof(OS04a10_mipi4lane_1080p_DCG_combine_12b_init) / sizeof(OS04a10_mipi4lane_1080p_DCG_combine_12b_init[0]); i++) {
                    OS04a10_IsiWriteRegIss(handle, OS04a10_mipi4lane_1080p_DCG_combine_12b_init[i][0], OS04a10_mipi4lane_1080p_DCG_combine_12b_init[i][1]);
                }
                break;
            default:
                TRACE(OS04a10_INFO, "%s:not support sensor mode %d\n", __func__,pOS04a10Ctx->sensorMode.index);
                return RET_NOTSUPP;
                break;
    }

        memcpy(&(pOS04a10Ctx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));

    } else {
        TRACE(OS04a10_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pOS04a10Ctx->sensorMode.index) {
        case 0:
            pOS04a10Ctx->oneLineExpTime      = 0.000051;
            pOS04a10Ctx->frameLengthLines    = 0x658;
            pOS04a10Ctx->curFrameLengthLines = pOS04a10Ctx->frameLengthLines;
            pOS04a10Ctx->maxIntegrationLine  = pOS04a10Ctx->curFrameLengthLines -8 ;
            pOS04a10Ctx->minIntegrationLine  = 2;
            pOS04a10Ctx->aecMaxGain          = 255.6801;
            pOS04a10Ctx->aecMinGain          = 1.0;
            pOS04a10Ctx->aGain.min           = 1.0;
            pOS04a10Ctx->aGain.max           = 15.99;
            pOS04a10Ctx->aGain.step          = (1.0f/16.0f);
            pOS04a10Ctx->dGain.min           = 1.0;
            pOS04a10Ctx->dGain.max           = 15.99;
            pOS04a10Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 1:
            pOS04a10Ctx->oneLineExpTime      = 0.000034;
            pOS04a10Ctx->frameLengthLines    = 0x54b;
            pOS04a10Ctx->curFrameLengthLines = pOS04a10Ctx->frameLengthLines;
            pOS04a10Ctx->maxIntegrationLine  = pOS04a10Ctx->curFrameLengthLines -8 ;
            pOS04a10Ctx->minIntegrationLine  = 2;
            pOS04a10Ctx->aecMaxGain          = 255.6801;
            pOS04a10Ctx->aecMinGain          = 1.0;
            pOS04a10Ctx->aGain.min           = 1.0;
            pOS04a10Ctx->aGain.max           = 15.99;
            pOS04a10Ctx->aGain.step          = (1.0f/16.0f);
            pOS04a10Ctx->dGain.min           = 1.0;
            pOS04a10Ctx->dGain.max           = 15.99;
            pOS04a10Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 2:
            pOS04a10Ctx->oneLineExpTime      = 0.00006;
            pOS04a10Ctx->frameLengthLines    = 0x4c3;
            pOS04a10Ctx->curFrameLengthLines = pOS04a10Ctx->frameLengthLines;
            pOS04a10Ctx->maxIntegrationLine  = pOS04a10Ctx->curFrameLengthLines -8 ;
            pOS04a10Ctx->minIntegrationLine  = 2;
            pOS04a10Ctx->aecMaxGain          = 255.6801;
            pOS04a10Ctx->aecMinGain          = 1.0;
            pOS04a10Ctx->aGain.min           = 1.0;
            pOS04a10Ctx->aGain.max           = 15.99;
            pOS04a10Ctx->aGain.step          = (1.0f/16.0f);
            pOS04a10Ctx->dGain.min           = 1.0;
            pOS04a10Ctx->dGain.max           = 15.99;
            pOS04a10Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        case 3:
            pOS04a10Ctx->oneLineExpTime      = 0.000034;
            pOS04a10Ctx->frameLengthLines    = 0x4ab;
            pOS04a10Ctx->curFrameLengthLines = pOS04a10Ctx->frameLengthLines;
            pOS04a10Ctx->maxIntegrationLine  = pOS04a10Ctx->curFrameLengthLines -8 ;
            pOS04a10Ctx->minIntegrationLine  = 2;
            pOS04a10Ctx->aecMaxGain          = 255.6801;
            pOS04a10Ctx->aecMinGain          = 1.0;
            pOS04a10Ctx->aGain.min           = 1.0;
            pOS04a10Ctx->aGain.max           = 15.99;
            pOS04a10Ctx->aGain.step          = (1.0f/16.0f);
            pOS04a10Ctx->dGain.min           = 1.0;
            pOS04a10Ctx->dGain.max           = 15.99;
            pOS04a10Ctx->dGain.step          = (1.0f/1024.0f);
            break;
        default:
            TRACE(OS04a10_INFO, "%s:not support sensor mode %d\n", __func__,pOS04a10Ctx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    pOS04a10Ctx->maxFps  = pOS04a10Ctx->sensorMode.fps;
    pOS04a10Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pOS04a10Ctx->currFps = pOS04a10Ctx->maxFps;

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(OS04a10_DEBUG, "%s: OS04a10 System-Reset executed\n", __func__);
    osSleep(100);

    result = OS04a10_AecSetModeParameters(handle, pOS04a10Ctx);
    if (result != RET_SUCCESS) {
        TRACE(OS04a10_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pOS04a10Ctx->configured = BOOL_TRUE;
    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT OS04a10_IsiCloseIss(IsiSensorHandle_t handle)
{
    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s (enter)\n", __func__);

    if (pOS04a10Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)OS04a10_IsiSetStreamingIss(pOS04a10Ctx, BOOL_FALSE);

    TRACE(OS04a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS04a10_IsiReleaseIss(IsiSensorHandle_t handle)
{
    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s (enter)\n", __func__);

    if (pOS04a10Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pOS04a10Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pOS04a10Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pOS04a10Ctx, 0, sizeof(OS04a10_Context_t));
    osFree(pOS04a10Ctx);
    TRACE(OS04a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS04a10_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;
    uint32_t sensorId = 0;
    uint32_t correctId = 0x5304;

    TRACE(OS04a10_INFO, "%s (enter)\n", __func__);

    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL || pOS04a10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = OS04a10_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(OS04a10_ERROR, "%s: Read Sensor ID Error! \n",__func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(OS04a10_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n", __func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(OS04a10_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__, correctId, sensorId);
    TRACE(OS04a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS04a10_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(OS04a10_INFO, "%s (enter)\n", __func__);

    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL || pOS04a10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (!pValue) return (RET_NULL_POINTER);

    regVal   = 0;
    result   = OS04a10_IsiReadRegIss(handle, 0x300a, &regVal);
    sensorId = (regVal & 0xff) << 8;

    regVal   = 0;
    result   |= OS04a10_IsiReadRegIss(handle, 0x300b, &regVal);
    sensorId |= (regVal & 0xff);

    *pValue = sensorId;
    TRACE(OS04a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS04a10_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s (enter)\n", __func__);

    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL || pOS04a10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOS04a10Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    if(on){
        result = OS04a10_IsiWriteRegIss(handle, 0x0100, 1);
        if (result != RET_SUCCESS) {
            TRACE(OS04a10_ERROR, "%s: set sensor streaming on error! \n",__func__);
            return (RET_FAILURE);
        }
    }

    if(!on){
        result = OS04a10_IsiWriteRegIss(handle, 0x0100, 0);
        if (result != RET_SUCCESS) {
            TRACE(OS04a10_ERROR, "%s: set sensor streaming off error! \n",__func__);
            return (RET_FAILURE);
        }
    }

    pOS04a10Ctx->streaming = on;

    TRACE(OS04a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OS04a10_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    const OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);

    if (pOS04a10Ctx == NULL) {
        TRACE(OS04a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(OS04a10_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    if (pOS04a10Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        /* linear mode*/
        pAeBaseInfo->gain.min        = pOS04a10Ctx->aecMinGain;//total gain
        pAeBaseInfo->gain.max        = pOS04a10Ctx->aecMaxGain;
        pAeBaseInfo->intTime.min     = pOS04a10Ctx->aecMinIntegrationTime;
        pAeBaseInfo->intTime.max     = pOS04a10Ctx->aecMaxIntegrationTime;

        pAeBaseInfo->aGain           = pOS04a10Ctx->aGain;//min, max, step
        pAeBaseInfo->dGain           = pOS04a10Ctx->dGain;

        pAeBaseInfo->aecCurGain      = pOS04a10Ctx->aecCurGain;
        pAeBaseInfo->aecCurIntTime   = pOS04a10Ctx->aecCurIntegrationTime;

        pAeBaseInfo->aecGainStep     = pOS04a10Ctx->aecGainIncrement;
        pAeBaseInfo->aecIntTimeStep  = pOS04a10Ctx->aecIntegrationTimeIncrement;
    } else if (pOS04a10Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        //based on HCG
        //get time limit and total gain limit
        pAeBaseInfo->longGain.min        = pOS04a10Ctx->aecMinGain;
        pAeBaseInfo->longGain.max        = pOS04a10Ctx->aecMaxGain;
        pAeBaseInfo->longIntTime.min     = pOS04a10Ctx->aecMinIntegrationTime;
        pAeBaseInfo->longIntTime.max     = pOS04a10Ctx->aecMaxIntegrationTime;

        //get again/dgain min/max/step info
        pAeBaseInfo->aLongGain           = pOS04a10Ctx->aGain;
        pAeBaseInfo->dLongGain           = pOS04a10Ctx->dGain;

        pAeBaseInfo->aecCurGain      = pOS04a10Ctx->aecCurGain;

        //get current intTime and gain
        pAeBaseInfo->curGain.gain[0]       = pOS04a10Ctx->aecCurLongGain;
        pAeBaseInfo->curGain.gain[1]       = pOS04a10Ctx->aecCurGain;

        pAeBaseInfo->curIntTime.intTime[0] = pOS04a10Ctx->aecCurLongIntegrationTime;
        pAeBaseInfo->curIntTime.intTime[1] = pOS04a10Ctx->aecCurIntegrationTime;

        pAeBaseInfo->aecGainStep     = pOS04a10Ctx->aecGainIncrement;
        pAeBaseInfo->aecIntTimeStep  = pOS04a10Ctx->aecIntegrationTimeIncrement;
        pAeBaseInfo->stitchingMode   = pOS04a10Ctx->sensorMode.stitchingMode;
        pAeBaseInfo->nativeMode      = pOS04a10Ctx->sensorMode.nativeMode;
        pAeBaseInfo->nativeHdrRatio[0] = 1.0;

        pAeBaseInfo->conversionGainDCG = 1.0;
    } else {
        TRACE(OS04a10_INFO, "%s:not support this mode!\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS04a10_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);
    uint32_t again = 0;

    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL || pOS04a10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOS04a10Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        again = (uint32_t)(pSensorAGain->gain[ISI_LINEAR_PARAS] * 16);
        result = OS04a10_IsiWriteRegIss(handle, 0x3508, (again >> 4) & 0x1f);
        result |= OS04a10_IsiWriteRegIss(handle, 0x3509, (again & 0x0f) << 4 );
        pOS04a10Ctx->curAgain = again/16.0f;
    } else if (pOS04a10Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {

        pOS04a10Ctx->hcgAgain = (pSensorAGain->gain[ISI_LINEAR_PARAS] * 5);//make hcg agin is high than lcg again
        pOS04a10Ctx->lcgAgain = (pSensorAGain->gain[ISI_LINEAR_PARAS]);

        //set HCG again
        if(pOS04a10Ctx->hcgAgain > pOS04a10Ctx->aGain.max){
            pOS04a10Ctx->hcgAgain = pOS04a10Ctx->aGain.max;
        }
        again = (uint32_t)(pOS04a10Ctx->hcgAgain * 16);
        result = OS04a10_IsiWriteRegIss(handle, 0x3508, (again >> 4) & 0x1f);
        result |= OS04a10_IsiWriteRegIss(handle, 0x3509, (again & 0x0f) << 4 );

        //set LCG again
        if(pOS04a10Ctx->lcgAgain > pOS04a10Ctx->aGain.max){
            pOS04a10Ctx->lcgAgain = pOS04a10Ctx->aGain.max;
        }
        again = (uint32_t)(pOS04a10Ctx->lcgAgain * 16);
        result |= OS04a10_IsiWriteRegIss(handle, 0x3548, (again >> 4) & 0x1f);
        result |= OS04a10_IsiWriteRegIss(handle, 0x3549, (again & 0x0f) << 4 );

        pOS04a10Ctx->curAgain = pSensorAGain->gain[ISI_LINEAR_PARAS];
    } else {
        TRACE(OS04a10_INFO, "%s:not support this mode!\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS04a10_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);
    uint32_t dgain = 0;

    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL || pOS04a10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOS04a10Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        dgain = (uint32_t)(pSensorDGain->gain[ISI_LINEAR_PARAS] * 1024);
        result = OS04a10_IsiWriteRegIss(handle, 0x350a, (dgain >> 10) & 0x0f);
        result |= OS04a10_IsiWriteRegIss(handle,0x350b, (dgain >> 2) & 0xff);
        result |= OS04a10_IsiWriteRegIss(handle,0x350c, (dgain & 0x03) << 6);
        pOS04a10Ctx->curDgain = dgain/1024.0f;
        pOS04a10Ctx->aecCurGain = pOS04a10Ctx->curAgain * pOS04a10Ctx->curDgain;
    } else if (pOS04a10Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {

        pOS04a10Ctx->hcgDgain = (pSensorDGain->gain[ISI_LINEAR_PARAS] * 5);//make hcg dgin is high than lcg dgain
        pOS04a10Ctx->lcgDgain = (pSensorDGain->gain[ISI_LINEAR_PARAS]);

        //set HCG dgain
        if(pOS04a10Ctx->hcgDgain > pOS04a10Ctx->dGain.max) {
            pOS04a10Ctx->hcgDgain = pOS04a10Ctx->dGain.max;
        }
        dgain = (uint32_t)(pOS04a10Ctx->hcgDgain * 1024);
        result = OS04a10_IsiWriteRegIss(handle, 0x350a, (dgain >> 10) & 0x0f);
        result |= OS04a10_IsiWriteRegIss(handle,0x350b, (dgain >> 2) & 0xff);
        result |= OS04a10_IsiWriteRegIss(handle,0x350c, (dgain & 0x03) << 6);

        //set LCG dgain
        if(pOS04a10Ctx->lcgDgain > pOS04a10Ctx->dGain.max) {
            pOS04a10Ctx->lcgDgain = pOS04a10Ctx->dGain.max;
        }
        dgain = (uint32_t)(pOS04a10Ctx->lcgDgain * 1024);
        result |= OS04a10_IsiWriteRegIss(handle, 0x354a, (dgain >> 10) & 0x0f);
        result |= OS04a10_IsiWriteRegIss(handle,0x354b, (dgain >> 2) & 0xff);
        result |= OS04a10_IsiWriteRegIss(handle,0x354c, (dgain & 0x03) << 6);

        pOS04a10Ctx->curDgain = dgain/1024.0f;
        pOS04a10Ctx->aecCurLongGain = pOS04a10Ctx->curAgain * pOS04a10Ctx->curDgain;
        pOS04a10Ctx->aecCurGain = pOS04a10Ctx->curAgain * pOS04a10Ctx->curDgain;
    } else {
        TRACE(OS04a10_INFO, "%s:not support this mode!\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS04a10_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    const OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);

    if (pOS04a10Ctx == NULL) {
        TRACE(OS04a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorAGain->gain[ISI_LINEAR_PARAS]       = pOS04a10Ctx->curAgain;

    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS04a10_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    const OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);

    if (pOS04a10Ctx == NULL) {
        TRACE(OS04a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorDGain->gain[ISI_LINEAR_PARAS] = pOS04a10Ctx->curDgain;


    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS04a10_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);
    uint16_t expLine = 0;
    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;

    if (!pOS04a10Ctx) {
        TRACE(OS04a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = pSensorIntTime->intTime[ISI_LINEAR_PARAS] / pOS04a10Ctx->oneLineExpTime;
    expLine = MIN(pOS04a10Ctx->maxIntegrationLine, MAX(pOS04a10Ctx->minIntegrationLine, expLine));
    TRACE(OS04a10_DEBUG, "%s: set expLine = 0x%04x\n", __func__, expLine);

    result =  OS04a10_IsiWriteRegIss(handle, 0x3501, (expLine >> 8) & 0xff);
    result |= OS04a10_IsiWriteRegIss(handle, 0x3502, expLine & 0xff);

    pOS04a10Ctx->aecCurIntegrationTime = expLine * pOS04a10Ctx->oneLineExpTime;
    pOS04a10Ctx->aecCurLongIntegrationTime = expLine * pOS04a10Ctx->oneLineExpTime;


    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS04a10_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    const OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);

    if (!pOS04a10Ctx) {
        TRACE(OS04a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    pSensorIntTime->intTime[ISI_LINEAR_PARAS] = pOS04a10Ctx->aecCurIntegrationTime;

    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS04a10_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t * pFps)
{
    const OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);

    if (pOS04a10Ctx == NULL) {
        TRACE(OS04a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pOS04a10Ctx->currFps;

    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS04a10_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    RESULT result = RET_SUCCESS;
    int32_t newVts = 0;

    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);

    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL) {
        TRACE(OS04a10_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

   if (fps > pOS04a10Ctx->maxFps) {
        TRACE(OS04a10_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, fps, pOS04a10Ctx->maxFps, pOS04a10Ctx->minFps,pOS04a10Ctx->maxFps);
        fps = pOS04a10Ctx->maxFps;
    }
    if (fps < pOS04a10Ctx->minFps) {
        TRACE(OS04a10_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, fps, pOS04a10Ctx->minFps, pOS04a10Ctx->minFps,pOS04a10Ctx->maxFps);
        fps = pOS04a10Ctx->minFps;
    }
     
    newVts = pOS04a10Ctx->frameLengthLines*pOS04a10Ctx->sensorMode.fps / fps;
    result =  OS04a10_IsiWriteRegIss(handle, 0x380e, newVts >> 8);
    result |=  OS04a10_IsiWriteRegIss(handle, 0x380f, newVts & 0xff);
    pOS04a10Ctx->currFps              = fps;
    pOS04a10Ctx->curFrameLengthLines  = newVts;
    pOS04a10Ctx->maxIntegrationLine   = pOS04a10Ctx->curFrameLengthLines - 8;
    pOS04a10Ctx->aecMaxIntegrationTime = pOS04a10Ctx->maxIntegrationLine * pOS04a10Ctx->oneLineExpTime;

    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS04a10_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    const OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    pIspStatus->useSensorAWB = false;

    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

static RESULT OS04a10_IsiSetWBIss(IsiSensorHandle_t handle, const IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);
    uint16_t rGain, grGain, gbGain, bGain;

    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL || pOS04a10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)  return RET_NULL_POINTER;

    TRACE(OS04a10_INFO, "%s: wb info: pWb->rGain = %f, pWb->grGain = %f, pWb->gbGain = %f, pWb->bGain = %f.\n", 
        __func__, pWb->rGain, pWb->grGain, pWb->gbGain, pWb->bGain);
    //os04a10 awb gain, 0x400 means gain 1.0x
    bGain  = (uint16_t)(pWb->bGain*1024);
    gbGain = (uint16_t)(pWb->gbGain*1024);
    grGain = (uint16_t)(pWb->grGain*1024);
    rGain  = (uint16_t)(pWb->rGain*1024);

    //set HCG wb gain
    result = OS04a10_IsiWriteRegIss(handle, 0x5180, (bGain >> 8) & 0x7f);
    result |= OS04a10_IsiWriteRegIss(handle, 0x5181, bGain & 0xff);
    result |= OS04a10_IsiWriteRegIss(handle, 0x5182, (gbGain >> 8) & 0x7f);
    result |= OS04a10_IsiWriteRegIss(handle, 0x5183, gbGain & 0xff);
    result |= OS04a10_IsiWriteRegIss(handle, 0x5184, (grGain >> 8) & 0x7f);
    result |= OS04a10_IsiWriteRegIss(handle, 0x5185, grGain & 0xff);
    result |= OS04a10_IsiWriteRegIss(handle, 0x5186, (rGain >> 8) & 0x7f);
    result |= OS04a10_IsiWriteRegIss(handle, 0x5187, rGain & 0xff);

    //set LCG wb gain
    result = OS04a10_IsiWriteRegIss(handle, 0x51a0, (bGain >> 8) & 0x7f);
    result |= OS04a10_IsiWriteRegIss(handle, 0x51a1, bGain & 0xff);
    result |= OS04a10_IsiWriteRegIss(handle, 0x51a2, (gbGain >> 8) & 0x7f);
    result |= OS04a10_IsiWriteRegIss(handle, 0x51a3, gbGain & 0xff);
    result |= OS04a10_IsiWriteRegIss(handle, 0x51a4, (grGain >> 8) & 0x7f);
    result |= OS04a10_IsiWriteRegIss(handle, 0x51a5, grGain & 0xff);
    result |= OS04a10_IsiWriteRegIss(handle, 0x51a6, (rGain >> 8) & 0x7f);
    result |= OS04a10_IsiWriteRegIss(handle, 0x51a7, rGain & 0xff);

    pOS04a10Ctx->sensorWb.bGain  = bGain / 1024.0;
    pOS04a10Ctx->sensorWb.gbGain = gbGain / 1024.0;
    pOS04a10Ctx->sensorWb.grGain = grGain / 1024.0;
    pOS04a10Ctx->sensorWb.rGain  = rGain / 1024.0;

    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OS04a10_IsiGetWBIss(IsiSensorHandle_t handle, IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);

    const OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)  return RET_NULL_POINTER;

    memcpy(pWb, &pOS04a10Ctx->sensorWb, sizeof(IsiSensorWb_t));

    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OS04a10_IsiSetBlcIss(IsiSensorHandle_t handle, const IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);

    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)  return RET_NULL_POINTER;

    TRACE(OS04a10_INFO, "%s: bls info: pBlc->red = %d. \n", __func__, pBlc->red);
    result = OS04a10_IsiWriteRegIss(handle, 0x4004, (pBlc->red >> 4) & 0xf);
    result |= OS04a10_IsiWriteRegIss(handle, 0x4005, (pBlc->red << 4) & 0xff);//HCG BLC
    result |= OS04a10_IsiWriteRegIss(handle, 0x402e, (pBlc->red >> 4) & 0xf);
    result |= OS04a10_IsiWriteRegIss(handle, 0x402f, (pBlc->red << 4) & 0xff);//LCG BLC

    memcpy(&pOS04a10Ctx->sensorBlc, pBlc, sizeof(IsiSensorBlc_t));

    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OS04a10_IsiGetBlcIss(IsiSensorHandle_t handle, IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);
    
    const OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)  return RET_NULL_POINTER;

    memcpy(pBlc, &pOS04a10Ctx->sensorBlc, sizeof(IsiSensorBlc_t));

    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS04a10_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);

    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL || pOS04a10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOS04a10Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = OS04a10_IsiWriteRegIss(handle, 0x5080, 0x00);
    } else {
        result = OS04a10_IsiWriteRegIss(handle, 0x5080, 0x80);
    }

    pOS04a10Ctx->testPattern = tpg.enable;

    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS04a10_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);

    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL || pOS04a10Ctx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOS04a10Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!OS04a10_IsiReadRegIss(handle, 0x5080, &value)) {
        pTpg->enable = ((value & 0x80) != 0) ? 1 : 0;
        if(pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pOS04a10Ctx->testPattern = pTpg->enable;
    }

    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OS04a10_IsiGetExpandCurveIss(IsiSensorHandle_t handle, IsiSensorCompandCurve_t *pCurve)
{
    RESULT result = RET_SUCCESS;
    TRACE(OS04a10_INFO, "%s: (enter)\n", __func__);

    OS04a10_Context_t *pOS04a10Ctx = (OS04a10_Context_t *) handle;
    if (pOS04a10Ctx == NULL || pOS04a10Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    uint8_t expandPx[64] = {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6};
    memcpy(pCurve->compandPx, expandPx, sizeof(expandPx));

    pCurve->compandXData[0] = 0;
    pCurve->compandYData[0] = 0;
    for(int i=1; i<65; i++){
        pCurve->compandXData[i] = (1 << pCurve->compandPx[i-1]) + pCurve->compandXData[i-1];

        if(pCurve->compandXData[i] < 1024){
            pCurve->compandYData[i] = pCurve->compandXData[i];
        } else if (pCurve->compandXData[i] < 1280){
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 768) << 2;//*4
        } else if (pCurve->compandXData[i] < 2048){
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 1024) << 3;//*8
        } else if (pCurve->compandXData[i] < 3584){
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 1536) << 4;//*16
        } else {
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 3072) << 6;//*64
        }
    }

    TRACE(OS04a10_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OS04a10_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "OS04a10";
    TRACE(OS04a10_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = OS04a10_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = OS04a10_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = OS04a10_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = OS04a10_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = OS04a10_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = OS04a10_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = OS04a10_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = OS04a10_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = OS04a10_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = OS04a10_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = OS04a10_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = OS04a10_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = OS04a10_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = OS04a10_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = OS04a10_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = OS04a10_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = OS04a10_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = OS04a10_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = OS04a10_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = OS04a10_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = OS04a10_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = OS04a10_IsiGetIspStatusIss;
        pIsiSensor->pIsiSetWBIss                        = OS04a10_IsiSetWBIss;
        pIsiSensor->pIsiGetWBIss                        = OS04a10_IsiGetWBIss;
        pIsiSensor->pIsiSetBlcIss                       = OS04a10_IsiSetBlcIss;
        pIsiSensor->pIsiGetBlcIss                       = OS04a10_IsiGetBlcIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = OS04a10_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = OS04a10_IsiGetTpgIss;
        pIsiSensor->pIsiGetExpandCurveIss               = OS04a10_IsiGetExpandCurveIss;

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

    TRACE(OS04a10_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t OS04a10_IsiCamDrvConfig = {
    .cameraDriverID      = 0x5304,
    .pIsiGetSensorIss    = OS04a10_IsiGetSensorIss,
};
