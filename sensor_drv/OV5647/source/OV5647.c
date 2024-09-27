// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Synaptics Incorporated

#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <common/return_codes.h>
#include <common/misc.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "isi.h"
#include "isi_iss.h"
#include "isi_vvsensor.h"

#include "OV5647_priv.h"

CREATE_TRACER(OV5647_INFO , "OV5647: ", INFO,    1);
CREATE_TRACER(OV5647_WARN , "OV5647: ", WARNING, 1);
CREATE_TRACER(OV5647_ERROR, "OV5647: ", ERROR,   1);
CREATE_TRACER(OV5647_DEBUG,     "OV5647: ", INFO, 1);
CREATE_TRACER(OV5647_REG_INFO , "OV5647: ", INFO, 1);
CREATE_TRACER(OV5647_REG_DEBUG, "OV5647: ", INFO, 1);

#define OV5647_MIN_GAIN_STEP    (1.0f/4.0f)  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain) */
#define AGAIN_MAX   255.0f

/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pov5647_mode_info[] = {
    {
        .index     = 0,
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
        .fps       = 60 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_GRBG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
        .mipiLane = ISI_MIPI_2LANES,
        .dphyFreq = 280000,
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
        .fps      = 16 * ISI_FPS_QUANTIZE,
        .hdrMode = ISI_SENSOR_MODE_LINEAR,
        .bitWidth = 10,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
        .mipiLane = ISI_MIPI_2LANES,
        .dphyFreq = 408000,
    },
    {
        .index     = 2,
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
        .fps      = 30 * ISI_FPS_QUANTIZE,
        .hdrMode = ISI_SENSOR_MODE_LINEAR,
        .nativeMode = ISI_SENSOR_NATIVE_DCG,
        .bitWidth = 12,
        .compress.enable = 1,
        .compress.xBit  = 16,
        .compress.yBit  = 12,
        .bayerPattern = ISI_BPAT_GBRG,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
        .mipiLane = ISI_MIPI_2LANES,
    },
};

static RESULT OV5647_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    int32_t ret = 0;
    TRACE(OV5647_INFO, "%s (enter)\n", __func__);

    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL || pOV5647Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    ret = HalReadI2CReg(pOV5647Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (ret != 0) {
        TRACE(OV5647_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV5647_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV5647_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    int ret = 0;
    TRACE(OV5647_INFO, "%s (enter)\n", __func__);

    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL || pOV5647Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }
    ret = HalWriteI2CReg(pOV5647Ctx->isiCtx.halI2cHandle, addr, value);
    if (ret != 0) {
        TRACE(OV5647_ERROR, "%s: hal write sensor register error!\n",__func__);
        return (RET_FAILURE);
    }

    TRACE(OV5647_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT OV5647_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(OV5647_INFO, "%s (enter)\n", __func__);
    const OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    memcpy(pMode, &(pOV5647Ctx->sensorMode), sizeof(pOV5647Ctx->sensorMode));

    TRACE(OV5647_INFO, "%s (exit)\n", __func__);
    return (RET_SUCCESS);
}

static  RESULT OV5647_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    const OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pov5647_mode_info)/sizeof(pov5647_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pov5647_mode_info)/sizeof(pov5647_mode_info[0])); i++) {
        if (pov5647_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pov5647_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(OV5647_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT OV5647_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(OV5647_INFO, "%s (enter)\n", __func__);

    if (pOV5647Ctx == NULL)
        return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pOV5647Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pOV5647Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pOV5647Ctx->sensorMode.size.width;
    pCaps->resolution.height = pOV5647Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = pOV5647Ctx->sensorMode.mipiLane;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;
    pCaps->dphyFreq          = pOV5647Ctx->sensorMode.dphyFreq;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(OV5647_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV5647_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s (enter)\n", __func__);

    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) malloc(sizeof(OV5647_Context_t));
    if (!pOV5647Ctx) {
        TRACE(OV5647_ERROR, "%s: Can't allocate ov5647 context\n",__func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pOV5647Ctx, 0, sizeof(OV5647_Context_t));

    pOV5647Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pOV5647Ctx->groupHold          = BOOL_FALSE;
    pOV5647Ctx->oldGain            = 0;
    pOV5647Ctx->oldIntegrationTime = 0;
    pOV5647Ctx->configured         = BOOL_FALSE;
    pOV5647Ctx->streaming          = BOOL_FALSE;
    pOV5647Ctx->testPattern        = BOOL_FALSE;
    pOV5647Ctx->isAfpsRun          = BOOL_FALSE;
    pOV5647Ctx->sensorMode.index   = 0;

    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x36;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pOV5647Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pOV5647Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(OV5647_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

    result = HalAddRef(pOV5647Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pOV5647Ctx);
        return (result);
    }

    *pHandle = (IsiSensorHandle_t) pOV5647Ctx;

    TRACE(OV5647_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV5647_AecSetModeParameters(IsiSensorHandle_t handle, OV5647_Context_t * pOV5647Ctx)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s%s: (enter)\n", __func__,pOV5647Ctx->isAfpsRun ? "(AFPS)" : "");
    uint16_t value = 0;
    uint32_t regVal = 0;
    float again = 0, dgain = 0;

    pOV5647Ctx->aecIntegrationTimeIncrement = pOV5647Ctx->oneLineExpTime;
    pOV5647Ctx->aecMinIntegrationTime       = pOV5647Ctx->oneLineExpTime * pOV5647Ctx->minIntegrationLine;
    pOV5647Ctx->aecMaxIntegrationTime       = pOV5647Ctx->oneLineExpTime * pOV5647Ctx->maxIntegrationLine;

    TRACE(OV5647_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__, pOV5647Ctx->isAfpsRun ? "(AFPS)" : "",pOV5647Ctx->aecMaxIntegrationTime);

    pOV5647Ctx->aecGainIncrement = OV5647_MIN_GAIN_STEP;

    // Get current exposure time set by default
    OV5647_IsiReadRegIss(handle, 0x3500, &value);
    regVal = (value << 16) & 0xff0000;
    OV5647_IsiReadRegIss(handle, 0x3501, &value);
    regVal = regVal | ( (value << 8) & 0xff00);
    OV5647_IsiReadRegIss(handle, 0x3502, &value);
    regVal = regVal | (value & 0xff);
    pOV5647Ctx->aecCurIntegrationTime = (regVal >> 4) * pOV5647Ctx->oneLineExpTime;
    pOV5647Ctx->aecCurLongIntegrationTime = (regVal >> 4) * pOV5647Ctx->oneLineExpTime;

    if(pOV5647Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR){
        // Get current gain set by default
        OV5647_IsiReadRegIss(handle, 0x350a, &value);
        regVal = (value & 0xff) << 8;
        OV5647_IsiReadRegIss(handle, 0x350b, &value);
        regVal = regVal | (value & 0xff);
        again = regVal / 4.0f;

        dgain = 1;
        pOV5647Ctx->aecCurGain = again * dgain;
    } else {
        /* HCG analog gain and digital gain*/
        OV5647_IsiReadRegIss(handle, 0x350a, &value);
        regVal = (value & 0xff) << 8;
        OV5647_IsiReadRegIss(handle, 0x350b, &value);
        regVal = regVal | (value & 0xff);
        again = regVal / 4.0f;

        dgain = 1;
        pOV5647Ctx->aecCurGain = again * dgain;//dpf limit
        pOV5647Ctx->aecCurLongGain = again * dgain;
    }
    pOV5647Ctx->oldGain               = 0;
    pOV5647Ctx->oldIntegrationTime    = 0;

    TRACE(OV5647_INFO, "%s%s: (exit)\n", __func__,pOV5647Ctx->isAfpsRun ? "(AFPS)" : "");

    return (result);
}

static RESULT OV5647_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    uint16_t regValue = 0;
    uint16_t vts = 0;
    float maxExpTime = 0;

    TRACE(OV5647_INFO, "%s (enter)\n", __func__);

    if (!pOV5647Ctx) {
        TRACE(OV5647_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOV5647Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pOV5647Ctx->sensorMode.index   = mode;

    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i = 0; i < sizeof(pov5647_mode_info) / sizeof(IsiSensorMode_t); i++) {
        if (pov5647_mode_info[i].index == pOV5647Ctx->sensorMode.index) {
            SensorDefaultMode = &(pov5647_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(OV5647_mipi2lane_480p_init) / sizeof(OV5647_mipi2lane_480p_init[0]); i++) {
                    OV5647_IsiWriteRegIss(handle, OV5647_mipi2lane_480p_init[i][0], OV5647_mipi2lane_480p_init[i][1]);
                }
                break;
            case 1: //2k
                for (int i = 0; i<sizeof(OV5647_mipi2lane_1080p_init) / sizeof(OV5647_mipi2lane_1080p_init[0]); i++) {
                    OV5647_IsiWriteRegIss(handle, OV5647_mipi2lane_1080p_init[i][0], OV5647_mipi2lane_1080p_init[i][1]);
                }
                break;
            case 2: //native 2dol mode
                for (int i = 0; i<sizeof(OV5647_mipi2lane_1440p_init) / sizeof(OV5647_mipi2lane_1440p_init[0]); i++) {
                    OV5647_IsiWriteRegIss(handle, OV5647_mipi2lane_1440p_init[i][0], OV5647_mipi2lane_1440p_init[i][1]);
                }
                break;
            default:
                TRACE(OV5647_INFO, "%s:not support sensor mode %d\n", __func__,pOV5647Ctx->sensorMode.index);
                return RET_NOTSUPP;
                break;
    }

        memcpy(&(pOV5647Ctx->sensorMode),SensorDefaultMode,sizeof(IsiSensorMode_t));
    } else {
        TRACE(OV5647_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }
    OV5647_IsiReadRegIss(handle, 0x380e, &regValue);
    vts = (regValue << 8 ) & 0xff00;
    OV5647_IsiReadRegIss(handle, 0x380f, &regValue);
    vts |= regValue & 0xff;
    maxExpTime = ISI_FPS_QUANTIZE / (float)pOV5647Ctx->sensorMode.fps;
    switch(pOV5647Ctx->sensorMode.index) {
        case 0:
        case 1:
            pOV5647Ctx->oneLineExpTime      = maxExpTime / vts;
            pOV5647Ctx->frameLengthLines    = vts;
            pOV5647Ctx->curFrameLengthLines = pOV5647Ctx->frameLengthLines;
            pOV5647Ctx->maxIntegrationLine  = pOV5647Ctx->curFrameLengthLines - 4; // -4 as seen in sample code
            pOV5647Ctx->minIntegrationLine  = 1;
            pOV5647Ctx->aecMaxGain          = AGAIN_MAX;
            pOV5647Ctx->aecMinGain          = 4.0;
            pOV5647Ctx->aGain.min           = 4.0;
            pOV5647Ctx->aGain.max           = AGAIN_MAX;
            pOV5647Ctx->aGain.step          = (1.0f/4.0f); //Gain=0x350a[1:0], 0x350b[7:0]/16
            pOV5647Ctx->dGain.min           = 1.0;
            pOV5647Ctx->dGain.max           = 1.0;
            pOV5647Ctx->dGain.step          = (1.0f/256.0f);
            break;
        case 2:
            pOV5647Ctx->oneLineExpTime      = 0.000069607;
            pOV5647Ctx->frameLengthLines    = 0x466;
            pOV5647Ctx->curFrameLengthLines = pOV5647Ctx->frameLengthLines;
            pOV5647Ctx->maxIntegrationLine  = pOV5647Ctx->curFrameLengthLines - 4;
            pOV5647Ctx->minIntegrationLine  = 1;
            pOV5647Ctx->aecMaxGain          = 24;
            pOV5647Ctx->aecMinGain          = 3;
            pOV5647Ctx->aGain.min           = 3.0;
            pOV5647Ctx->aGain.max           = 24;
            pOV5647Ctx->aGain.step          = (1.0f/256.0f);
            pOV5647Ctx->dGain.min           = 1.0;
            pOV5647Ctx->dGain.max           = 1.0;
            pOV5647Ctx->dGain.step          = (1.0f/256.0f);
            break;
        default:
            return (RET_NOTAVAILABLE);
            break;
    }

    pOV5647Ctx->maxFps  = pOV5647Ctx->sensorMode.fps;
    pOV5647Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pOV5647Ctx->currFps = pOV5647Ctx->maxFps;

    result = OV5647_AecSetModeParameters(handle, pOV5647Ctx);
    if (result != RET_SUCCESS) {
        TRACE(OV5647_ERROR, "%s: SetupOutputWindow failed.\n",__func__);
        return (result);
    }

    pOV5647Ctx->configured = BOOL_TRUE;
    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT OV5647_IsiCloseIss(IsiSensorHandle_t handle)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s (enter)\n", __func__);

    if (pOV5647Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)OV5647_IsiSetStreamingIss(pOV5647Ctx, BOOL_FALSE);

    TRACE(OV5647_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV5647_IsiReleaseIss(IsiSensorHandle_t handle)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s (enter)\n", __func__);

    if (pOV5647Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pOV5647Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pOV5647Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pOV5647Ctx, 0, sizeof(OV5647_Context_t));
    free(pOV5647Ctx);
    TRACE(OV5647_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV5647_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;
    uint32_t correctId = 0x5647;
    uint32_t sensorId = 0;
    TRACE(OV5647_INFO, "%s (enter)\n", __func__);

    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL || pOV5647Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = OV5647_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(OV5647_ERROR, "%s: Read Sensor ID Error! \n",__func__);
        return (RET_FAILURE);
    }

    if (correctId != sensorId) {
        TRACE(OV5647_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n", __func__, correctId, sensorId);
        return (RET_FAILURE);
    }

    TRACE(OV5647_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__, correctId, sensorId);
    TRACE(OV5647_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV5647_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t * pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t regVal;
    uint32_t sensorId;

    TRACE(OV5647_INFO, "%s (enter)\n", __func__);

    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL || pOV5647Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (!pValue) return (RET_NULL_POINTER);

    regVal = 0;
    result = OV5647_IsiReadRegIss(handle, 0x300a, &regVal);
    sensorId = (regVal & 0xff) << 8;

    regVal = 0;
    result |= OV5647_IsiReadRegIss(handle, 0x300b, &regVal);
    sensorId |= (regVal & 0xff);

    *pValue = sensorId;
    TRACE(OV5647_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV5647_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;
    int ret = 0;
    TRACE(OV5647_INFO, "%s (enter)\n", __func__);

    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL || pOV5647Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV5647Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    ret = OV5647_IsiWriteRegIss(handle, 0x100, on);

    if (ret != 0) {
        return (RET_FAILURE);
    }

    pOV5647Ctx->streaming = on;

    TRACE(OV5647_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT OV5647_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);

    if (pOV5647Ctx == NULL) {
        TRACE(OV5647_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(OV5647_ERROR, "%s: NULL pointer received!!\n", __func__);
        return (RET_NULL_POINTER);
    }

    /* linear and native2dol*/
    pAeBaseInfo->gain.min        = pOV5647Ctx->aecMinGain;
    pAeBaseInfo->gain.max        = pOV5647Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pOV5647Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pOV5647Ctx->aecMaxIntegrationTime;

    pAeBaseInfo->aGain           = pOV5647Ctx->aGain;
    pAeBaseInfo->dGain           = pOV5647Ctx->dGain;

    pAeBaseInfo->aecCurGain      = pOV5647Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime   = pOV5647Ctx->aecCurIntegrationTime;
    pAeBaseInfo->aecGainStep     = pOV5647Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep  = pOV5647Ctx->aecIntegrationTimeIncrement;

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV5647_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);
    float  newGain;
    uint32_t  regValue;

    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL || pOV5647Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV5647Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR) {
        /* base LCG, set linear LCG channel total gain*/
        newGain = pSensorAGain->gain[ISI_LINEAR_PARAS];
        if (newGain < pOV5647Ctx->aecMinGain) {
            newGain = pOV5647Ctx->aecMinGain;
        } else if (newGain > pOV5647Ctx->aecMaxGain) {
            newGain = pOV5647Ctx->aecMaxGain;
        }

        regValue = newGain * 4;

        result = OV5647_IsiWriteRegIss(handle, 0x350a, (regValue >> 8) & 0x3);
        result |= OV5647_IsiWriteRegIss(handle, 0x350b, regValue & 0xff);

        pOV5647Ctx->curAgain   = (float)regValue / 4.0f;
        pOV5647Ctx->curDgain   = 1.0f;
        pOV5647Ctx->aecCurGain = pOV5647Ctx->curAgain * pOV5647Ctx->curDgain;

    } else {
        TRACE(OV5647_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV5647_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV5647_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);

    if (pOV5647Ctx == NULL) {
        TRACE(OV5647_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    if(pOV5647Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR || pOV5647Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pSensorAGain->gain[ISI_LINEAR_PARAS]       = pOV5647Ctx->curAgain;

    } else {
        TRACE(OV5647_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV5647_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);

    if (pOV5647Ctx == NULL) {
        TRACE(OV5647_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    if (pOV5647Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR || pOV5647Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pSensorDGain->gain[ISI_LINEAR_PARAS] = pOV5647Ctx->curDgain;

    } else {
        TRACE(OV5647_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV5647_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;

    if (!pOV5647Ctx) {
        TRACE(OV5647_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOV5647Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR || pOV5647Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        result = OV5647_SetIntTime(handle, pSensorIntTime->intTime[ISI_LINEAR_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(OV5647_INFO, "%s: set sensor IntTime[ISI_LINEAR_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

    }  else {
        TRACE(OV5647_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV5647_SetIntTime(IsiSensorHandle_t handle, float newIntegrationTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    uint32_t expLine = 0;
    uint32_t lineUnitExp = 0;

    if (!pOV5647Ctx) {
        TRACE(OV5647_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    expLine = newIntegrationTime / pOV5647Ctx->oneLineExpTime;
    expLine = MIN(pOV5647Ctx->maxIntegrationLine, MAX(pOV5647Ctx->minIntegrationLine, expLine));

    // Exposure setting (Group Write) - Start
    result |= OV5647_IsiWriteRegIss(handle, 0x3208, 0x00);

    lineUnitExp = (expLine << 4); // Exposure in units of 1/16 line. Hence multiply by 16 to make it line unit
    result |= OV5647_IsiWriteRegIss(handle, 0x3500, (lineUnitExp >> 16) & 0xff);
    result |= OV5647_IsiWriteRegIss(handle, 0x3501, (lineUnitExp >> 8) & 0xff);
    // Lower 4bits are fraction of a line; they should be 0 since OV5647 does not support fraction
    //  line exposure
    result |= OV5647_IsiWriteRegIss(handle, 0x3502, lineUnitExp & 0xf0);

    result |= OV5647_IsiWriteRegIss(handle, 0x3208, 0x10);
    result |= OV5647_IsiWriteRegIss(handle, 0x3208, 0xA0);
    // Exposure setting - End

    pOV5647Ctx->aecCurIntegrationTime     = expLine * pOV5647Ctx->oneLineExpTime;
    pOV5647Ctx->aecCurLongIntegrationTime = expLine * pOV5647Ctx->oneLineExpTime;

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV5647_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);

    if (!pOV5647Ctx) {
        TRACE(OV5647_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);

    if (pOV5647Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR || pOV5647Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pSensorIntTime->intTime[ISI_TRI_EXP_S_PARAS] = pOV5647Ctx->aecCurIntegrationTime;

    } else {
        TRACE(OV5647_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV5647_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t * pFps)
{
    const OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);

    if (pOV5647Ctx == NULL) {
        TRACE(OV5647_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pOV5647Ctx->currFps;

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV5647_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);

    if (pOV5647Ctx == NULL) {
        TRACE(OV5647_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (fps > pOV5647Ctx->maxFps) {
        TRACE(OV5647_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, fps, pOV5647Ctx->maxFps, pOV5647Ctx->minFps,pOV5647Ctx->maxFps);
        fps = pOV5647Ctx->maxFps;
    }
    if (fps < pOV5647Ctx->minFps) {
        TRACE(OV5647_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, fps, pOV5647Ctx->minFps, pOV5647Ctx->minFps,pOV5647Ctx->maxFps);
        fps = pOV5647Ctx->minFps;
    }

#if 0
    uint16_t FrameLengthLines;
    FrameLengthLines = pOV5647Ctx->frameLengthLines * pOV5647Ctx->maxFps / fps;
    result = OV5647_IsiWriteRegIss(handle, 0x30b2,(FrameLengthLines >> 8) & 0xff);
    result |= OV5647_IsiWriteRegIss(handle, 0x30b3, FrameLengthLines & 0xff);
    if (result != RET_SUCCESS) {
        TRACE(OV5647_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_FAILURE);
    }
    pOV5647Ctx->currFps               = fps;
    pOV5647Ctx->curFrameLengthLines   = FrameLengthLines;
    pOV5647Ctx->maxIntegrationLine    = pOV5647Ctx->curFrameLengthLines - 3;
    pOV5647Ctx->aecMaxIntegrationTime = pOV5647Ctx->maxIntegrationLine * pOV5647Ctx->oneLineExpTime;

    TRACE(OV5647_INFO, "%s: set sensor fps = %d\n", __func__,pOV5647Ctx->currFps);
#endif

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV5647_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorAWB = false;
    pIspStatus->useSensorBLC = false;

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

static RESULT OV5647_IsiSetBlcIss(IsiSensorHandle_t handle, const IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);

//    uint32_t rOffset=0, grOffset=0, gbOffset=0, bOffset=0;
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL || pOV5647Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)
        return RET_NULL_POINTER;

    float rGain=0, grGain=0, gbGain=0, bGain=0;
    rGain  = pOV5647Ctx->sensorWb.rGain;
    grGain = pOV5647Ctx->sensorWb.grGain;
    gbGain = pOV5647Ctx->sensorWb.gbGain;
    bGain  = pOV5647Ctx->sensorWb.bGain;

    if (rGain  < 1)  rGain  = 1;
    if (grGain < 1)  grGain = 1;
    if (gbGain < 1)  gbGain = 1;
    if (bGain  < 1)  bGain  = 1;

    memcpy(&pOV5647Ctx->sensorBlc, pBlc, sizeof(IsiSensorBlc_t));
    //TODO: Sensor WB Currently not supported

    if (result != RET_SUCCESS) {
        TRACE(OV5647_ERROR, "%s: set sensor blc error\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV5647_IsiGetBlcIss(IsiSensorHandle_t handle, IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);

    const OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)  return RET_NULL_POINTER;

    memcpy(pBlc, &pOV5647Ctx->sensorBlc, sizeof(IsiSensorBlc_t));

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV5647_IsiSetWBIss(IsiSensorHandle_t handle, const IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);

    //bool update_flag = false;
    //uint32_t rGain, grGain, gbGain, bGain;
    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL || pOV5647Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)
        return RET_NULL_POINTER;
    //TODO: Sensor WB Currently not supported

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV5647_IsiGetWBIss(IsiSensorHandle_t handle, IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);

    const OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL ) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)  return RET_NULL_POINTER;

    memcpy(pWb, &pOV5647Ctx->sensorWb, sizeof(IsiSensorWb_t));

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV5647_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);

    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL || pOV5647Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV5647Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = OV5647_IsiWriteRegIss(handle, 0x503d, 0x00);
    } else {
        result = OV5647_IsiWriteRegIss(handle, 0x503d, 0x80);
    }
    pOV5647Ctx->testPattern = tpg.enable;

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV5647_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);

    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL || pOV5647Ctx->isiCtx.halI2cHandle == NULL || pTpg == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOV5647Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!OV5647_IsiReadRegIss(handle, 0x503d, &value)) {
        pTpg->enable = ((value & 0x80) != 0) ? 1 : 0;
        if(pTpg->enable) {
           pTpg->pattern = (0xff & value);
        }
      pOV5647Ctx->testPattern = pTpg->enable;
    }

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT OV5647_IsiGetExpandCurveIss(IsiSensorHandle_t handle, IsiSensorCompandCurve_t *pCurve)
{
    RESULT result = RET_SUCCESS;
    TRACE(OV5647_INFO, "%s: (enter)\n", __func__);

    OV5647_Context_t *pOV5647Ctx = (OV5647_Context_t *) handle;
    if (pOV5647Ctx == NULL || pOV5647Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    // TODO: Refer to the below logic and implement compand module for ov5647 if needed
    uint8_t expandPx[64] = {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6};
    memcpy(pCurve->compandPx, expandPx, sizeof(expandPx));

    pCurve->compandXData[0] = 0;
    pCurve->compandYData[0] = 0;
    for(int i = 1; i < 65; i++) {
        pCurve->compandXData[i] = (1 << pCurve->compandPx[i-1]) + pCurve->compandXData[i-1];

        if(pCurve->compandXData[i] < 512){
            pCurve->compandYData[i] = pCurve->compandXData[i] << 1;//*2
        } else if (pCurve->compandXData[i] < 768){
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 256) << 2;//*4
        } else if (pCurve->compandXData[i] < 2560){
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 512) << 3;//*8
        } else {
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 2048) << 5;//*32
        }
    }

    TRACE(OV5647_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT OV5647_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "OV5647";
    TRACE(OV5647_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = OV5647_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = OV5647_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = OV5647_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = OV5647_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = OV5647_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = OV5647_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = OV5647_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = OV5647_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = OV5647_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = OV5647_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = OV5647_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = OV5647_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = OV5647_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = OV5647_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = OV5647_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = OV5647_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = OV5647_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = OV5647_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = OV5647_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = OV5647_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = OV5647_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = OV5647_IsiGetIspStatusIss;
        pIsiSensor->pIsiSetBlcIss                       = OV5647_IsiSetBlcIss;
        pIsiSensor->pIsiGetBlcIss                       = OV5647_IsiGetBlcIss;
        pIsiSensor->pIsiSetWBIss                        = OV5647_IsiSetWBIss;
        pIsiSensor->pIsiGetWBIss                        = OV5647_IsiGetWBIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = OV5647_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = OV5647_IsiGetTpgIss;
        pIsiSensor->pIsiGetExpandCurveIss               = OV5647_IsiGetExpandCurveIss;

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

    TRACE(OV5647_INFO, "%s (exit)\n", __func__);
    return (result);
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t OV5647_IsiCamDrvConfig = {
    .cameraDriverID      = 0x5647,
    .pIsiGetSensorIss    = OV5647_IsiGetSensorIss,
};
