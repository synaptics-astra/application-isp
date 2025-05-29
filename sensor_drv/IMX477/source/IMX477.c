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

#include "IMX477_priv.h"

CREATE_TRACER(IMX477_INFO , "IMX477: ", INFO,    1);
CREATE_TRACER(IMX477_WARN , "IMX477: ", WARNING, 1);
CREATE_TRACER(IMX477_ERROR, "IMX477: ", ERROR,   1);
CREATE_TRACER(IMX477_DEBUG,     "IMX477: ", INFO, 1);
CREATE_TRACER(IMX477_REG_INFO , "IMX477: ", INFO, 1);
CREATE_TRACER(IMX477_REG_DEBUG, "IMX477: ", INFO, 1);

#define IVTPXCK_period          4.76e-9f // 4.76ns (210Mhz)
#define FINE_INTEG_TIME         1936.0f

#define IMX477_UNITY_GAIN       1.0f
#define IMX477_AEC_MIN_GAIN     IMX477_UNITY_GAIN
#define IMX477_AEC_MAX_GAIN     22.0f
#define IMX477_AEC_GAIN_STEP    IMX477_UNITY_GAIN

/* Chip ID */
#define IMX477_CHIP_ID                  0x0477

#define IMX477_REG_CHIP_ID              0x0016
#define IMX477_REG_MODE_SELECT          0x0100
#define IMX477_REG_GRP_PARAM_HOLD       0x0104

/* Exposure control */
#define IMX477_REG_COARSE_INT           0x0202

/* Analog gain control */
#define IMX477_REG_ANALOG_GAIN          0x0204
#define IMX477_ANA_GAIN_MIN             0
#define IMX477_ANA_GAIN_MAX             978
#define IMX477_ANA_GAIN_BASE            1024.0f // base value for analog gain calculation

/* Long exposure */
#define IMX477_REG_FRAME_LENGTH         0x0340
#define IMX477_REG_LINE_LEN_PCK         0x0342

/* Trigger mode */
#define IMX477_REG_MC_MODE              0x3f0b
#define IMX477_REG_MS_SEL               0x3041
#define IMX477_REG_XVS_IO_CTRL          0x3040
#define IMX477_REG_EXTOUT_EN            0x4b81

/* Test pattern generation */
#define IMX477_REG_TP_MODE              0x0600

/* DPC */
#define IMX477_REG_MAP_COUP_CORR_EN     0x0b05
#define IMX477_REG_SING_DEF_CORR_EN     0x0b06

/* Mode configs */
static IsiSensorMode_t pimx477_mode_info[] = {
    {
        /* 4k res 3840x2160 RAW10 30fps mode */
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
        .mipiLane = ISI_MIPI_2LANES,
        .dphyFreq = 900000,
    },
#ifdef SUPPORT_FULL_RESOLUTION // Enable only for debugging
    {
        /* FullRes 4056x2288 RAW10 30fps mode */
        .index         = 1,
        .size      = {
            .boundsWidth  = 4056,
            .boundsHeight = 2288,
            .top           = 0,
            .left          = 0,
            .width         = 4056,
            .height        = 2288,
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
        .mipiLane = ISI_MIPI_2LANES,
        .dphyFreq = 900000,
    },
    {
        /* 12MPix 4056x3040 RAW12 10fps mode */
        .index         = 2,
        .size      = {
            .boundsWidth  = 4056,
            .boundsHeight = 3040,
            .top           = 0,
            .left          = 0,
            .width         = 4056,
            .height        = 3040,
        },
        .aeInfo    = {
            .intTimeDelayFrame = 2,
            .gainDelayFrame = 2,
        },
        .fps           = 10 * ISI_FPS_QUANTIZE,
        .hdrMode      = ISI_SENSOR_MODE_LINEAR,
        .bitWidth     = 12,
        .bayerPattern = ISI_BPAT_RGGB,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
        .mipiLane = ISI_MIPI_2LANES,
        .dphyFreq = 900000,
    },
#endif // SUPPORT_FULL_RESOLUTION
};

static RESULT IMX477_IsiReadRegIss(IsiSensorHandle_t handle,
        const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    int32_t ret = 0;

    TRACE(IMX477_INFO, "%s (enter)\n", __func__);

    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    if (pIMX477Ctx == NULL || pIMX477Ctx->isiCtx.halI2cHandle == NULL)
        return RET_NULL_POINTER;

    ret = HalReadI2CReg(pIMX477Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (ret != 0) {
        TRACE(IMX477_ERROR, "%s: hal read sensor register error!\n", __func__);
        return RET_FAILURE;
    }

    TRACE(IMX477_INFO, "%s: Read: Reg:0x%X, Val:0x%X\n", __func__, addr, *pValue);
    TRACE(IMX477_INFO, "%s (exit) result = %d\n", __func__, result);

    return result;
}

static RESULT IMX477_IsiWriteRegIss(IsiSensorHandle_t handle,
        const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    int ret = 0;

    TRACE(IMX477_INFO, "%s (enter)\n", __func__);

    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    if (pIMX477Ctx == NULL || pIMX477Ctx->isiCtx.halI2cHandle == NULL)
        return RET_NULL_POINTER;

    ret = HalWriteI2CReg(pIMX477Ctx->isiCtx.halI2cHandle, addr, value);
    if (ret != 0) {
        TRACE(IMX477_ERROR, "%s: hal write sensor register error!\n",__func__);
        return RET_FAILURE;
    }

    TRACE(IMX477_INFO, "%s: Write: Reg:0x%X, Val:0x%X\n", __func__, addr, value);
    TRACE(IMX477_INFO, "%s (exit) result = %d\n", __func__, result);

    return result;
}

/* Read 2 registers at a time */
static RESULT IMX477_IsiRead2RegsIss(IsiSensorHandle_t handle,
        uint16_t reg, uint32_t *regVal)
{
    RESULT result = RET_SUCCESS;
    uint16_t value = 0;

    result = IMX477_IsiReadRegIss(handle, reg, &value);
    if (result != RET_SUCCESS)
        return result;

    *regVal = (value & 0xFF) << 8;

    result = IMX477_IsiReadRegIss(handle, reg + 1, &value);
    if (result != RET_SUCCESS)
        return result;

    *regVal |= (value & 0xFF);

    return RET_SUCCESS;
}

/* Write 2 registers at a time */
static RESULT IMX477_IsiWrite2RegsIss(IsiSensorHandle_t handle,
        uint16_t reg, uint32_t regVal)
{
    RESULT result = RET_SUCCESS;

    result = IMX477_IsiWriteRegIss(handle, reg, (regVal >> 8) & 0xFF);
    if (result != RET_SUCCESS)
        return result;

    result = IMX477_IsiWriteRegIss(handle, reg + 1, regVal & 0xFF);
    if (result != RET_SUCCESS)
        return result;

    return RET_SUCCESS;
}

static RESULT IMX477_IsiGetModeIss(IsiSensorHandle_t handle,
        IsiSensorMode_t *pMode)
{
    TRACE(IMX477_INFO, "%s (enter)\n", __func__);

    const IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    if (pIMX477Ctx == NULL)
        return RET_WRONG_HANDLE;

    memcpy(pMode, &(pIMX477Ctx->sensorMode), sizeof(pIMX477Ctx->sensorMode));

    TRACE(IMX477_INFO, "%s (exit)\n", __func__);

    return RET_SUCCESS;
}

static  RESULT IMX477_IsiEnumModeIss(IsiSensorHandle_t handle,
        IsiSensorEnumMode_t *pEnumMode)
{
    const IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    if (pIMX477Ctx == NULL)
        return RET_NULL_POINTER;

    if (pEnumMode->index >= (sizeof(pimx477_mode_info)/sizeof(pimx477_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pimx477_mode_info)/sizeof(pimx477_mode_info[0])); i++) {
        if (pimx477_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pimx477_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(IMX477_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT IMX477_IsiGetCapsIss(IsiSensorHandle_t handle,
        IsiCaps_t *pCaps)
{
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX477_INFO, "%s (enter)\n", __func__);

    if (pIMX477Ctx == NULL)
        return RET_WRONG_HANDLE;

    if (pCaps == NULL)
        return RET_NULL_POINTER;

    pCaps->bitWidth          = pIMX477Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pIMX477Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pIMX477Ctx->sensorMode.size.width;
    pCaps->resolution.height = pIMX477Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = pIMX477Ctx->sensorMode.mipiLane;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;
    pCaps->dphyFreq          = pIMX477Ctx->sensorMode.dphyFreq;

    if (pCaps->bitWidth == 10)
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    else if (pCaps->bitWidth == 12)
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    else
        pCaps->mipiMode      = ISI_MIPI_OFF;

    TRACE(IMX477_INFO, "%s (exit)\n", __func__);

    return result;
}

static RESULT IMX477_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig,
        IsiSensorHandle_t *pHandle)
{
    RESULT result = RET_SUCCESS;

    TRACE(IMX477_INFO, "%s (enter)\n", __func__);

    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) malloc(sizeof(IMX477_Context_t));
    if (!pIMX477Ctx) {
        TRACE(IMX477_ERROR, "%s: Can't allocate imx477 context\n",__func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pIMX477Ctx, 0, sizeof(IMX477_Context_t));

    pIMX477Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pIMX477Ctx->groupHold          = BOOL_FALSE;
    pIMX477Ctx->oldGain            = 0;
    pIMX477Ctx->oldIntegrationTime = 0;
    pIMX477Ctx->configured         = BOOL_FALSE;
    pIMX477Ctx->streaming          = BOOL_FALSE;
    pIMX477Ctx->testPattern        = BOOL_FALSE;
    pIMX477Ctx->isAfpsRun          = BOOL_FALSE;
    pIMX477Ctx->sensorMode.index   = 0;

    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x1a;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pIMX477Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr,
            sccbConfig.addrByte, sccbConfig.dataByte);
    if (pIMX477Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(IMX477_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return RET_NULL_POINTER;
    }

    result = HalAddRef(pIMX477Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
    if (result != RET_SUCCESS) {
        free(pIMX477Ctx);
        return result;
    }

    *pHandle = (IsiSensorHandle_t) pIMX477Ctx;

    TRACE(IMX477_INFO, "%s (exit)\n", __func__);

    return result;
}

static RESULT IMX477_IsiGetTLineIss(IsiSensorHandle_t handle, float *Tline)
{
    RESULT result = RET_SUCCESS;
    uint32_t line_length_pck = 0;

    /* get the line_length_pck */
    result = IMX477_IsiRead2RegsIss(handle, IMX477_REG_LINE_LEN_PCK, &line_length_pck);
    if (result != RET_SUCCESS)
        return result;

    /* Calculate Tline */
    *Tline = ((float)line_length_pck * IVTPXCK_period) / 4.0f;
    TRACE(IMX477_INFO, "%s: Tline = %f\n", __func__, *Tline);

    return result;
}

static RESULT IMX477_AecSetModeParameters(IsiSensorHandle_t handle,
        IMX477_Context_t * pIMX477Ctx)
{
    RESULT result = RET_SUCCESS;
    uint32_t cur_integ_time = 0;
    uint32_t line_length_pck = 0;
    uint32_t cur_again = 0;
    float again = 0, dgain = 0;
    float Tline = 0.0f;

    TRACE(IMX477_INFO, "%s%s: (enter)\n", __func__,
            pIMX477Ctx->isAfpsRun ? "(AFPS)" : "");

    pIMX477Ctx->aecIntegrationTimeIncrement = pIMX477Ctx->oneLineExpTime;
    pIMX477Ctx->aecMinIntegrationTime = pIMX477Ctx->oneLineExpTime *
        pIMX477Ctx->minIntegrationLine;
    pIMX477Ctx->aecMaxIntegrationTime = pIMX477Ctx->oneLineExpTime *
        pIMX477Ctx->maxIntegrationLine;

    TRACE(IMX477_DEBUG, "%s%s: AecMaxIntegrationTime = %f \n", __func__,
            pIMX477Ctx->isAfpsRun ? "(AFPS)" : "", pIMX477Ctx->aecMaxIntegrationTime);

    pIMX477Ctx->aecGainIncrement = IMX477_AEC_GAIN_STEP;

    /* Get the current exposure time set by default */
    result = IMX477_IsiRead2RegsIss(handle, IMX477_REG_COARSE_INT, &cur_integ_time);
    if (result != RET_SUCCESS)
        return result;

    /* Get the line_length_pck */
    result = IMX477_IsiRead2RegsIss(handle, IMX477_REG_LINE_LEN_PCK, &line_length_pck);
    if (result != RET_SUCCESS)
        return result;

    /* Get the Tline */
    result =  IMX477_IsiGetTLineIss(handle, &Tline);
    if (result != RET_SUCCESS)
        return result;

    pIMX477Ctx->aecCurIntegrationTime = Tline *
        (cur_integ_time + (FINE_INTEG_TIME / line_length_pck));
    pIMX477Ctx->aecCurLongIntegrationTime = pIMX477Ctx->aecCurIntegrationTime;

    /* Get the current gain set by default */
    result = IMX477_IsiRead2RegsIss(handle, IMX477_REG_ANALOG_GAIN, &cur_again);
    if (result != RET_SUCCESS)
        return result;

    again = IMX477_ANA_GAIN_BASE / (IMX477_ANA_GAIN_BASE - cur_again);
    dgain = IMX477_UNITY_GAIN;
    pIMX477Ctx->aecCurGain = again * dgain;

    if(pIMX477Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR)
        pIMX477Ctx->aecCurLongGain = 0;
    else
        pIMX477Ctx->aecCurLongGain = pIMX477Ctx->aecCurGain;

    pIMX477Ctx->oldGain               = 0;
    pIMX477Ctx->oldIntegrationTime    = 0;

    TRACE(IMX477_INFO, "%s%s: (exit)\n", __func__,
            pIMX477Ctx->isAfpsRun ? "(AFPS)" : "");

    return result;
}

static RESULT IMX477_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX477_INFO, "%s (enter)\n", __func__);

    if (!pIMX477Ctx) {
        TRACE(IMX477_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return RET_WRONG_HANDLE;
    }

    if (pIMX477Ctx->streaming != BOOL_FALSE)
        return RET_WRONG_STATE;

    pIMX477Ctx->sensorMode.index   = mode;

    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i = 0; i < sizeof(pimx477_mode_info) / sizeof(IsiSensorMode_t); i++) {
        if (pimx477_mode_info[i].index == pIMX477Ctx->sensorMode.index) {
            SensorDefaultMode = &(pimx477_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        /* Update the common registers */
        for (int i = 0; i < sizeof(IMX477_common_regs) /
                sizeof(IMX477_common_regs[0]); i++) {
            IMX477_IsiWriteRegIss(handle, IMX477_common_regs[i][0],
                    IMX477_common_regs[i][1]);
        }
        /* Update the link frequency registers */
        for (int i = 0; i < sizeof(link_450Mhz_regs) /
                sizeof(link_450Mhz_regs[0]); i++) {
            IMX477_IsiWriteRegIss(handle, link_450Mhz_regs[i][0],
                    link_450Mhz_regs[i][1]);
        }
        /* Apply default values of current mode */
        switch(SensorDefaultMode->index) {
            case 0: /* 4k res 30fps mode */
                for (int i = 0; i < sizeof(IMX477_mipi2lane_4K_init) /
                        sizeof(IMX477_mipi2lane_4K_init[0]); i++) {
                    IMX477_IsiWriteRegIss(handle, IMX477_mipi2lane_4K_init[i][0],
                            IMX477_mipi2lane_4K_init[i][1]);
                }
                break;
#ifdef SUPPORT_FULL_RESOLUTION // Enable only for debugging
            case 1: /* FullRes 30fps mode */
                for (int i = 0; i < sizeof(IMX477_mipi2lane_full_res_10bit_init) /
                        sizeof(IMX477_mipi2lane_full_res_10bit_init[0]); i++) {
                    IMX477_IsiWriteRegIss(handle, IMX477_mipi2lane_full_res_10bit_init[i][0],
                            IMX477_mipi2lane_full_res_10bit_init[i][1]);
                }
                break;
            case 2: /* 12MPix 10fps mode */
                for (int i = 0; i < sizeof(IMX477_mipi2lane_full_res_12bit_init) /
                        sizeof(IMX477_mipi2lane_full_res_12bit_init[0]); i++) {
                    IMX477_IsiWriteRegIss(handle, IMX477_mipi2lane_full_res_12bit_init[i][0],
                            IMX477_mipi2lane_full_res_12bit_init[i][1]);
                }
                break;
#endif // SUPPORT_FULL_RESOLUTION
            default:
                TRACE(IMX477_INFO,  "%s:no support for sensor mode %d\n", __func__,
                        pIMX477Ctx->sensorMode.index);
                return RET_NOTSUPP;
                break;
        }
        /* Set on-sensor DPC. */
        IMX477_IsiWriteRegIss(handle, IMX477_REG_MAP_COUP_CORR_EN, 1);
        IMX477_IsiWriteRegIss(handle, IMX477_REG_SING_DEF_CORR_EN, 1);
        /* Set vsync trigger mode (tm): 0=standalone, 1=source, 2=sink */
        int tm = 0;
        IMX477_IsiWriteRegIss(handle, IMX477_REG_MC_MODE, (tm > 0) ? 1 : 0);
        IMX477_IsiWriteRegIss(handle, IMX477_REG_MS_SEL, (tm <= 1) ? 1 : 0);
        IMX477_IsiWriteRegIss(handle, IMX477_REG_XVS_IO_CTRL, (tm == 1) ? 1 : 0);
        IMX477_IsiWriteRegIss(handle, IMX477_REG_EXTOUT_EN, (tm == 1) ? 1 : 0);

        memcpy(&(pIMX477Ctx->sensorMode), SensorDefaultMode, sizeof(IsiSensorMode_t));
    } else {
        TRACE(IMX477_INFO, "%s: Invalid SensorDefaultMode\n", __func__);
        return RET_NULL_POINTER;
    }

    /* Get the frame length lines */
    uint32_t frame_length_lines = 0;
    result = IMX477_IsiRead2RegsIss(handle, IMX477_REG_FRAME_LENGTH, &frame_length_lines);
    if (result != RET_SUCCESS)
        return result;

    TRACE(IMX477_INFO, "%s: frame_length_lines = %d\n", __func__, frame_length_lines);

    /* Get the Tline */
    float Tline = 0.0f;
    result = IMX477_IsiGetTLineIss(handle, &Tline);
    if (result != RET_SUCCESS)
        return result;

    TRACE(IMX477_INFO, "%s: Tline = %f\n", __func__, Tline);

    switch(pIMX477Ctx->sensorMode.index) {
        case 0:
#ifdef SUPPORT_FULL_RESOLUTION // Enable only for debugging
        case 1:
        case 2:
#endif // SUPPORT_FULL_RESOLUTION
            pIMX477Ctx->oneLineExpTime      = Tline;
            pIMX477Ctx->frameLengthLines    = frame_length_lines;
            pIMX477Ctx->curFrameLengthLines = pIMX477Ctx->frameLengthLines;
            pIMX477Ctx->maxIntegrationLine  = pIMX477Ctx->frameLengthLines - 4;
            pIMX477Ctx->minIntegrationLine  = 1;
            pIMX477Ctx->aecMinGain          = IMX477_AEC_MIN_GAIN;
            pIMX477Ctx->aecMaxGain          = IMX477_AEC_MAX_GAIN;
            pIMX477Ctx->aGain.min           = IMX477_AEC_MIN_GAIN;
            pIMX477Ctx->aGain.max           = IMX477_AEC_MAX_GAIN;
            pIMX477Ctx->aGain.step          = IMX477_AEC_GAIN_STEP;
            pIMX477Ctx->dGain.min           = IMX477_UNITY_GAIN;
            pIMX477Ctx->dGain.max           = IMX477_UNITY_GAIN;
            pIMX477Ctx->dGain.step          = IMX477_UNITY_GAIN;
            break;
        default:
            return (RET_NOTAVAILABLE);
            break;
    }

    pIMX477Ctx->maxFps  = pIMX477Ctx->sensorMode.fps;
    pIMX477Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pIMX477Ctx->currFps = pIMX477Ctx->maxFps;

    result = IMX477_AecSetModeParameters(handle, pIMX477Ctx);
    if (result != RET_SUCCESS) {
        TRACE(IMX477_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return result;
    }

    pIMX477Ctx->configured = BOOL_TRUE;
    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return 0;
}

static RESULT IMX477_IsiCloseIss(IsiSensorHandle_t handle)
{
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX477_INFO, "%s (enter)\n", __func__);

    if (pIMX477Ctx == NULL)
        return RET_WRONG_HANDLE;

    (void)IMX477_IsiSetStreamingIss(pIMX477Ctx, BOOL_FALSE);

    TRACE(IMX477_INFO, "%s (exit)\n", __func__);

    return result;
}

static RESULT IMX477_IsiReleaseIss(IsiSensorHandle_t handle)
{
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX477_INFO, "%s (enter)\n", __func__);

    if (pIMX477Ctx == NULL)
        return RET_WRONG_HANDLE;

    HalI2cClose(pIMX477Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pIMX477Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pIMX477Ctx, 0, sizeof(IMX477_Context_t));
    free(pIMX477Ctx);

    TRACE(IMX477_INFO,  "%s (exit)\n", __func__);

    return result;
}

static RESULT IMX477_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;
    uint32_t correctId = IMX477_CHIP_ID;
    uint32_t sensorId = 0;

    TRACE(IMX477_INFO,  "%s (enter)\n", __func__);

    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    if (pIMX477Ctx == NULL || pIMX477Ctx->isiCtx.halI2cHandle == NULL)
        return RET_NULL_POINTER;

    result = IMX477_IsiGetRevisionIss(handle, &sensorId);
    if (result != RET_SUCCESS) {
        TRACE(IMX477_ERROR, "%s: Read Sensor ID Error! \n",__func__);
        return RET_FAILURE;
    }

    if (correctId != sensorId) {
        TRACE(IMX477_ERROR, "%s:ChipID =0x%x sensorId=%x error! \n", __func__,
                correctId, sensorId);
        return RET_FAILURE;
    }

    TRACE(IMX477_INFO,"%s ChipID = 0x%08x, sensorId = 0x%08x, success! \n", __func__,
            correctId, sensorId);

    TRACE(IMX477_INFO, "%s (exit)\n", __func__);

    return result;
}

static RESULT IMX477_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t * pValue)
{
    RESULT result = RET_SUCCESS;
    uint32_t sensorId;

    TRACE(IMX477_INFO, "%s (enter)\n", __func__);

    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    if (pIMX477Ctx == NULL || pIMX477Ctx->isiCtx.halI2cHandle == NULL)
        return RET_NULL_POINTER;

    if (!pValue)
        return RET_NULL_POINTER;

    result = IMX477_IsiRead2RegsIss(handle, IMX477_REG_CHIP_ID, &sensorId);
    if (result != RET_SUCCESS)
        return result;

    *pValue = sensorId;

    TRACE(IMX477_INFO,  "%s (exit)\n", __func__);

    return result;
}

static RESULT IMX477_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;

    TRACE(IMX477_INFO, "%s (enter)\n", __func__);

    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    if (pIMX477Ctx == NULL || pIMX477Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pIMX477Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    result = IMX477_IsiWriteRegIss(handle, IMX477_REG_MODE_SELECT, on);
    if (result != RET_SUCCESS)
        return result;

    pIMX477Ctx->streaming = on;

    TRACE(IMX477_INFO, "%s (exit)\n", __func__);

    return result;
}

static RESULT IMX477_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle,
        IsiAeBaseInfo_t *pAeBaseInfo)
{
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (pIMX477Ctx == NULL) {
        TRACE(IMX477_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return RET_WRONG_HANDLE;
    }

    if (pAeBaseInfo == NULL) {
        TRACE(IMX477_ERROR, "%s: NULL pointer received!!\n", __func__);
        return RET_NULL_POINTER;
    }

    /* linear and native2dol*/
    pAeBaseInfo->gain.min        = pIMX477Ctx->aecMinGain;
    pAeBaseInfo->gain.max        = pIMX477Ctx->aecMaxGain;
    pAeBaseInfo->intTime.min     = pIMX477Ctx->aecMinIntegrationTime;
    pAeBaseInfo->intTime.max     = pIMX477Ctx->aecMaxIntegrationTime;

    pAeBaseInfo->aGain           = pIMX477Ctx->aGain;
    pAeBaseInfo->dGain           = pIMX477Ctx->dGain;

    pAeBaseInfo->aecCurGain      = pIMX477Ctx->aecCurGain;
    pAeBaseInfo->aecCurIntTime   = pIMX477Ctx->aecCurIntegrationTime;
    pAeBaseInfo->aecGainStep     = pIMX477Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep  = pIMX477Ctx->aecIntegrationTimeIncrement;

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT IMX477_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *)handle;

    if (!pIMX477Ctx || !pSensorAGain) {
        TRACE(IMX477_ERROR, "%s: Invalid parameter\n", __func__);
        return RET_NULL_POINTER;
    }

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    /* Get the requested gain value */
    float newGain = pSensorAGain->gain[ISI_LINEAR_PARAS];

    /* Limit gain within valid range */
    if (newGain > pIMX477Ctx->aecMaxGain) newGain = pIMX477Ctx->aecMaxGain;
    if (newGain < pIMX477Ctx->aecMinGain) newGain = pIMX477Ctx->aecMinGain;

    /* Convert gain to register value (formula: ANA_GAIN_GLOBAL = 1024 - (1024 / Gain)) */
    uint32_t regValue = (uint32_t)(IMX477_ANA_GAIN_BASE - (IMX477_ANA_GAIN_BASE / newGain));

    if (regValue > IMX477_ANA_GAIN_MAX) regValue = IMX477_ANA_GAIN_MAX;
    if (regValue < IMX477_ANA_GAIN_MIN) regValue = IMX477_ANA_GAIN_MIN;

    /* Start group hold */
    result = IMX477_IsiWriteRegIss(handle, IMX477_REG_GRP_PARAM_HOLD, 0x01);
    if (result != RET_SUCCESS)
        return result;

    /* Write analog gain */
    result = IMX477_IsiWrite2RegsIss(handle, IMX477_REG_ANALOG_GAIN, regValue);
    if (result != RET_SUCCESS)
        return result;

    /* End group hold (apply changes) */
    result = IMX477_IsiWriteRegIss(handle, IMX477_REG_GRP_PARAM_HOLD, 0x00);
    if (result != RET_SUCCESS)
        return result;

    /* Get the current gain */
    result = IMX477_IsiRead2RegsIss(handle, IMX477_REG_ANALOG_GAIN, &regValue);
    if (result != RET_SUCCESS)
        return result;

    /* Store the applied gain values */
    pIMX477Ctx->curAgain = IMX477_ANA_GAIN_BASE / (IMX477_ANA_GAIN_BASE - regValue);
    pIMX477Ctx->curDgain = IMX477_UNITY_GAIN;
    pIMX477Ctx->aecCurGain = pIMX477Ctx->curAgain * pIMX477Ctx->curDgain;

    TRACE(IMX477_INFO, "%s: newGain = %f AGain = %f (Reg: 0x%X)\n", __func__,
            newGain, pIMX477Ctx->curAgain, regValue);

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT IMX477_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    if (pIMX477Ctx == NULL || pIMX477Ctx->isiCtx.halI2cHandle == NULL)
        return RET_NULL_POINTER;

    pIMX477Ctx->curDgain = IMX477_UNITY_GAIN;
    TRACE(IMX477_DEBUG, "Set NewGain %f\n", pSensorDGain->gain[ISI_LINEAR_PARAS]);

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT IMX477_IsiGetAGainIss(IsiSensorHandle_t handle,
        IsiSensorGain_t *pSensorAGain)
{
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (pIMX477Ctx == NULL) {
        TRACE(IMX477_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return RET_WRONG_HANDLE;
    }

    if (pSensorAGain == NULL)
        return RET_NULL_POINTER;

    if(pIMX477Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR ||
            pIMX477Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pSensorAGain->gain[ISI_LINEAR_PARAS] = pIMX477Ctx->curAgain;
    } else {
        TRACE(IMX477_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT IMX477_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (pIMX477Ctx == NULL) {
        TRACE(IMX477_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return RET_WRONG_HANDLE;
    }

    if (pSensorDGain == NULL)
        return RET_NULL_POINTER;

    if (pIMX477Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR ||
            pIMX477Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pSensorDGain->gain[ISI_LINEAR_PARAS] = pIMX477Ctx->curDgain;
    } else {
        TRACE(IMX477_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT IMX477_IsiSetIntTimeIss(IsiSensorHandle_t handle,
        const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (!pIMX477Ctx) {
        TRACE(IMX477_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return RET_WRONG_HANDLE;
    }

    if (pIMX477Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR ||
            pIMX477Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        result = IMX477_SetIntTime(handle, pSensorIntTime->intTime[ISI_LINEAR_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(IMX477_INFO, "%s: set sensor IntTime[ISI_LINEAR_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }
    }  else {
        TRACE(IMX477_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

static RESULT IMX477_SetIntTime(IsiSensorHandle_t handle, float newIntegrationTime)
{
    RESULT result = RET_SUCCESS;
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *)handle;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (!pIMX477Ctx) {
        TRACE(IMX477_ERROR, "%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return RET_WRONG_HANDLE;
    }

    /* Get the requested exposure time */
    float Tsh = newIntegrationTime;

    /* Get the line_length_pck */
    uint32_t line_length_pck = 0;
    result = IMX477_IsiRead2RegsIss(handle, IMX477_REG_LINE_LEN_PCK, &line_length_pck);
    if (result != RET_SUCCESS)
        return result;

    /* Get the Tline */
    float Tline = 0.0f;
    result = IMX477_IsiGetTLineIss(handle, &Tline);
    if (result != RET_SUCCESS)
        return result;

    /* Compute COARSE_INTEG_TIME */
    uint32_t coarse_integ_time = (uint32_t)((Tsh / Tline) - (FINE_INTEG_TIME / line_length_pck));

    /* Ensure COARSE_INTEG_TIME is within allowed range */
    uint16_t min_coarse = 8;  // As per the datasheet recommendation
    uint16_t max_coarse = pIMX477Ctx->maxIntegrationLine;

    if (coarse_integ_time > max_coarse) coarse_integ_time = max_coarse;
    if (coarse_integ_time < min_coarse) coarse_integ_time = min_coarse;

    /* Start group hold */
    result = IMX477_IsiWriteRegIss(handle, IMX477_REG_GRP_PARAM_HOLD, 0x01);
    if (result != RET_SUCCESS)
        return result;

    /* Write COARSE_INTEG_TIME */
    result = IMX477_IsiWrite2RegsIss(handle, IMX477_REG_COARSE_INT, coarse_integ_time);
    if (result != RET_SUCCESS)
        return result;

    /* End group hold (apply changes) */
    result = IMX477_IsiWriteRegIss(handle, IMX477_REG_GRP_PARAM_HOLD, 0x00);
    if (result != RET_SUCCESS)
        return result;

    /* Store the applied integration time */
    pIMX477Ctx->aecCurIntegrationTime = Tline *
        (coarse_integ_time + (FINE_INTEG_TIME / line_length_pck));

    TRACE(IMX477_INFO, "%s: newIntegrationTime = %f IntegrationTime = %f (Reg: 0x%X)\n",
            __func__, newIntegrationTime, pIMX477Ctx->aecCurIntegrationTime, coarse_integ_time);

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT IMX477_IsiGetIntTimeIss(IsiSensorHandle_t handle,
        IsiSensorIntTime_t *pSensorIntTime)
{
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (!pIMX477Ctx) {
        TRACE(IMX477_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return RET_WRONG_HANDLE;
    }

    if (!pSensorIntTime) return RET_NULL_POINTER;

    if (pIMX477Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_LINEAR
            || pIMX477Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        pSensorIntTime->intTime[ISI_TRI_EXP_S_PARAS] = pIMX477Ctx->aecCurIntegrationTime;
    } else {
        TRACE(IMX477_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT IMX477_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t * pFps)
{
    const IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (pIMX477Ctx == NULL) {
        TRACE(IMX477_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return RET_WRONG_HANDLE;
    }

    *pFps = pIMX477Ctx->currFps;

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT IMX477_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (pIMX477Ctx == NULL) {
        TRACE(IMX477_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return RET_WRONG_HANDLE;
    }

    if (fps > pIMX477Ctx->maxFps) {
        TRACE(IMX477_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",
                __func__, fps, pIMX477Ctx->maxFps, pIMX477Ctx->minFps,pIMX477Ctx->maxFps);
        fps = pIMX477Ctx->maxFps;
    }
    if (fps < pIMX477Ctx->minFps) {
        TRACE(IMX477_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",
                __func__, fps, pIMX477Ctx->minFps, pIMX477Ctx->minFps,pIMX477Ctx->maxFps);
        fps = pIMX477Ctx->minFps;
    }

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT IMX477_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (pIMX477Ctx == NULL)
        return RET_WRONG_HANDLE;

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorAWB = false;
    pIspStatus->useSensorBLC = false;

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return RET_SUCCESS;
}

static RESULT IMX477_IsiSetBlcIss(IsiSensorHandle_t handle, const IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (pIMX477Ctx == NULL || pIMX477Ctx->isiCtx.halI2cHandle == NULL)
        return RET_WRONG_HANDLE;

    if (pBlc == NULL)
        return RET_NULL_POINTER;

    float rGain=0, grGain=0, gbGain=0, bGain=0;
    rGain  = pIMX477Ctx->sensorWb.rGain;
    grGain = pIMX477Ctx->sensorWb.grGain;
    gbGain = pIMX477Ctx->sensorWb.gbGain;
    bGain  = pIMX477Ctx->sensorWb.bGain;

    if (rGain  < 1)  rGain  = 1;
    if (grGain < 1)  grGain = 1;
    if (gbGain < 1)  gbGain = 1;
    if (bGain  < 1)  bGain  = 1;

    memcpy(&pIMX477Ctx->sensorBlc, pBlc, sizeof(IsiSensorBlc_t));

    if (result != RET_SUCCESS) {
        TRACE(IMX477_ERROR, "%s: set sensor blc error\n", __func__);
        return RET_FAILURE;
    }

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

static RESULT IMX477_IsiGetBlcIss(IsiSensorHandle_t handle, IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    const IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (pIMX477Ctx == NULL)
        return RET_WRONG_HANDLE;

    if (pBlc == NULL)
        return RET_NULL_POINTER;

    memcpy(pBlc, &pIMX477Ctx->sensorBlc, sizeof(IsiSensorBlc_t));

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

static RESULT IMX477_IsiSetWBIss(IsiSensorHandle_t handle, const IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (pIMX477Ctx == NULL || pIMX477Ctx->isiCtx.halI2cHandle == NULL)
        return RET_WRONG_HANDLE;

    if (pWb == NULL)
        return RET_NULL_POINTER;

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

static RESULT IMX477_IsiGetWBIss(IsiSensorHandle_t handle, IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    const IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (pIMX477Ctx == NULL )
        return RET_WRONG_HANDLE;

    if (pWb == NULL)
        return RET_NULL_POINTER;

    memcpy(pWb, &pIMX477Ctx->sensorWb, sizeof(IsiSensorWb_t));

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT IMX477_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (pIMX477Ctx == NULL || pIMX477Ctx->isiCtx.halI2cHandle == NULL)
        return RET_NULL_POINTER;

    if (pIMX477Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    if (tpg.enable == 0)
        result = IMX477_IsiWrite2RegsIss(handle, IMX477_REG_TP_MODE, 0x00);
    else
        result = IMX477_IsiWrite2RegsIss(handle, IMX477_REG_TP_MODE, 0x01); // 1h: Solid color

    pIMX477Ctx->testPattern = tpg.enable;

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT IMX477_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;
    uint32_t value = 0;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (pIMX477Ctx == NULL || pIMX477Ctx->isiCtx.halI2cHandle == NULL
            || pTpg == NULL)
        return RET_NULL_POINTER;

    if (pIMX477Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    if (!IMX477_IsiRead2RegsIss(handle, IMX477_REG_TP_MODE, &value)) {
        if (value != 0)
            pTpg->enable = 1;
        if (pTpg->enable)
            pTpg->pattern = (0x01ff & value);
        pIMX477Ctx->testPattern = pTpg->enable;
    }

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

static RESULT IMX477_IsiGetExpandCurveIss(IsiSensorHandle_t handle,
        IsiSensorCompandCurve_t *pCurve)
{
    RESULT result = RET_SUCCESS;
    IMX477_Context_t *pIMX477Ctx = (IMX477_Context_t *) handle;

    TRACE(IMX477_INFO, "%s: (enter)\n", __func__);

    if (pIMX477Ctx == NULL || pIMX477Ctx->isiCtx.halI2cHandle == NULL)
        return RET_NULL_POINTER;

    // TODO: Refer to the below logic and implement compand module for imx477 if needed
    uint8_t expandPx[64] = {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6};
    memcpy(pCurve->compandPx, expandPx, sizeof(expandPx));

    pCurve->compandXData[0] = 0;
    pCurve->compandYData[0] = 0;
    for(int i = 1; i < 65; i++) {
        pCurve->compandXData[i] = (1 << pCurve->compandPx[i-1]) + pCurve->compandXData[i-1];

        if(pCurve->compandXData[i] < 512)
            pCurve->compandYData[i] = pCurve->compandXData[i] << 1;//*2
        else if (pCurve->compandXData[i] < 768)
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 256) << 2;//*4
        else if (pCurve->compandXData[i] < 2560)
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 512) << 3;//*8
        else
            pCurve->compandYData[i] = (pCurve->compandXData[i] - 2048) << 5;//*32
    }

    TRACE(IMX477_INFO, "%s: (exit)\n", __func__);

    return result;
}

RESULT IMX477_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "IMX477";

    TRACE(IMX477_INFO, "%s (enter)\n", __func__);

    if (pIsiSensor != NULL) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = IMX477_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = IMX477_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = IMX477_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = IMX477_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = IMX477_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = IMX477_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = IMX477_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = IMX477_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = IMX477_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = IMX477_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = IMX477_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = IMX477_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = IMX477_IsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = IMX477_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = IMX477_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = IMX477_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = IMX477_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = IMX477_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = IMX477_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = IMX477_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = IMX477_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = IMX477_IsiGetIspStatusIss;
        pIsiSensor->pIsiSetBlcIss                       = IMX477_IsiSetBlcIss;
        pIsiSensor->pIsiGetBlcIss                       = IMX477_IsiGetBlcIss;
        pIsiSensor->pIsiSetWBIss                        = IMX477_IsiSetWBIss;
        pIsiSensor->pIsiGetWBIss                        = IMX477_IsiGetWBIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = IMX477_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = IMX477_IsiGetTpgIss;
        pIsiSensor->pIsiGetExpandCurveIss               = IMX477_IsiGetExpandCurveIss;

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

    TRACE(IMX477_INFO, "%s (exit)\n", __func__);

    return result;
}

/*****************************************************************************
 * each sensor driver need to declare this struct for isi load
 *****************************************************************************/
IsiCamDrvConfig_t IMX477_IsiCamDrvConfig = {
    .cameraDriverID      = 0x477,
    .pIsiGetSensorIss    = IMX477_IsiGetSensorIss,
};
