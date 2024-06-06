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
#include <stdio.h>
#include "isi.h"
#include "isi_iss.h"
#include "isi_otp.h"
#include "IMX258_priv.h"
#include "imx258_linear.h"

CREATE_TRACER( IMX258_INFO , "IMX258: ", INFO,    1);
CREATE_TRACER( IMX258_WARN , "IMX258: ", WARNING, 1);
CREATE_TRACER( IMX258_ERROR, "IMX258: ", ERROR,   1);
CREATE_TRACER( IMX258_DEBUG,     "IMX258: ", INFO, 1);
CREATE_TRACER( IMX258_REG_INFO , "IMX258: ", INFO, 1);
CREATE_TRACER( IMX258_REG_DEBUG, "IMX258: ", INFO, 1);

#define IMX258_MaxAGain     ( 16.0f )            /* imx258 max analog gain time */
#define IMX258_MIN_GAIN_STEP    (1.0f/64.0f)

#define IMX258_SLAVE_ADDR	0x1A
#define IMX258_DPHY_FREQ	1075000

#define H8V8
#define H1V1

/*****************************************************************************
 *Sensor Info
*****************************************************************************/
static IsiSensorMode_t pimx258_mode_info[] = {
    {
        .index         = 0,
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
        .fps           = 30 * ISI_FPS_QUANTIZE,
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
};

static RESULT IMX258_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX258_DEBUG,  "%s (enter)\n", __func__);

    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    if (pIMX258Ctx == NULL || pIMX258Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pIMX258Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(IMX258_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(IMX258_DEBUG,  "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT IMX258_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX258_DEBUG,  "%s (enter)\n", __func__);

    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    if (pIMX258Ctx == NULL || pIMX258Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pIMX258Ctx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(IMX258_ERROR, "%s: hal write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(IMX258_DEBUG,  "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT IMX258_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(IMX258_DEBUG,  "%s (enter)\n", __func__);

    const IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    if (pIMX258Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pIMX258Ctx->sensorMode), sizeof(pIMX258Ctx->sensorMode));

    TRACE(IMX258_DEBUG,  "%s (exit)\n", __func__);
    return ( RET_SUCCESS );
}

static  RESULT IMX258_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(IMX258_DEBUG,  "%s (enter) with index %d\n", __func__, pEnumMode->index);
    const IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    if (pIMX258Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pimx258_mode_info)/sizeof(pimx258_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pimx258_mode_info)/sizeof(pimx258_mode_info[0])); i++) {
        if (pimx258_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pimx258_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(IMX258_DEBUG,  "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT IMX258_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(IMX258_DEBUG,  "%s (enter)\n", __func__);

    if (pIMX258Ctx == NULL) return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pIMX258Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pIMX258Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pIMX258Ctx->sensorMode.size.width;
    pCaps->resolution.height = pIMX258Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;
    pCaps->dphyFreq          = IMX258_DPHY_FREQ;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else if (pCaps->bitWidth == 8) {
         pCaps->mipiMode      = ISI_FORMAT_RAW_8;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(IMX258_DEBUG,  "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX258_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle) {
    RESULT result = RET_SUCCESS;
    TRACE(IMX258_DEBUG,  "%s (enter)\n", __func__);

    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) malloc(sizeof(IMX258_Context_t));
    if (!pIMX258Ctx) {
        TRACE(IMX258_ERROR, "%s: Can't allocate imx258 context\n",__func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pIMX258Ctx, 0, sizeof(IMX258_Context_t));

    pIMX258Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pIMX258Ctx->groupHold          = BOOL_FALSE;
    pIMX258Ctx->oldGain            = 0;
    pIMX258Ctx->oldIntegrationTime = 0;
    pIMX258Ctx->configured         = BOOL_FALSE;
    pIMX258Ctx->streaming          = BOOL_FALSE;
    pIMX258Ctx->testPattern        = BOOL_FALSE;
    pIMX258Ctx->isAfpsRun          = BOOL_FALSE;
    pIMX258Ctx->sensorMode.index   = 0;

    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = IMX258_SLAVE_ADDR;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pIMX258Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pIMX258Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(IMX258_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pIMX258Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pIMX258Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pIMX258Ctx;

    TRACE(IMX258_DEBUG,  "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX258_AecSetModeParameters(IsiSensorHandle_t handle, IMX258_Context_t *pIMX258Ctx)
{
    uint32_t regVal = 0;
	uint16_t value = 0;
    RESULT result = RET_SUCCESS;
    TRACE(IMX258_DEBUG,  "%s%s: (enter)\n", __func__,pIMX258Ctx->isAfpsRun ? "(AFPS)" : "");

    pIMX258Ctx->aecIntegrationTimeIncrement = pIMX258Ctx->oneLineExpTime;
    pIMX258Ctx->aecMinIntegrationTime       = pIMX258Ctx->oneLineExpTime * pIMX258Ctx->minIntegrationLine;
    pIMX258Ctx->aecMaxIntegrationTime       = pIMX258Ctx->oneLineExpTime * pIMX258Ctx->maxIntegrationLine;

    TRACE(IMX258_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pIMX258Ctx->isAfpsRun ? "(AFPS)" : "",pIMX258Ctx->aecMaxIntegrationTime);

    pIMX258Ctx->aecGainIncrement = IMX258_MIN_GAIN_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pIMX258Ctx->oldGain               = 0;
    pIMX258Ctx->oldIntegrationTime    = 0;

    IMX258_IsiReadRegIss(handle, 0x0202, &value);
    regVal = (value & 0x3f) << 8;
    IMX258_IsiReadRegIss(handle, 0x0203, &value);
    regVal = regVal | (value & 0xff);
    pIMX258Ctx->aecCurIntegrationTime = regVal * pIMX258Ctx->oneLineExpTime;
    pIMX258Ctx->aecCurGain = 1;

    TRACE(IMX258_DEBUG, "%s%s: (exit)\n", __func__,pIMX258Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT IMX258_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX258_DEBUG,  "%s (enter)\n", __func__);

    if (!pIMX258Ctx) {
        TRACE(IMX258_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pIMX258Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pIMX258Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pimx258_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (pimx258_mode_info[i].index == pIMX258Ctx->sensorMode.index) {
            SensorDefaultMode = &(pimx258_mode_info[i]);
            break;
        }
    }

	TRACE(IMX258_DEBUG, "%s SensorDefaultMode.index = %d\n", __func__, SensorDefaultMode->index);

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
        case 0:
            for (int i = 0; i<sizeof(IMX258_REG_3840) / sizeof(IMX258_REG_3840[0]); i++) {
                    IMX258_IsiWriteRegIss(handle, IMX258_REG_3840[i][0], IMX258_REG_3840[i][1]);
                }
            break;
        case 1:
            for (int i = 0; i<sizeof(IMX258_REG_3840_LANE2) / sizeof(IMX258_REG_3840_LANE2[0]); i++) {
                    IMX258_IsiWriteRegIss(handle, IMX258_REG_3840_LANE2[i][0], IMX258_REG_3840_LANE2[i][1]);
                }
            break;
        case 2:
            for (int i = 0; i<sizeof(IMX258_REG_1920) / sizeof(IMX258_REG_1920[0]); i++) {
                    IMX258_IsiWriteRegIss(handle, IMX258_REG_1920[i][0], IMX258_REG_1920[i][1]);
                }
            break;
        case 3:
            for (int i = 0; i<sizeof(IMX258_REG_1920_LANE2) / sizeof(IMX258_REG_1920_LANE2[0]); i++) {
                    IMX258_IsiWriteRegIss(handle, IMX258_REG_1920_LANE2[i][0], IMX258_REG_1920_LANE2[i][1]);
                }
            break;
        default:
            TRACE(IMX258_DEBUG,  "%s:not support sensor mode %d\n", __func__,pIMX258Ctx->sensorMode.index);
            return RET_NOTSUPP;
       }

        memcpy(&(pIMX258Ctx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));
    } else {
        TRACE(IMX258_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

	TRACE(IMX258_DEBUG,  "%s pIMX258Ctx->sensorMode.index = %d\n", __func__, pIMX258Ctx->sensorMode.index);
			pIMX258Ctx->VtPixClkFreq     = 126880000.0f;
			pIMX258Ctx->LineLengthPck    =  0x14E8;//HTS[0x0342,0x0343]
            pIMX258Ctx->aecCurGain          = 1.0;

    switch(pIMX258Ctx->sensorMode.index) {
        case 0:
            pIMX258Ctx->oneLineExpTime      = 0.000010545;
            pIMX258Ctx->frameLengthLines    = 0xC56; //VTS[0x0340,0x0341]
            pIMX258Ctx->curFrameLengthLines = pIMX258Ctx->frameLengthLines;
            pIMX258Ctx->maxIntegrationLine  = pIMX258Ctx->frameLengthLines -8 ;
            pIMX258Ctx->minIntegrationLine  = 4;
            pIMX258Ctx->aecMaxGain          = 128;
            pIMX258Ctx->aecMinGain          = 1.0;
            pIMX258Ctx->aecCurGain          = 1.0;
            pIMX258Ctx->aGain.min           = 1.0;
            pIMX258Ctx->aGain.max           = 16.0;
            pIMX258Ctx->aGain.step          = (1.0f/256.0f);
            pIMX258Ctx->dGain.min           = 1.0;
            pIMX258Ctx->dGain.max           = 8;
            pIMX258Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 1:
            pIMX258Ctx->oneLineExpTime      = 0.00003556;
            pIMX258Ctx->frameLengthLines    = 0x65A;
            pIMX258Ctx->curFrameLengthLines = pIMX258Ctx->frameLengthLines;
            pIMX258Ctx->maxIntegrationLine  = pIMX258Ctx->curFrameLengthLines - 8;
            pIMX258Ctx->minIntegrationLine  = 4;
            pIMX258Ctx->aecMaxGain          = 128;
            pIMX258Ctx->aecMinGain          = 1.0;
            pIMX258Ctx->aGain.min           = 1.0;
            pIMX258Ctx->aGain.max           = 16.0;
            pIMX258Ctx->aGain.step          = (1.0f/256.0f);
            pIMX258Ctx->dGain.min           = 1.0;
            pIMX258Ctx->dGain.max           = 8;
            pIMX258Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 2:
            pIMX258Ctx->oneLineExpTime      = 0.00001876;
            pIMX258Ctx->frameLengthLines    = 0x6F0;
            pIMX258Ctx->curFrameLengthLines = pIMX258Ctx->frameLengthLines;
            pIMX258Ctx->maxIntegrationLine  = pIMX258Ctx->frameLengthLines -8 ;
            pIMX258Ctx->minIntegrationLine  = 4;
            pIMX258Ctx->aecMaxGain          = 128;
            pIMX258Ctx->aecMinGain          = 1.0;
            pIMX258Ctx->aGain.min           = 1.0;
            pIMX258Ctx->aGain.max           = 16.0;
            pIMX258Ctx->aGain.step          = (1.0f/256.0f);
            pIMX258Ctx->dGain.min           = 1.0;
            pIMX258Ctx->dGain.max           = 8;
            pIMX258Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 3:
            pIMX258Ctx->oneLineExpTime      = 0.00001876;
            pIMX258Ctx->frameLengthLines    = 0x6F0;
            pIMX258Ctx->curFrameLengthLines = pIMX258Ctx->frameLengthLines;
            pIMX258Ctx->maxIntegrationLine  = pIMX258Ctx->frameLengthLines -8 ;
            pIMX258Ctx->minIntegrationLine  = 4;
            pIMX258Ctx->aecMaxGain          = 128;
            pIMX258Ctx->aecMinGain          = 1.0;
            pIMX258Ctx->aGain.min           = 1.0;
            pIMX258Ctx->aGain.max           = 16.0;
            pIMX258Ctx->aGain.step          = (1.0f/256.0f);
            pIMX258Ctx->dGain.min           = 1.0;
            pIMX258Ctx->dGain.max           = 8;
            pIMX258Ctx->dGain.step          = (1.0f/256.0f);
            break;
        default:
            TRACE(IMX258_DEBUG,  "%s:not support sensor mode %d\n", __func__,pIMX258Ctx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    pIMX258Ctx->maxFps  = pIMX258Ctx->sensorMode.fps;
    pIMX258Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pIMX258Ctx->currFps = pIMX258Ctx->maxFps;

    /* 1.) SW reset of image sensor (via I2C register interface)  be careful, bits 6..0 are reserved, reset bit is not sticky */
    TRACE(IMX258_DEBUG, "%s: IMX258 System-Reset executed\n", __func__);
    osSleep(100);

    result = IMX258_AecSetModeParameters(handle, pIMX258Ctx);
    if (result != RET_SUCCESS) {
        TRACE(IMX258_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pIMX258Ctx->configured = BOOL_TRUE;
    TRACE(IMX258_DEBUG,  "%s: (exit)\n", __func__);
    return 0;
}

static RESULT IMX258_IsiCloseIss(IsiSensorHandle_t handle)
{
    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX258_DEBUG,  "%s (enter)\n", __func__);

    if (pIMX258Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)IMX258_IsiSetStreamingIss(pIMX258Ctx, BOOL_FALSE);

    TRACE(IMX258_DEBUG,  "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX258_IsiReleaseIss(IsiSensorHandle_t handle)
{
    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX258_DEBUG,  "%s (enter)\n", __func__);

    if (pIMX258Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pIMX258Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pIMX258Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pIMX258Ctx, 0, sizeof(IMX258_Context_t));
    free(pIMX258Ctx);
    TRACE(IMX258_DEBUG,  "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX258_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;

    uint32_t sensorId = 0;
    uint32_t correctId = 0x0258;

    TRACE(IMX258_DEBUG,  "%s (enter)\n", __func__);

    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    if (pIMX258Ctx == NULL || pIMX258Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = IMX258_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(IMX258_ERROR, "%s: Read Sensor ID Error! \n", __func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(IMX258_ERROR, "%s:ChipID =0x%x sensorId=0x%x error! \n",__func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(IMX258_INFO, "%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__,correctId, sensorId);

    TRACE(IMX258_DEBUG,  "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX258_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(IMX258_DEBUG,  "%s (enter)\n", __func__);

    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    if (pIMX258Ctx == NULL || pIMX258Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    TRACE(IMX258_DEBUG,  "%s (enter) offset 0x0016\n", __func__);
    regVal = 0;
    result = IMX258_IsiReadRegIss(handle, 0x0016, &regVal);
    sensorId = (regVal & 0xff) << 8;

    regVal = 0;
    result |= IMX258_IsiReadRegIss(handle, 0x0017, &regVal);
    sensorId |= (regVal & 0xff);

    *pValue = sensorId;
    TRACE(IMX258_DEBUG,  "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX258_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on) {
    RESULT result = RET_SUCCESS;
    TRACE(IMX258_DEBUG,  "%s (enter)\n", __func__);

    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    if (pIMX258Ctx == NULL || pIMX258Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

	if (pIMX258Ctx->configured != BOOL_TRUE) return RET_WRONG_STATE;

	if(!on)
		IMX258_IsiWriteRegIss(handle, 0x0106, 0);
	result = IMX258_IsiWriteRegIss(handle, 0x0100, on);
	if (result != RET_SUCCESS) {
		TRACE(IMX258_ERROR, "%s: set sensor streaming error! \n",__func__);
		return (RET_FAILURE);
	}

	pIMX258Ctx->streaming = on;

	TRACE(IMX258_DEBUG, "%s (exit)\n", __func__);
	return (result);
}

static RESULT IMX258_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX258_DEBUG, "%s: (enter)\n", __func__);

    if (pIMX258Ctx == NULL) {
        TRACE(IMX258_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(IMX258_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    //get time limit and total gain limit
    pAeBaseInfo->gain.min        = pIMX258Ctx->aecMinGain;
    pAeBaseInfo->gain.max        = pIMX258Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pIMX258Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pIMX258Ctx->aecMaxIntegrationTime;

    //get again/dgain info
    pAeBaseInfo->aGain           = pIMX258Ctx->aGain;
    pAeBaseInfo->dGain           = pIMX258Ctx->dGain;

    pAeBaseInfo->aecCurGain     = pIMX258Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime  = pIMX258Ctx->aecCurIntegrationTime;
    pAeBaseInfo->aecGainStep    = pIMX258Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep = pIMX258Ctx->aecIntegrationTimeIncrement;

	TRACE(IMX258_DEBUG, "%s: with gain %f and intg %f \n", __func__,  pAeBaseInfo->aecCurGain, pAeBaseInfo->aecCurIntTime);

    TRACE(IMX258_DEBUG, "%s: (enter)\n", __func__);
    return (result);
}

RESULT IMX258_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX258_DEBUG, "%s: (enter)\n", __func__);
    uint32_t again = 0;

    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    if (pIMX258Ctx == NULL || pIMX258Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

	again = 512-512/pSensorAGain->gain[ISI_LINEAR_PARAS];
	if(again >= 480) {
		again = 480;
	}

	IMX258_IsiWriteRegIss(handle, 0x0104, 0x01);
#ifdef  H1V1
    result |= IMX258_IsiWriteRegIss(handle, 0x0204, (again >> 8) & 0xff);
    result |= IMX258_IsiWriteRegIss(handle, 0x0205, again & 0xff );
#endif
	IMX258_IsiWriteRegIss(handle, 0x0104, 0x00);

    TRACE(IMX258_DEBUG, "Set Again %d NewGain %f\n",
			again, pSensorAGain->gain[ISI_LINEAR_PARAS]);
    pIMX258Ctx->curAgain = pSensorAGain->gain[ISI_LINEAR_PARAS];
    pIMX258Ctx->aecCurGain = pSensorAGain->gain[ISI_LINEAR_PARAS];

    TRACE(IMX258_DEBUG,  "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX258_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
	RESULT result = RET_SUCCESS;
	TRACE(IMX258_DEBUG,  "%s: (enter)\n", __func__);
	uint32_t dgain = 0;

    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    if (pIMX258Ctx == NULL || pIMX258Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

	dgain = (548 * (pSensorDGain->gain[ISI_LINEAR_PARAS] - 1) + 256); // 548.4286 = (4095 - 256) / (8 - 1)
	if( dgain <= 256) {
		dgain = 256;
	}

	IMX258_IsiWriteRegIss(handle, 0x0104, 0x01);
	if(dgain > 1) {
#ifdef H1V1
		result |= IMX258_IsiWriteRegIss(handle, 0x020e, (dgain & 0xFF00) >> 8);
		result |= IMX258_IsiWriteRegIss(handle, 0x020f, dgain & 0x00FF);
		result |= IMX258_IsiWriteRegIss(handle, 0x0210, (dgain & 0xFF00) >> 8);
		result |= IMX258_IsiWriteRegIss(handle, 0x0211, dgain & 0x00FF);
		result |= IMX258_IsiWriteRegIss(handle, 0x0212, (dgain & 0xFF00) >> 8);
		result |= IMX258_IsiWriteRegIss(handle, 0x0213, dgain & 0x00FF);
		result |= IMX258_IsiWriteRegIss(handle, 0x0214, (dgain & 0xFF00) >> 8);
		result |= IMX258_IsiWriteRegIss(handle, 0x0215, dgain & 0x00FF);
#endif
	}
	IMX258_IsiWriteRegIss(handle, 0x0104, 0x00);

	pIMX258Ctx->curDgain = pSensorDGain->gain[ISI_LINEAR_PARAS];
    TRACE(IMX258_DEBUG, "Set Dgain %d NewGain %f\n",
			dgain, pSensorDGain->gain[ISI_LINEAR_PARAS]);

	TRACE(IMX258_DEBUG,  "%s: (exit)\n", __func__);
	return (result);
}

RESULT IMX258_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    const IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX258_DEBUG,  "%s: (enter)\n", __func__);

    if (pIMX258Ctx == NULL) {
        TRACE(IMX258_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorAGain->gain[ISI_LINEAR_PARAS] = pIMX258Ctx->curAgain;

    TRACE(IMX258_DEBUG,  "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX258_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    const IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX258_DEBUG,  "%s: (enter)\n", __func__);

    if (pIMX258Ctx == NULL) {
        TRACE(IMX258_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorDGain->gain[ISI_LINEAR_PARAS] = pIMX258Ctx->curDgain;

    TRACE(IMX258_DEBUG,  "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX258_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX258_DEBUG, "%s: (enter)\n", __func__);
    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    uint32_t expLine = 0;

    if (!pIMX258Ctx) {
        TRACE(IMX258_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = pSensorIntTime->intTime[ISI_LINEAR_PARAS] / pIMX258Ctx->oneLineExpTime;
    expLine = MIN(pIMX258Ctx->maxIntegrationLine, MAX(pIMX258Ctx->minIntegrationLine, expLine));
    TRACE(IMX258_DEBUG, "%s: set expLine = 0x%04x\n", __func__, expLine);

	 IMX258_IsiWriteRegIss(handle, 0x0104, 0x01);
    result =  IMX258_IsiWriteRegIss(handle, 0x0202, (expLine >> 8) & 0x3f);
    result |= IMX258_IsiWriteRegIss(handle, 0x0203, (expLine & 0xff));

	 IMX258_IsiWriteRegIss(handle, 0x0104, 0x00);
    pIMX258Ctx->aecCurIntegrationTime = expLine * pIMX258Ctx->oneLineExpTime;

    TRACE(IMX258_DEBUG, "%s: set IntTime = %f\n", __func__, pIMX258Ctx->aecCurIntegrationTime);

    TRACE(IMX258_DEBUG, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX258_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    const IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX258_DEBUG,  "%s: (enter)\n", __func__);

    if (!pIMX258Ctx) {
        TRACE(IMX258_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    pSensorIntTime->intTime[ISI_LINEAR_PARAS] = pIMX258Ctx->aecCurIntegrationTime;

    TRACE(IMX258_DEBUG,  "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX258_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t *pFps)
{
    const IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX258_DEBUG,  "%s: (enter)\n", __func__);

    if (pIMX258Ctx == NULL) {
        TRACE(IMX258_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pIMX258Ctx->currFps;

    TRACE(IMX258_DEBUG,  "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX258_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    RESULT result = RET_SUCCESS;
    int32_t NewVts = 0;
    TRACE(IMX258_DEBUG,  "%s: (enter)\n", __func__);

    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    if (pIMX258Ctx == NULL) {
        TRACE(IMX258_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (fps > pIMX258Ctx->maxFps) {
        TRACE(IMX258_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__,
                             fps, pIMX258Ctx->maxFps, pIMX258Ctx->minFps,pIMX258Ctx->maxFps);
        fps = pIMX258Ctx->maxFps;
    }
    if (fps < pIMX258Ctx->minFps) {
        TRACE(IMX258_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__,
                            fps, pIMX258Ctx->minFps, pIMX258Ctx->minFps,pIMX258Ctx->maxFps);
        fps = pIMX258Ctx->minFps;
    }

    NewVts = pIMX258Ctx->frameLengthLines*pIMX258Ctx->sensorMode.fps / fps;

    #ifdef  H1V1
    result  =  IMX258_IsiWriteRegIss(handle, 0x033d, NewVts >> 16);
    result |=  IMX258_IsiWriteRegIss(handle, 0x033e, ((NewVts >> 8) & 0xff));
    result |=  IMX258_IsiWriteRegIss(handle, 0x033f, NewVts & 0xff);
    #endif

    #ifdef  H8V8
    result |=  IMX258_IsiWriteRegIss(handle, 0xe819, NewVts >> 16);
    result |=  IMX258_IsiWriteRegIss(handle, 0xe81a, ((NewVts >> 8) & 0xff));
    result |=  IMX258_IsiWriteRegIss(handle, 0xe81b, NewVts & 0xff);
    #endif

    pIMX258Ctx->currFps              = fps;
    pIMX258Ctx->curFrameLengthLines  = NewVts;
    pIMX258Ctx->maxIntegrationLine   = pIMX258Ctx->curFrameLengthLines - 8;
    //pIMX258Ctx->aecMaxIntegrationTime = pIMX258Ctx->maxIntegrationLine * pIMX258Ctx->oneLineExpTime;

    TRACE(IMX258_DEBUG,  "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX258_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    const IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    if (pIMX258Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(IMX258_DEBUG,  "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    pIspStatus->useSensorAWB = false;

    TRACE(IMX258_DEBUG,  "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT IMX258_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX258_DEBUG,  "%s: (enter)\n", __func__);

    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    if (pIMX258Ctx == NULL || pIMX258Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pIMX258Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = IMX258_IsiWriteRegIss(handle, 0x0600, 0x00);
        result |= IMX258_IsiWriteRegIss(handle, 0x0601, 0x00);
    } else {
        result = IMX258_IsiWriteRegIss(handle, 0x0600, 0x00);
        result |= IMX258_IsiWriteRegIss(handle, 0x0601, 0x02);
    }

    pIMX258Ctx->testPattern = tpg.enable;

    TRACE(IMX258_DEBUG,  "%s: (exit)\n", __func__);
    return (result);
}
RESULT IMX258_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(IMX258_DEBUG,  "%s: (enter)\n", __func__);

    IMX258_Context_t *pIMX258Ctx = (IMX258_Context_t *) handle;
    if (pIMX258Ctx == NULL || pIMX258Ctx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pIMX258Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!IMX258_IsiReadRegIss(handle, 0x6001,&value)) {
        pTpg->enable = ((value & 0x02) != 0) ? 1 : 0;
        if(pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pIMX258Ctx->testPattern = pTpg->enable;
    }

    TRACE(IMX258_DEBUG,  "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX258_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "IMX258";
    TRACE( IMX258_DEBUG, "%s (enter)\n", __func__);

    if ( pIsiSensor != NULL ) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = IMX258_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = IMX258_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = IMX258_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = IMX258_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = IMX258_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = IMX258_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = IMX258_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = IMX258_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = IMX258_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = IMX258_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = IMX258_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = IMX258_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = IMX258_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = IMX258_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = IMX258_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = IMX258_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = IMX258_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = IMX258_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = IMX258_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = IMX258_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = IMX258_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = IMX258_IsiGetIspStatusIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = IMX258_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = IMX258_IsiGetTpgIss;

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

    TRACE( IMX258_DEBUG, "%s (exit)\n", __func__);
    return ( result );
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t IMX258_IsiCamDrvConfig = {
    .cameraDriverID      = 0x0000,
    .pIsiGetSensorIss    = IMX258_IsiGetSensorIss,
};
