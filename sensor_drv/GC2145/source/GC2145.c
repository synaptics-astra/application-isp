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
#include "isi_priv.h"
#include "GC2145_priv.h"

CREATE_TRACER(GC2145_INFO , "GC2145: ", INFO,    1);
CREATE_TRACER(GC2145_WARN , "GC2145: ", WARNING, 1);
CREATE_TRACER(GC2145_ERROR, "GC2145: ", ERROR,   1);
CREATE_TRACER(GC2145_DEBUG,     "GC2145: ", INFO, 1);
CREATE_TRACER(GC2145_REG_INFO , "GC2145: ", INFO, 1);
CREATE_TRACER(GC2145_REG_DEBUG, "GC2145: ", INFO, 1);

#define GC2145_AGAIN_ACCURACY    16.0f
#define GC2145_DGAIN_ACCURACY    64.0f

/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pGC2145_mode_info[] = {
    {
        .index     = 0,
        .size      = {
            .boundsWidth  = 1600,
            .boundsHeight = 1200,
            .top           = 0,
            .left          = 0,
            .width         = 1600,
            .height        = 1200,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 1,
            .gainDelayFrame = 1,
        },
        .fps       = 15 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 8,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
        .dataType = ISI_MODE_YUV
    },

};

static RESULT GC2145_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t Addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;

    TRACE(GC2145_INFO, "%s (enter)\n", __func__);

    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    if (pGC2145Ctx == NULL || pGC2145Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pGC2145Ctx->isiCtx.halI2cHandle, Addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(GC2145_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(GC2145_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT GC2145_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t Addr, const uint16_t Value)
{
    RESULT result = RET_SUCCESS;
    TRACE(GC2145_INFO, "%s (enter)\n", __func__);

    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    if (pGC2145Ctx == NULL || pGC2145Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(GC2145_ERROR, "%s: NULL POINTER!\n", __func__);
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pGC2145Ctx->isiCtx.halI2cHandle, Addr, Value);
    if (result != RET_SUCCESS) {
        TRACE(GC2145_ERROR, "%s: hal write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(GC2145_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT GC2145_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(GC2145_INFO, "%s (enter)\n", __func__);
    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    if (pGC2145Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pGC2145Ctx->sensorMode), sizeof(pGC2145Ctx->sensorMode));

    TRACE(GC2145_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT GC2145_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(GC2145_INFO, "%s (enter)\n", __func__);
    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    if (pGC2145Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pGC2145_mode_info)/sizeof(pGC2145_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pGC2145_mode_info)/sizeof(pGC2145_mode_info[0])); i++) {
        if (pGC2145_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pGC2145_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(GC2145_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT GC2145_ISiSetPowerIss(IsiSensorHandle_t handle, bool_t isEnable)
{
    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;

    RESULT result = RET_SUCCESS;
	HalDvpConfig_t dvpCfg;
	uint8_t dvpCtrlId = 0;
    TRACE(GC2145_INFO, "%s (enter)\n", __func__);

    if (pGC2145Ctx == NULL) return (RET_WRONG_HANDLE);

	dvpCfg.pwdnPol = ISI_SYNC_POL_LOW_ACIVE;
	dvpCfg.rstPol = ISI_SYNC_POL_HIGH_ACIVE;
	dvpCfg.fsinPol = ISI_SYNC_POL_HIGH_ACIVE; //not used

    HalI2cContext_t *pI2cCtx = (HalI2cContext_t *)pGC2145Ctx->isiCtx.halI2cHandle;
	if (pI2cCtx->devId == 8) {
		dvpCtrlId = 0;
	} else if (pI2cCtx->devId == 9) {
		dvpCtrlId = 1;
	}else {
		//;
	}
	result = HalDvpSetPower(dvpCtrlId,dvpCfg,isEnable);
	if (result != RET_SUCCESS) {
		TRACE(GC2145_ERROR, "%s: hal power sensor error!\n", __func__);
		return result;
	}

    TRACE(GC2145_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT GC2145_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(GC2145_INFO, "%s (enter)\n", __func__);

    if (pGC2145Ctx == NULL) return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pGC2145Ctx->sensorMode.bitWidth;
    pCaps->bayerPattern      = pGC2145Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pGC2145Ctx->sensorMode.size.width;
    pCaps->resolution.height = pGC2145Ctx->sensorMode.size.height;
    pCaps->mipiLanes        = ISI_MIPI_OFF;

    pCaps->hSyncPol = ISI_SYNC_POL_HIGH_ACIVE;
    pCaps->vSyncPol = ISI_SYNC_POL_LOW_ACIVE;
    pCaps->sampleEdge = ISI_SAMPLE_EDGE_POL_POSITIVE;

	if (pGC2145Ctx->sensorMode.dataType == ISI_MODE_YUV) {
		pCaps->mode  = ISI_MODE_YUV;
		pCaps->vinType = ISI_ITF_TYPE_BT601;
		pCaps->ccirSeq = ISI_SENSOR_CCIR_SEQUENCE_YCbYCr;
		pCaps->mipiMode = ISI_FORMAT_YUV422P_8; //use to store format

	} else if (pGC2145Ctx->sensorMode.dataType == ISI_MODE_BAYER){
		pCaps->mode  = ISI_MODE_BAYER;
		pCaps->vinType = ISI_ITF_TYPE_DVP;
		pCaps->mipiMode = ISI_FORMAT_RAW_8;
	}

    TRACE(GC2145_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT GC2145_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle)
{
    RESULT result = RET_SUCCESS;
    TRACE(GC2145_INFO, "%s (enter)\n", __func__);

    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) osMalloc(sizeof(GC2145_Context_t));
    if (!pGC2145Ctx) {
        TRACE(GC2145_ERROR, "%s: Can't allocate GC2145 context\n",__func__);
        return (RET_OUTOFMEM);
    }
    MEMSET(pGC2145Ctx, 0, sizeof(GC2145_Context_t));

    pGC2145Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pGC2145Ctx->configured         = BOOL_FALSE;
    pGC2145Ctx->streaming          = BOOL_FALSE;
    pGC2145Ctx->testPattern        = BOOL_FALSE;
    pGC2145Ctx->isAfpsRun          = BOOL_FALSE;
    pGC2145Ctx->sensorMode.index   = 0;

    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x78;
    sccbConfig.addrByte  = 1;
    sccbConfig.dataByte  = 1;

    pGC2145Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pGC2145Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(GC2145_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pGC2145Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        osFree(pGC2145Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pGC2145Ctx;

	result = GC2145_ISiSetPowerIss((IsiSensorHandle_t) pGC2145Ctx, BOOL_TRUE);

    TRACE(GC2145_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT GC2145_AecSetModeParameters(IsiSensorHandle_t handle, GC2145_Context_t * pGC2145Ctx)
{
    RESULT result = RET_SUCCESS;
    //uint16_t reg16 = 0, value = 0;
    TRACE(GC2145_INFO, "%s%s: (enter)\n", __func__, pGC2145Ctx->isAfpsRun ? "(AFPS)" : "");

    pGC2145Ctx->aecIntegrationTimeIncrement = pGC2145Ctx->oneLineExpTime;
    pGC2145Ctx->aecMinIntegrationTime       = pGC2145Ctx->oneLineExpTime * pGC2145Ctx->minIntegrationLine;
    pGC2145Ctx->aecMaxIntegrationTime       = pGC2145Ctx->oneLineExpTime * pGC2145Ctx->maxIntegrationLine;

    TRACE(GC2145_DEBUG, "%s%s: aecMaxIntegrationTime = %f \n", __func__, pGC2145Ctx->isAfpsRun ? "(AFPS)" : "",pGC2145Ctx->aecMaxIntegrationTime);

    pGC2145Ctx->aecGainIncrement = (1.0f/GC2145_AGAIN_ACCURACY);

    // //reflects the state of the sensor registers, must equal default settings
    // GC2145_IsiWriteRegIss(handle, 0xfd, 0x01);
    // //again
    // result = GC2145_IsiReadRegIss(handle, 0x24,&reg16);
    // pGC2145Ctx->curAgain = (float32_t)reg16/GC2145_AGAIN_ACCURACY;

    // //dgain
    // reg16 = 0;
    // GC2145_IsiReadRegIss(handle, 0x37, &reg16);
    // value = (reg16 & 0x7) << 8;
    // GC2145_IsiReadRegIss(handle, 0x39, &reg16);
    // value |= (reg16 & 0xFF);
    // pGC2145Ctx->curDgain = (float32_t)value/GC2145_DGAIN_ACCURACY;

    // //aecGain
    // pGC2145Ctx->aecCurGain = pGC2145Ctx->curAgain * pGC2145Ctx->curDgain;

    // //exposure time
    // value = 0;
    // reg16 = 0;
    // GC2145_IsiReadRegIss(handle, 0x03, &reg16);
    // value = (uint16_t)reg16 << 8;
    // GC2145_IsiReadRegIss(handle, 0x04, &reg16);
    // value |= (reg16 & 0xFF);
    // pGC2145Ctx->aecCurIntegrationTime = value * pGC2145Ctx->oneLineExpTime;

    TRACE(GC2145_INFO, "%s%s: (exit)\n", __func__,pGC2145Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT GC2145_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(GC2145_INFO, "%s (enter)\n", __func__);

    if (!pGC2145Ctx) {
        TRACE(GC2145_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pGC2145Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pGC2145Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pGC2145_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (pGC2145_mode_info[i].index == pGC2145Ctx->sensorMode.index) {
            SensorDefaultMode = &(pGC2145_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(GC2145_yuv8_1080p_init) / sizeof(GC2145_yuv8_1080p_init[0]); i++) {
                    GC2145_IsiWriteRegIss(handle, GC2145_yuv8_1080p_init[i][0], GC2145_yuv8_1080p_init[i][1]);
                }
                break;
            // case 1:
            //     for (int i = 0; i<sizeof(GC2145_raw10_640_480_init) / sizeof(GC2145_raw10_640_480_init[0]); i++) {
            //         GC2145_IsiWriteRegIss(handle, GC2145_raw10_640_480_init[i][0], GC2145_raw10_640_480_init[i][1]);
            //     }
            //     break;
            // case 2:
            //     for (int i = 0; i<sizeof(GC2145_raw10_1080p_30fps_init) / sizeof(GC2145_raw10_1080p_30fps_init[0]); i++) {
            //         GC2145_IsiWriteRegIss(handle, GC2145_raw10_1080p_30fps_init[i][0], GC2145_raw10_1080p_30fps_init[i][1]);
            //     }
            //     break;
            default:
                TRACE(GC2145_INFO, "%s:not support sensor mode %d\n", __func__,pGC2145Ctx->sensorMode.index);
                return RET_NOTSUPP;
                break;
    }

        memcpy(&(pGC2145Ctx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));

    } else {
        TRACE(GC2145_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pGC2145Ctx->sensorMode.index) {
        case 0:
            pGC2145Ctx->oneLineExpTime   = 0.00004508;
            pGC2145Ctx->frameLengthLines    = 0x5c7;//vblank 0x172, vactive:0x455
            pGC2145Ctx->curFrameLengthLines = pGC2145Ctx->frameLengthLines;
            pGC2145Ctx->frameLengthActive   = pGC2145Ctx->frameLengthLines - 0x172;
            pGC2145Ctx->maxIntegrationLine  = pGC2145Ctx->frameLengthLines - 9;
            pGC2145Ctx->minIntegrationLine  = 4;
            pGC2145Ctx->aecMaxGain          = 496;
            pGC2145Ctx->aecMinGain          = 1.0;
            pGC2145Ctx->aGain.min           = 1.0;
            pGC2145Ctx->aGain.max           = 15.5;
            pGC2145Ctx->aGain.step          = (1.0f/GC2145_AGAIN_ACCURACY);
            pGC2145Ctx->dGain.min           = 1.0;
            pGC2145Ctx->dGain.max           = 32;
            pGC2145Ctx->dGain.step          = (1.0f/GC2145_DGAIN_ACCURACY);
            break;
        case 1:
            pGC2145Ctx->oneLineExpTime   = 0.00004508;
            pGC2145Ctx->frameLengthLines    = 0x455; //vblank 0x22a,vactive: 0x22b
            pGC2145Ctx->curFrameLengthLines = pGC2145Ctx->frameLengthLines;
            pGC2145Ctx->frameLengthActive   = pGC2145Ctx->frameLengthLines - 0x22a;
            pGC2145Ctx->maxIntegrationLine  = pGC2145Ctx->curFrameLengthLines - 9;
            pGC2145Ctx->minIntegrationLine  = 4;
            pGC2145Ctx->aecMaxGain          = 496;
            pGC2145Ctx->aecMinGain          = 1.0;
            pGC2145Ctx->aGain.min           = 1.0;
            pGC2145Ctx->aGain.max           = 15.5;
            pGC2145Ctx->aGain.step          = (1.0f/GC2145_AGAIN_ACCURACY);
            pGC2145Ctx->dGain.min           = 1.0;
            pGC2145Ctx->dGain.max           = 32;
            pGC2145Ctx->dGain.step          = (1.0f/GC2145_DGAIN_ACCURACY);
            break;
        case 2:
            pGC2145Ctx->oneLineExpTime   = 0.00004508;
            pGC2145Ctx->frameLengthLines    = 0x455;//vblank 0, vactive:0x455
            pGC2145Ctx->curFrameLengthLines = pGC2145Ctx->frameLengthLines;
            pGC2145Ctx->frameLengthActive   = pGC2145Ctx->frameLengthLines;
            pGC2145Ctx->maxIntegrationLine  = pGC2145Ctx->frameLengthLines - 9;
            pGC2145Ctx->minIntegrationLine  = 4;
            pGC2145Ctx->aecMaxGain          = 496;
            pGC2145Ctx->aecMinGain          = 1.0;
            pGC2145Ctx->aGain.min           = 1.0;
            pGC2145Ctx->aGain.max           = 15.5;
            pGC2145Ctx->aGain.step          = (1.0f/GC2145_AGAIN_ACCURACY);
            pGC2145Ctx->dGain.min           = 1.0;
            pGC2145Ctx->dGain.max           = 32;
            pGC2145Ctx->dGain.step          = (1.0f/GC2145_DGAIN_ACCURACY);
            break;
        default:
            TRACE(GC2145_INFO, "%s:not support sensor mode %d\n", __func__,pGC2145Ctx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    //pGC2145Ctx->expoFrmType = ISI_EXPO_FRAME_TYPE_1FRAME;

    pGC2145Ctx->maxFps  = pGC2145Ctx->sensorMode.fps;
    pGC2145Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pGC2145Ctx->currFps = pGC2145Ctx->maxFps;

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(GC2145_DEBUG, "%s: GC2145 System-Reset executed\n", __func__);
    osSleep(100);

    result = GC2145_AecSetModeParameters(handle, pGC2145Ctx);
    if (result != RET_SUCCESS) {
        TRACE(GC2145_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pGC2145Ctx->configured = BOOL_TRUE;
    TRACE(GC2145_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT GC2145_IsiCloseIss(IsiSensorHandle_t handle)
{
    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(GC2145_INFO, "%s (enter)\n", __func__);

    if (pGC2145Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)GC2145_IsiSetStreamingIss(pGC2145Ctx, BOOL_FALSE);

    TRACE(GC2145_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT GC2145_IsiReleaseIss(IsiSensorHandle_t handle)
{
    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(GC2145_INFO, "%s (enter)\n", __func__);

    if (pGC2145Ctx == NULL) return (RET_WRONG_HANDLE);

	GC2145_ISiSetPowerIss((IsiSensorHandle_t) pGC2145Ctx, BOOL_FALSE);

    HalI2cClose(pGC2145Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pGC2145Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pGC2145Ctx, 0, sizeof(GC2145_Context_t));
    osFree(pGC2145Ctx);
    TRACE(GC2145_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT GC2145_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;

    uint32_t sensor_id = 0;
    uint32_t correct_id = 0x2145;

    TRACE(GC2145_INFO, "%s (enter)\n", __func__);

    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    if (pGC2145Ctx == NULL || pGC2145Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = GC2145_IsiGetRevisionIss(handle, &sensor_id);
    if (result != RET_SUCCESS) {
        TRACE(GC2145_ERROR, "%s: Read Sensor ID Error! \n",__func__);
        return (RET_FAILURE);
    }

    if (correct_id != sensor_id) {
        TRACE(GC2145_ERROR, "%s:ChipID =0x%x sensor_id=%x error! \n", __func__, correct_id, sensor_id);
        return (RET_FAILURE);
    }

    TRACE(GC2145_INFO,"%s ChipID = 0x%08x, sensor_id = 0x%08x, success! \n", __func__, correct_id, sensor_id);
    return (result);
}

static RESULT GC2145_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    uint32_t sensor_id = 0;

    TRACE(GC2145_INFO, "%s (enter)\n", __func__);

    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    if (pGC2145Ctx == NULL || pGC2145Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (!pValue) return (RET_NULL_POINTER);

    result    = GC2145_IsiReadRegIss(handle, 0xf0, &value);
    sensor_id = (value & 0xff) << 8;

    result   |= GC2145_IsiReadRegIss(handle, 0xf1, &value);
    sensor_id |= (value & 0xff);

    *pValue = sensor_id;
    TRACE(GC2145_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT GC2145_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;
    TRACE(GC2145_INFO, "%s (enter)\n", __func__);

    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    if (pGC2145Ctx == NULL || pGC2145Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pGC2145Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

//    result = GC2145_IsiWriteRegIss(handle, 0x0100, on);
//    if (result != RET_SUCCESS) {
//        TRACE(GC2145_ERROR, "%s: set sensor streaming error! \n",__func__);
//        return (RET_FAILURE);
//    }

    pGC2145Ctx->streaming = on;

    TRACE(GC2145_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT GC2145_pIsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(GC2145_INFO, "%s: (enter)\n", __func__);

    if (pGC2145Ctx == NULL) {
        TRACE(GC2145_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(GC2145_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    //get time limit and total gain limit
    pAeBaseInfo->gain.min        = pGC2145Ctx->aecMinGain;
    pAeBaseInfo->gain.max        = pGC2145Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pGC2145Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pGC2145Ctx->aecMaxIntegrationTime;

    pAeBaseInfo->vsGain.min      = pGC2145Ctx->aecMinGain;
    pAeBaseInfo->vsGain.max      = pGC2145Ctx->aecMaxGain;

    //get again/dgain info
    pAeBaseInfo->aGain           = pGC2145Ctx->aGain;
    pAeBaseInfo->dGain           = pGC2145Ctx->dGain;


    pAeBaseInfo->aecCurGain     = pGC2145Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime  = pGC2145Ctx->aecCurIntegrationTime;
    //pAeBaseInfo->aecCurLongGain = 0;
    pAeBaseInfo->aecGainStep    = pGC2145Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep = pGC2145Ctx->aecIntegrationTimeIncrement;
    //pAeBaseInfo->expoFrmType    = pGC2145Ctx->expoFrmType;
    pAeBaseInfo->stitchingMode  = pGC2145Ctx->sensorMode.stitchingMode;

    TRACE(GC2145_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT GC2145_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(GC2145_INFO, "%s: (enter)\n", __func__);
    // uint16_t again = 0;

    // GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    // if (pGC2145Ctx == NULL || pGC2145Ctx->isiCtx.halI2cHandle == NULL) {
    //     return RET_WRONG_HANDLE;
    // }
    // if (pSensorAGain == NULL) {
    //     return RET_NULL_POINTER;
    // }

    // if (pSensorAGain->expoFrmType != ISI_EXPO_FRAME_TYPE_1FRAME) {
    //     TRACE(GC2145_ERROR, "%s: set sensor AGain[ISI_LINEAR_PARAS] error %d!\n", __func__, pSensorAGain->expoFrmType);
    //     return RET_UNSUPPORT_ID;
    // }

    // if (pGC2145Ctx->aGain.min > pSensorAGain->Gain[ISI_LINEAR_PARAS]
    //     || pGC2145Ctx->aGain.max < pSensorAGain->Gain[ISI_LINEAR_PARAS]) {
    //     TRACE(GC2145_ERROR, "%s: set sensor again is out of range %f!\n", __func__, pSensorAGain->Gain);
    //     return RET_OUTOFRANGE;
    // }

    // again = (uint16_t)(pSensorAGain->Gain[ISI_LINEAR_PARAS] * GC2145_AGAIN_ACCURACY);

    // result = GC2145_IsiWriteRegIss(handle, 0xfd, 0x01);
    // result |= GC2145_IsiWriteRegIss(handle, 0x24,again);
    // result |= GC2145_IsiWriteRegIss(handle, 0x01, 0x01);
    // pGC2145Ctx->curAgain = (float32_t)again/GC2145_AGAIN_ACCURACY;
    // TRACE(GC2145_DEBUG, "%s: set_again=0x%x, curAgain=%f \n", __func__, again, pGC2145Ctx->curAgain);

    TRACE(GC2145_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC2145_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    //GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(GC2145_INFO, "%s: (enter)\n", __func__);

    // if (pGC2145Ctx == NULL) {
    //     TRACE(GC2145_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
    //     return (RET_WRONG_HANDLE);
    // }

    // if (pSensorAGain == NULL) {
    //     return (RET_NULL_POINTER);
    // }

    // if (pSensorAGain->expoFrmType != ISI_EXPO_FRAME_TYPE_1FRAME) {
    //     TRACE(GC2145_ERROR, "%s: get sensor AGain[ISI_LINEAR_PARAS] error %d!\n", __func__, pSensorAGain->expoFrmType);
    //     return RET_FAILURE;
    // }

    // pSensorAGain->Gain[ISI_LINEAR_PARAS]       = pGC2145Ctx->curAgain;

    TRACE(GC2145_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC2145_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(GC2145_INFO, "%s: (enter)\n", __func__);
    // uint16_t dgain = 0;

    // GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    // if (pGC2145Ctx == NULL || pGC2145Ctx->isiCtx.halI2cHandle == NULL) {
    //     return RET_WRONG_HANDLE;
    // }

    // if (pSensorDGain == NULL) {
    //     return RET_NULL_POINTER;
    // }

    // if (pSensorDGain->expoFrmType != ISI_EXPO_FRAME_TYPE_1FRAME) {
    //     TRACE(GC2145_ERROR, "%s: set sensor DGain[ISI_LINEAR_PARAS] error %d!\n", __func__, pSensorDGain->expoFrmType);
    //     return RET_UNSUPPORT_ID;
    // }

    // if (pGC2145Ctx->dGain.min > pSensorDGain->Gain[ISI_LINEAR_PARAS]
    //     || pGC2145Ctx->dGain.max < pSensorDGain->Gain[ISI_LINEAR_PARAS]) {
    //     TRACE(GC2145_ERROR, "%s: set sensor dgain is out of range %f!\n", __func__, pSensorDGain->Gain);
    //     return RET_OUTOFRANGE;
    // }
    // TRACE(GC2145_DEBUG, "%s: set_dgain=%f\n", __func__, pSensorDGain->Gain[ISI_LINEAR_PARAS]);

    // dgain = (uint16_t)(pSensorDGain->Gain[ISI_LINEAR_PARAS] * GC2145_DGAIN_ACCURACY);

    // result = GC2145_IsiWriteRegIss(handle,0xfd, 0x01);

    // result |= GC2145_IsiWriteRegIss(handle, 0x37, (dgain >> 8) & 0x7);
    // result |= GC2145_IsiWriteRegIss(handle, 0x39, (dgain & 0xff));

    // result |= GC2145_IsiWriteRegIss(handle, 0x01, 0x01);
    // pGC2145Ctx->curDgain = dgain/GC2145_DGAIN_ACCURACY;
    // pGC2145Ctx->aecCurGain = pGC2145Ctx->curAgain * pGC2145Ctx->curDgain;
    // TRACE(GC2145_DEBUG, "%s: set_dgain=0x%x, curDgain=%f, aecCurGain=%f\n", __func__, dgain, pGC2145Ctx->curDgain, pGC2145Ctx->aecCurGain);

    TRACE(GC2145_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC2145_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
//    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(GC2145_INFO, "%s: (enter)\n", __func__);

    // if (pGC2145Ctx == NULL) {
    //     TRACE(GC2145_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
    //     return (RET_WRONG_HANDLE);
    // }

    // if (pSensorDGain == NULL) {
    //     return (RET_NULL_POINTER);
    // }
    // if (pSensorDGain->expoFrmType != ISI_EXPO_FRAME_TYPE_1FRAME) {
    //     TRACE(GC2145_INFO, "%s: get sensor DGain[ISI_LINEAR_PARAS] error %d!\n", __func__, pSensorDGain->expoFrmType);
    //     return RET_UNSUPPORT_ID;
    // }

    // pSensorDGain->Gain[ISI_LINEAR_PARAS] = pGC2145Ctx->curDgain;
    TRACE(GC2145_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC2145_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(GC2145_INFO, "%s: (enter)\n", __func__);
    // GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;

    // if (!pGC2145Ctx) {
    //     TRACE(GC2145_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
    //     return (RET_WRONG_HANDLE);
    // }

    // if (pSensorIntTime->expoFrmType != ISI_EXPO_FRAME_TYPE_1FRAME) {
    //     TRACE(GC2145_INFO, "%s: set sensor IntTime[ISI_LINEAR_PARAS] error %d!\n", __func__, pSensorIntTime->expoFrmType);
    //     return RET_UNSUPPORT_ID;
    // }

    // float32_t newIntegrationTime = pSensorIntTime->IntTime[ISI_LINEAR_PARAS];
    // uint32_t exp_line = 0;

    // exp_line = newIntegrationTime / pGC2145Ctx->oneLineExpTime;
    // exp_line = MIN(pGC2145Ctx->maxIntegrationLine,MAX(4, exp_line));
    // TRACE(GC2145_DEBUG, "%s: set exp_line = 0x%04x\n", __func__, exp_line);

    // result =  GC2145_IsiWriteRegIss(handle, 0xfd, 0x1);
    // result |=  GC2145_IsiWriteRegIss(handle, 0x03,(exp_line >>8) & 0xff);
    // result |= GC2145_IsiWriteRegIss(handle, 0x04,(exp_line & 0xff));
    // result |= GC2145_IsiWriteRegIss(handle, 0x01, 0x01);

    // pGC2145Ctx->aecCurIntegrationTime = exp_line * pGC2145Ctx->oneLineExpTime;

    // TRACE(GC2145_DEBUG, "%s: set IntTime = %f\n", __func__, pGC2145Ctx->aecCurIntegrationTime);

    TRACE(GC2145_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC2145_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
//    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(GC2145_INFO, "%s: (enter)\n", __func__);

    // if (!pGC2145Ctx) {
    //     TRACE(GC2145_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
    //     return (RET_WRONG_HANDLE);
    // }

    // if (!pSensorIntTime) return (RET_NULL_POINTER);

    // if (pSensorIntTime->expoFrmType != ISI_EXPO_FRAME_TYPE_1FRAME) {
    //     TRACE(GC2145_INFO, "%s: get sensor IntTime[ISI_LINEAR_PARAS] error %d!\n", __func__, pSensorIntTime->expoFrmType);
    //     return RET_UNSUPPORT_ID;
    // }

    // pSensorIntTime->IntTime[ISI_LINEAR_PARAS] = pGC2145Ctx->aecCurIntegrationTime;
    // TRACE(GC2145_DEBUG, "%s: get IntTime = %f\n", __func__, pGC2145Ctx->aecCurIntegrationTime);

    TRACE(GC2145_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC2145_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t * pFps)
{
    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(GC2145_INFO, "%s: (enter)\n", __func__);

    if (pGC2145Ctx == NULL) {
        TRACE(GC2145_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pGC2145Ctx->currFps;

    TRACE(GC2145_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC2145_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t Fps)
{
    RESULT result = RET_SUCCESS;
//     uint16_t newVTS = 0;
//     uint16_t dummyLines = 0;
//     uint16_t value = 0;


//     TRACE(GC2145_INFO, "%s: (enter)\n", __func__);

//     GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
//     if (pGC2145Ctx == NULL) {
//         TRACE(GC2145_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
//         return (RET_WRONG_HANDLE);
//     }

//    if (Fps > pGC2145Ctx->maxFps) {
//         TRACE(GC2145_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, Fps, pGC2145Ctx->maxFps, pGC2145Ctx->minFps,pGC2145Ctx->maxFps);
//         Fps = pGC2145Ctx->maxFps;
//     }
//     if (Fps < pGC2145Ctx->minFps) {
//         TRACE(GC2145_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, Fps, pGC2145Ctx->minFps, pGC2145Ctx->minFps,pGC2145Ctx->maxFps);
//         Fps = pGC2145Ctx->minFps;
//     }

//     newVTS = pGC2145Ctx->frameLengthLines * pGC2145Ctx->sensorMode.fps / Fps;

//     //insert dummy line number
//     dummyLines = newVTS - pGC2145Ctx->frameLengthActive;

    // result =  GC2145_IsiWriteRegIss(handle, 0xfe, 0x00);

    // result |=  GC2145_IsiWriteRegIss(handle, 0x07, dummyLines >> 8);
    // result |=  GC2145_IsiWriteRegIss(handle, 0x08, dummyLines & 0xff);
    // //trigger active
    // result |=  GC2145_IsiWriteRegIss(handle, 0x01, 0x01);

    // pGC2145Ctx->currFps              = Fps;
    // pGC2145Ctx->curFrameLengthLines  = newVTS;
    // pGC2145Ctx->maxIntegrationLine   = pGC2145Ctx->curFrameLengthLines - 9;
    // pGC2145Ctx->aecMaxIntegrationTime = pGC2145Ctx->maxIntegrationLine * pGC2145Ctx->oneLineExpTime;

    TRACE(GC2145_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC2145_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    if (pGC2145Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(GC2145_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    pIspStatus->useSensorAWB = false;

    TRACE(GC2145_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT GC2145_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t Tpg)
{
    RESULT result = RET_SUCCESS;
//    uint16_t value = 0;
    TRACE(GC2145_INFO, "%s: (enter)\n", __func__);

    // GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    // if (pGC2145Ctx == NULL || pGC2145Ctx->isiCtx.halI2cHandle == NULL) {
    //     return RET_NULL_POINTER;
    // }

    // if (pGC2145Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    // result = GC2145_IsiWriteRegIss(handle, 0xfd, 0x01);

    // result |= GC2145_IsiReadRegIss(handle, 0x0d, &value);

    // if (Tpg.enable == 0) {
    //     result |= GC2145_IsiWriteRegIss(handle, 0x0d, value & 0xfe);
    // } else {
    //     result |= GC2145_IsiWriteRegIss(handle, 0x0d, value | 0x01);
    // }

    // pGC2145Ctx->testPattern = Tpg.enable;

    TRACE(GC2145_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC2145_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *Tpg)
{
    RESULT result = RET_SUCCESS;
//    uint16_t value = 0;
    TRACE(GC2145_INFO, "%s: (enter)\n", __func__);

    // GC2145_Context_t *pGC2145Ctx = (GC2145_Context_t *) handle;
    // if (pGC2145Ctx == NULL || pGC2145Ctx->isiCtx.halI2cHandle == NULL || Tpg == NULL) {
    //     return RET_NULL_POINTER;
    // }

    // if (pGC2145Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    // result = GC2145_IsiWriteRegIss(handle, 0xfd, 0x01);

    // if (RET_SUCCESS == GC2145_IsiReadRegIss(handle, 0x0d, &value)) {
    //     Tpg->enable = ((value & 0x01) != 0) ? 1 : 0;
    //     if(Tpg->enable) {
    //        Tpg->pattern = (0x01 & value);
    //        pGC2145Ctx->testPattern = BOOL_TRUE;
    //     } else {
    //        pGC2145Ctx->testPattern = BOOL_FALSE;
    //     }
    // }

    TRACE(GC2145_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT GC2145_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "GC2145";
    TRACE(GC2145_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = GC2145_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = GC2145_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = GC2145_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = GC2145_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = GC2145_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = GC2145_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = GC2145_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = GC2145_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = GC2145_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = GC2145_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = GC2145_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = GC2145_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = GC2145_pIsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = GC2145_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = GC2145_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = GC2145_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = GC2145_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = GC2145_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = GC2145_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = GC2145_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = GC2145_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = GC2145_IsiGetIspStatusIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = GC2145_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = GC2145_IsiGetTpgIss;

        /* AF */
        pIsiSensor->pIsiFocusCreateIss                  = NULL;
        pIsiSensor->pIsiFocusReleaseIss                 = NULL;
        pIsiSensor->pIsiFocusGetCalibrateIss            = NULL;
        pIsiSensor->pIsiFocusSetIss                     = NULL;
        pIsiSensor->pIsiFocusGetIss                     = NULL;

    } else {
        result = RET_NULL_POINTER;
    }

    TRACE(GC2145_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t GC2145_IsiCamDrvConfig = {
    .cameraDriverID      = 0x2145,
    .pIsiGetSensorIss    = GC2145_IsiGetSensorIss,
};
