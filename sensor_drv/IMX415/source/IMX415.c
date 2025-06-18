// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Synaptics Incorporated
#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <common/misc.h>
#include <math.h>
#include <stdio.h>
#include "isi.h"
#include "isi_iss.h"
#include "isi_otp.h"
#include "IMX415_priv.h"
#include "imx415_linear.h"

CREATE_TRACER( IMX415_INFO , "IMX415: ", INFO,    1);
CREATE_TRACER( IMX415_WARN , "IMX415: ", WARNING, 1);
CREATE_TRACER( IMX415_ERROR, "IMX415: ", ERROR,   1);
CREATE_TRACER( IMX415_DEBUG,     "IMX415: ", INFO, 1);
CREATE_TRACER( IMX415_REG_INFO , "IMX415: ", INFO, 1);
CREATE_TRACER( IMX415_REG_DEBUG, "IMX415: ", INFO, 1);

#define IMX415_MIN_REG_STEP             ( 10.0f/3.0f )
#define IMX415_MAX_REG_VAL              ( 240 )
#define IMX415_MIN_REG_VAL              ( 4 )

#define IMX415_MIN_AEC_STEP             ( 1.0f/3.0f )
#define IMX415_MAX_AEC_VAL              ( 72 )
#define IMX415_MIN_AEC_VAL              ( 1 )

#define IMX415_MIN_AGAIN_STEP           IMX415_MIN_AEC_STEP
#define IMX415_MAX_AGAIN_VAL            IMX415_MAX_AEC_VAL
#define IMX415_MIN_AGAIN_VAL            IMX415_MIN_AEC_VAL

#define IMX415_FRAME_PERIOD             ( 0x08CA )      /* Ref DOC - page 59 */
#define IMX415_H_PERIOD_30FPS           ( 0.0000149f )  /* Ref DOC - Page 59 */
#define IMX415_T_OFFSET_12B             ( 0.00000268f ) /* Ref DOC - Page 70 */

#define IMX415_MIN_INTEGRATION_TIME     ( 0.0001 )
#define IMX415_MAX_INTEGRATION_TIME     ( 0.032 )

#define IMX415_MIN_STORAGE_TIME         ( 8 )
#define IMX415_MAX_STORAGE_TIME         ( 0x008CA - 4 )

#define IMX415_SLAVE_ADDR               ( 0x1A )

/*****************************************************************************
 *Sensor Info
*****************************************************************************/
static IsiSensorMode_t pimx415_mode_info[] = {
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
        .bitWidth     = 12,
        .bayerPattern = ISI_BPAT_GBRG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
        .dphyFreq = 891000,
    },
    {
        .index         = 1,
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
        .fps           = 30 * ISI_FPS_QUANTIZE,
        .hdrMode      = ISI_SENSOR_MODE_LINEAR,
        .bitWidth     = 12,
        .bayerPattern = ISI_BPAT_GBRG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
        .dphyFreq = 891000,
    },
};

static RESULT IMX415_IsiReadRegIss(IsiSensorHandle_t handle,
        const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG, "%s (enter)\n", __func__);

    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    if (pIMX415Ctx == NULL || pIMX415Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pIMX415Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(IMX415_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(IMX415_DEBUG, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT IMX415_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr,
        const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG, "%s (enter)\n", __func__);

    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    if (pIMX415Ctx == NULL || pIMX415Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pIMX415Ctx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(IMX415_ERROR, "%s: hal write sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(IMX415_DEBUG, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT IMX415_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(IMX415_DEBUG, "%s (enter)\n", __func__);

    const IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    if (pIMX415Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pIMX415Ctx->sensorMode), sizeof(pIMX415Ctx->sensorMode));

    TRACE(IMX415_DEBUG, "%s (exit)\n", __func__);
    return ( RET_SUCCESS );
}

static  RESULT IMX415_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(IMX415_DEBUG, "%s (enter) with index %d\n", __func__, pEnumMode->index);
    const IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    if (pIMX415Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pimx415_mode_info)/sizeof(pimx415_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pimx415_mode_info)/sizeof(pimx415_mode_info[0])); i++) {
        if (pimx415_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pimx415_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(IMX415_DEBUG, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT IMX415_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(IMX415_DEBUG, "%s (enter)\n", __func__);

    if (pIMX415Ctx == NULL) return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pIMX415Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pIMX415Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pIMX415Ctx->sensorMode.size.width;
    pCaps->resolution.height = pIMX415Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;
    pCaps->dphyFreq          = pIMX415Ctx->sensorMode.dphyFreq;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else if (pCaps->bitWidth == 8) {
         pCaps->mipiMode      = ISI_FORMAT_RAW_8;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(IMX415_DEBUG, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX415_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle) {
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG, "%s (enter)\n", __func__);

    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) malloc(sizeof(IMX415_Context_t));
    if (!pIMX415Ctx) {
        TRACE(IMX415_ERROR, "%s: Can't allocate imx415 context\n",__func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pIMX415Ctx, 0, sizeof(IMX415_Context_t));

    pIMX415Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pIMX415Ctx->groupHold          = BOOL_FALSE;
    pIMX415Ctx->oldGain            = 0;
    pIMX415Ctx->oldIntegrationTime = 0;
    pIMX415Ctx->configured         = BOOL_FALSE;
    pIMX415Ctx->streaming          = BOOL_FALSE;
    pIMX415Ctx->testPattern        = BOOL_FALSE;
    pIMX415Ctx->isAfpsRun          = BOOL_FALSE;
    pIMX415Ctx->sensorMode.index   = 0;

    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = IMX415_SLAVE_ADDR;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pIMX415Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr,
            sccbConfig.addrByte, sccbConfig.dataByte);
    if (pIMX415Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(IMX415_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pIMX415Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pIMX415Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pIMX415Ctx;

    TRACE(IMX415_DEBUG, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX415_AecSetModeParameters(IsiSensorHandle_t handle, IMX415_Context_t *pIMX415Ctx)
{
    uint32_t regVal = 0;
    uint16_t value = 0;
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG, "%s%s: (enter)\n", __func__, pIMX415Ctx->isAfpsRun ? "(AFPS)" : "");

    pIMX415Ctx->aecMinIntegrationTime = (1
            * ((float)pIMX415Ctx->LineLengthPck) )
        / 4 / pIMX415Ctx->VtPixClkFreq;
    pIMX415Ctx->aecMaxIntegrationTime = ( ((float)pIMX415Ctx->frameLengthLines)
            * ((float)pIMX415Ctx->LineLengthPck) )
        / 4 / pIMX415Ctx->VtPixClkFreq;
    pIMX415Ctx->aecIntegrationTimeIncrement =
        (float)pIMX415Ctx->LineLengthPck / (float)pIMX415Ctx->VtPixClkFreq;

    TRACE(IMX415_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__,
            pIMX415Ctx->isAfpsRun ? "(AFPS)" : "",pIMX415Ctx->aecMaxIntegrationTime);

    pIMX415Ctx->aecGainIncrement = IMX415_MIN_AEC_STEP;

    //reflects the state of the sensor registers, must equal default settings
    pIMX415Ctx->oldGain               = 0;
    pIMX415Ctx->oldIntegrationTime    = 0;

    IMX415_IsiReadRegIss(handle, 0x3050, &value);
    regVal = (value & 0xff);
    IMX415_IsiReadRegIss(handle, 0x3051, &value);
    regVal = regVal | (value & 0xff) << 8;
    IMX415_IsiReadRegIss(handle, 0x3052, &value);
    regVal = regVal | (value & 0xf) << 16;
    pIMX415Ctx->aecCurIntegrationTime = regVal * pIMX415Ctx->oneLineExpTime;

    TRACE(IMX415_DEBUG, "%s%s: (exit)\n", __func__, pIMX415Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static void set_gain_value(IsiSensorHandle_t handle,
        IMX415_Context_t *pIMX415Ctx, float NewGain)
{
    float    Gain = 0.0f;
    uint32_t uiGain = 0U;

    if (NewGain < pIMX415Ctx->aecMinGain) NewGain = pIMX415Ctx->aecMinGain;
    if (NewGain > pIMX415Ctx->aecMaxGain) NewGain = pIMX415Ctx->aecMaxGain;

    Gain = ((float)(NewGain * IMX415_MIN_REG_STEP));
    uiGain = (uint32_t)(Gain);

    if (Gain != pIMX415Ctx->oldGain) {
        if (uiGain < IMX415_MIN_REG_VAL) uiGain = IMX415_MIN_REG_VAL;
        if (uiGain > IMX415_MAX_REG_VAL) uiGain = IMX415_MAX_REG_VAL;

        IMX415_IsiWriteRegIss(handle, 0x3090, uiGain & 0x00FF);
        IMX415_IsiWriteRegIss(handle, 0x3091, ((uiGain & 0x0100) >> 8));

        pIMX415Ctx->oldGain = Gain;
    }
}

static float get_gain_value(IsiSensorHandle_t handle,
        IMX415_Context_t *pIMX415Ctx)
{
    uint32_t uiGain = 0U;
    uint16_t lowByte = 0U;
    uint16_t highByte = 0U;
    float Gain = 0.0f;

    IMX415_IsiReadRegIss(handle, 0x3090, &lowByte);
    IMX415_IsiReadRegIss(handle, 0x3091, &highByte);

    uiGain = (highByte << 8) | (lowByte & 0x00FF);
    Gain = (float)(uiGain) / IMX415_MIN_REG_STEP;

    if (Gain < pIMX415Ctx->aecMinGain) Gain = pIMX415Ctx->aecMinGain;
    if (Gain > pIMX415Ctx->aecMaxGain) Gain = pIMX415Ctx->aecMaxGain;

    return Gain;
}

static RESULT IMX415_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    uint16_t regVal;

    TRACE(IMX415_DEBUG, "%s (enter)\n", __func__);

    if (!pIMX415Ctx) {
        TRACE(IMX415_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pIMX415Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pIMX415Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pimx415_mode_info)/ sizeof(IsiSensorMode_t); i++) {
        if (pimx415_mode_info[i].index == pIMX415Ctx->sensorMode.index) {
            SensorDefaultMode = &(pimx415_mode_info[i]);
            break;
        }
    }

    TRACE(IMX415_DEBUG, "%s SensorDefaultMode.index = %d\n", __func__, SensorDefaultMode->index);

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
        case 0:
            for (int i = 0; i<sizeof(IMX415_REG_3840) / sizeof(IMX415_REG_3840[0]); i++) {
                    IMX415_IsiWriteRegIss(handle, IMX415_REG_3840[i][0], IMX415_REG_3840[i][1]);

                    result = IMX415_IsiReadRegIss(handle, IMX415_REG_3840[i][0], &regVal);
                }
            break;
        case 1:
            for (int i = 0; i<sizeof(IMX415_REG_1920) / sizeof(IMX415_REG_1920[0]); i++) {
                    IMX415_IsiWriteRegIss(handle, IMX415_REG_1920[i][0], IMX415_REG_1920[i][1]);
                }
            break;
        default:
            TRACE(IMX415_DEBUG, "%s:not support sensor mode %d\n",
                    __func__, pIMX415Ctx->sensorMode.index);
            return RET_NOTSUPP;
       }

        memcpy(&(pIMX415Ctx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));
    } else {
        TRACE(IMX415_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    TRACE(IMX415_DEBUG, "%s pIMX415Ctx->sensorMode.index = %d\n",
            __func__, pIMX415Ctx->sensorMode.index);
    pIMX415Ctx->VtPixClkFreq     = 126880000.0f;
    pIMX415Ctx->LineLengthPck    = 0x14E8;
    /* set the default gain to 16, this will ensure we get a decent raw capture.
     * For ISP post processed formats, the AE feature will kick in and correct the gain */
    pIMX415Ctx->aecCurGain       = 16.0;

    switch(pIMX415Ctx->sensorMode.index) {
        case 0:
            pIMX415Ctx->oneLineExpTime      = 0.000010545;
            pIMX415Ctx->frameLengthLines    = 0xC56;
            pIMX415Ctx->curFrameLengthLines = pIMX415Ctx->frameLengthLines;
            pIMX415Ctx->maxIntegrationLine  = pIMX415Ctx->frameLengthLines - 8;
            pIMX415Ctx->minIntegrationLine  = 4;
            pIMX415Ctx->aecMaxGain          = IMX415_MAX_AEC_VAL;
            pIMX415Ctx->aecMinGain          = IMX415_MIN_AEC_VAL;
            pIMX415Ctx->aGain.min           = IMX415_MIN_AGAIN_VAL;
            pIMX415Ctx->aGain.max           = IMX415_MAX_AGAIN_VAL;
            pIMX415Ctx->aGain.step          = IMX415_MIN_AGAIN_STEP;
            pIMX415Ctx->dGain.min           = 1.0;
            pIMX415Ctx->dGain.max           = 1.0;
            pIMX415Ctx->dGain.step          = 1.0;
            break;
        case 1:
            pIMX415Ctx->oneLineExpTime      = 0.00001876;
            pIMX415Ctx->frameLengthLines    = 0x6F0;
            pIMX415Ctx->curFrameLengthLines = pIMX415Ctx->frameLengthLines;
            pIMX415Ctx->maxIntegrationLine  = pIMX415Ctx->curFrameLengthLines - 8;
            pIMX415Ctx->minIntegrationLine  = 4;
            pIMX415Ctx->aecMaxGain          = IMX415_MAX_AEC_VAL;
            pIMX415Ctx->aecMinGain          = IMX415_MIN_AEC_VAL;
            pIMX415Ctx->aGain.min           = IMX415_MIN_AGAIN_VAL;
            pIMX415Ctx->aGain.max           = IMX415_MAX_AGAIN_VAL;
            pIMX415Ctx->aGain.step          = IMX415_MIN_AGAIN_STEP;
            pIMX415Ctx->dGain.min           = 1.0;
            pIMX415Ctx->dGain.max           = 1.0;
            pIMX415Ctx->dGain.step          = 1.0;
            break;
        default:
            TRACE(IMX415_DEBUG,  "%s:not support sensor mode %d\n",
                    __func__, pIMX415Ctx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    set_gain_value(handle, pIMX415Ctx, pIMX415Ctx->aecCurGain);

    pIMX415Ctx->maxFps  = pIMX415Ctx->sensorMode.fps;
    pIMX415Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pIMX415Ctx->currFps = pIMX415Ctx->maxFps;

    TRACE(IMX415_DEBUG, "%s: IMX415 System-Reset executed\n", __func__);
    osSleep(100);

    result = IMX415_AecSetModeParameters(handle, pIMX415Ctx);
    if (result != RET_SUCCESS) {
        TRACE(IMX415_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pIMX415Ctx->configured = BOOL_TRUE;
    TRACE(IMX415_DEBUG, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT IMX415_IsiCloseIss(IsiSensorHandle_t handle)
{
    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG,  "%s (enter)\n", __func__);

    if (pIMX415Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)IMX415_IsiSetStreamingIss(pIMX415Ctx, BOOL_FALSE);

    TRACE(IMX415_DEBUG, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX415_IsiReleaseIss(IsiSensorHandle_t handle)
{
    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG, "%s (enter)\n", __func__);

    if (pIMX415Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pIMX415Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pIMX415Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pIMX415Ctx, 0, sizeof(IMX415_Context_t));
    free(pIMX415Ctx);

    TRACE(IMX415_DEBUG, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX415_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;

    uint32_t sensorId = 0;

    TRACE(IMX415_DEBUG,  "%s (enter)\n", __func__);

    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    if (pIMX415Ctx == NULL || pIMX415Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = IMX415_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(IMX415_ERROR, "%s: Read Sensor ID Error! \n", __func__);
        return (RET_FAILURE);
    }

    TRACE(IMX415_DEBUG, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX415_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(IMX415_DEBUG, "%s (enter)\n", __func__);

    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    if (pIMX415Ctx == NULL || pIMX415Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    TRACE(IMX415_DEBUG,  "%s (enter) offset 0x0016\n", __func__);
    regVal = 0;
    result = IMX415_IsiReadRegIss(handle, 0x0016, &regVal);
    sensorId = (regVal & 0xff) << 8;

    regVal = 0;
    result |= IMX415_IsiReadRegIss(handle, 0x0017, &regVal);
    sensorId |= (regVal & 0xff);

    *pValue = sensorId;
    TRACE(IMX415_DEBUG, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX415_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on) {
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG,  "%s (enter)\n", __func__);

    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    if (pIMX415Ctx == NULL || pIMX415Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pIMX415Ctx->configured != BOOL_TRUE) return RET_WRONG_STATE;

    result = IMX415_IsiWriteRegIss(handle, 0x3000, !on);
    if (result != RET_SUCCESS) {
       return (RET_FAILURE);
    }
    //TODO: Revisit the delay
    osSleep( 300 );
    if(on) {
        result = IMX415_IsiWriteRegIss(handle, 0x3002, 0x00);
        if (result != RET_SUCCESS) {
             TRACE(IMX415_ERROR, "%s: set sensor streaming error! \n",__func__);
             return (RET_FAILURE);
        }
    }

    pIMX415Ctx->streaming = on;

    TRACE(IMX415_DEBUG, "%s (exit)\n", __func__);
    return (result);
}

static RESULT IMX415_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX415_DEBUG, "%s: (enter)\n", __func__);

    if (pIMX415Ctx == NULL) {
        TRACE(IMX415_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(IMX415_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    //get time limit and total gain limit
    pAeBaseInfo->gain.min        = pIMX415Ctx->aecMinGain;
    pAeBaseInfo->gain.max        = pIMX415Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pIMX415Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pIMX415Ctx->aecMaxIntegrationTime;

    //get again/dgain info
    pAeBaseInfo->aGain           = pIMX415Ctx->aGain;
    pAeBaseInfo->dGain           = pIMX415Ctx->dGain;

    pAeBaseInfo->aecCurGain     = pIMX415Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime  = pIMX415Ctx->aecCurIntegrationTime;
    pAeBaseInfo->aecGainStep    = pIMX415Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep = pIMX415Ctx->aecIntegrationTimeIncrement;

    TRACE(IMX415_DEBUG, "%s: with gain %f and intg %f \n", __func__,
            pAeBaseInfo->aecCurGain, pAeBaseInfo->aecCurIntTime);

    TRACE(IMX415_DEBUG, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX415_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG, "%s: (enter)\n", __func__);

    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    if (pIMX415Ctx == NULL || pIMX415Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    set_gain_value(handle, pIMX415Ctx, pSensorAGain->gain[ISI_LINEAR_PARAS]);
    pIMX415Ctx->aecCurGain = get_gain_value(handle, pIMX415Ctx);
    TRACE(IMX415_DEBUG, "%s: AGain=%f\n", __func__, pIMX415Ctx->aecCurGain);

    TRACE(IMX415_DEBUG, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX415_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG,  "%s: (enter)\n", __func__);

    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    if (pIMX415Ctx == NULL || pIMX415Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    pIMX415Ctx->curDgain = 0;
    TRACE(IMX415_DEBUG, "Set NewGain %f\n", pSensorDGain->gain[ISI_LINEAR_PARAS]);

    TRACE(IMX415_DEBUG, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX415_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    const IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG,  "%s: (enter)\n", __func__);

    if (pIMX415Ctx == NULL) {
        TRACE(IMX415_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorAGain->gain[ISI_LINEAR_PARAS] = pIMX415Ctx->aecCurGain;

    TRACE(IMX415_DEBUG, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX415_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    const IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG, "%s: (enter)\n", __func__);

    if (pIMX415Ctx == NULL) {
        TRACE(IMX415_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    pSensorDGain->gain[ISI_LINEAR_PARAS] = pIMX415Ctx->curDgain;

    TRACE(IMX415_DEBUG, "%s: (exit)\n", __func__);
    return (result);
}

static void set_integration_time(void *handle, IMX415_Context_t *pIMX415Ctx,
        float NewIntegrationTime) {

    uint32_t StorageTime = 0UL;
    float Tline = 0.0f;

    if (NewIntegrationTime < IMX415_MIN_INTEGRATION_TIME)
        NewIntegrationTime = IMX415_MIN_INTEGRATION_TIME;
    if (NewIntegrationTime > IMX415_MAX_INTEGRATION_TIME)
        NewIntegrationTime = IMX415_MAX_INTEGRATION_TIME;

    Tline = (NewIntegrationTime - IMX415_T_OFFSET_12B) / IMX415_H_PERIOD_30FPS;
    StorageTime = (uint32_t)(IMX415_FRAME_PERIOD - Tline);

    if(StorageTime < IMX415_MIN_STORAGE_TIME)
        StorageTime = IMX415_MIN_STORAGE_TIME;
    if(StorageTime > IMX415_MAX_STORAGE_TIME)
        StorageTime = IMX415_MAX_STORAGE_TIME;

    if (NewIntegrationTime != pIMX415Ctx->oldIntegrationTime) {
        IMX415_IsiWriteRegIss(handle, 0x3050, (StorageTime & 0x00FF));
        IMX415_IsiWriteRegIss(handle, 0x3051, ((StorageTime & 0xFF00) >> 8));
        IMX415_IsiWriteRegIss(handle, 0x3052, ((StorageTime & 0x0F0000) >> 16));

        pIMX415Ctx->oldIntegrationTime = NewIntegrationTime;
    }
}

static float get_integration_time(void *handle, IMX415_Context_t *pIMX415Ctx)
{
    uint32_t StorageTime = 0UL;
    float IntegrationTime = 0.0f;
    float Tline = 0.0f;
    uint16_t reg_3050 = 0, reg_3051 = 0, reg_3052 = 0;

    IMX415_IsiReadRegIss(handle, 0x3050, &reg_3050);
    IMX415_IsiReadRegIss(handle, 0x3051, &reg_3051);
    IMX415_IsiReadRegIss(handle, 0x3052, &reg_3052);

    StorageTime = (reg_3052 << 16) | (reg_3051 << 8) | reg_3050;

    if (StorageTime < IMX415_MIN_STORAGE_TIME)
        StorageTime = IMX415_MIN_STORAGE_TIME;
    if (StorageTime > IMX415_MAX_STORAGE_TIME)
        StorageTime = IMX415_MAX_STORAGE_TIME;

    Tline = IMX415_FRAME_PERIOD - (float)StorageTime;
    IntegrationTime = (Tline * IMX415_H_PERIOD_30FPS) + IMX415_T_OFFSET_12B;

    if (IntegrationTime < IMX415_MIN_INTEGRATION_TIME)
        IntegrationTime = IMX415_MIN_INTEGRATION_TIME;
    if (IntegrationTime > IMX415_MAX_INTEGRATION_TIME)
        IntegrationTime = IMX415_MAX_INTEGRATION_TIME;

    return IntegrationTime;
}

RESULT IMX415_IsiSetIntTimeIss(IsiSensorHandle_t handle,
        const IsiSensorIntTime_t *pSensorIntTime)
{
    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG, "%s: (enter)\n", __func__);

    if (!pIMX415Ctx) {
        TRACE(IMX415_ERROR,
                "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    set_integration_time(handle, pIMX415Ctx, pSensorIntTime->intTime[ISI_LINEAR_PARAS]);
    pIMX415Ctx->aecCurIntegrationTime = get_integration_time(handle, pIMX415Ctx);
    TRACE(IMX415_DEBUG, "%s: intTime:%f\n", __func__, pIMX415Ctx->aecCurIntegrationTime);

    TRACE(IMX415_DEBUG, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX415_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    const IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG,  "%s: (enter)\n", __func__);

    if (!pIMX415Ctx) {
        TRACE(IMX415_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    pSensorIntTime->intTime[ISI_LINEAR_PARAS] = pIMX415Ctx->aecCurIntegrationTime;

    TRACE(IMX415_DEBUG, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX415_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t *pFps)
{
    const IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG,  "%s: (enter)\n", __func__);

    if (pIMX415Ctx == NULL) {
        TRACE(IMX415_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pIMX415Ctx->currFps;

    TRACE(IMX415_DEBUG, "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX415_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    RESULT result = RET_SUCCESS;
    int32_t NewVts = 0;
    TRACE(IMX415_DEBUG,  "%s: (enter)\n", __func__);

    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    if (pIMX415Ctx == NULL) {
        TRACE(IMX415_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (fps > pIMX415Ctx->maxFps) {
        TRACE(IMX415_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__,
                             fps, pIMX415Ctx->maxFps, pIMX415Ctx->minFps,pIMX415Ctx->maxFps);
        fps = pIMX415Ctx->maxFps;
    }
    if (fps < pIMX415Ctx->minFps) {
        TRACE(IMX415_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__,
                            fps, pIMX415Ctx->minFps, pIMX415Ctx->minFps,pIMX415Ctx->maxFps);
        fps = pIMX415Ctx->minFps;
    }

    NewVts = pIMX415Ctx->frameLengthLines*pIMX415Ctx->sensorMode.fps / fps;

    pIMX415Ctx->currFps              = fps;
    pIMX415Ctx->curFrameLengthLines  = NewVts;
    pIMX415Ctx->maxIntegrationLine   = pIMX415Ctx->curFrameLengthLines - 8;

    TRACE(IMX415_DEBUG,  "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX415_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    const IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    if (pIMX415Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(IMX415_DEBUG,  "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = false;
    pIspStatus->useSensorAWB = false;

    TRACE(IMX415_DEBUG,  "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT IMX415_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG,  "%s: (enter)\n", __func__);

    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    if (pIMX415Ctx == NULL || pIMX415Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    TRACE(IMX415_DEBUG,  "%s: (exit)\n", __func__);
    return (result);
}
RESULT IMX415_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(IMX415_DEBUG,  "%s: (enter)\n", __func__);

    IMX415_Context_t *pIMX415Ctx = (IMX415_Context_t *) handle;
    if (pIMX415Ctx == NULL || pIMX415Ctx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    TRACE(IMX415_DEBUG,  "%s: (exit)\n", __func__);
    return (result);
}

RESULT IMX415_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "IMX415";
    TRACE( IMX415_DEBUG, "%s (enter)\n", __func__);

    if ( pIsiSensor != NULL ) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = IMX415_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = IMX415_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = IMX415_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = IMX415_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = IMX415_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = IMX415_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = IMX415_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = IMX415_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = IMX415_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = IMX415_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = IMX415_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = IMX415_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = IMX415_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = IMX415_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = IMX415_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = IMX415_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = IMX415_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = IMX415_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = IMX415_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = IMX415_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = IMX415_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = IMX415_IsiGetIspStatusIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = IMX415_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = IMX415_IsiGetTpgIss;

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

    TRACE( IMX415_DEBUG, "%s (exit)\n", __func__);
    return ( result );
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t IMX415_IsiCamDrvConfig = {
    .cameraDriverID      = 0x0000,
    .pIsiGetSensorIss    = IMX415_IsiGetSensorIss,
};
