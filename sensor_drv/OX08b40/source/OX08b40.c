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
#include "OX08b40_priv.h"

CREATE_TRACER( Ox08b40_INFO , "Ox08b40: ", INFO,    1 );
CREATE_TRACER( Ox08b40_WARN , "Ox08b40: ", WARNING, 1 );
CREATE_TRACER( Ox08b40_ERROR, "Ox08b40: ", ERROR,   1 );
CREATE_TRACER( Ox08b40_DEBUG,     "Ox08b40: ", INFO, 0 );
CREATE_TRACER( Ox08b40_REG_INFO , "Ox08b40: ", INFO, 1 );
CREATE_TRACER( Ox08b40_REG_DEBUG, "Ox08b40: ", INFO, 1 );

#define Ox08b40_MIN_GAIN_STEP    ( 1.0f/1024.0f )  /**< min gain step size used by GUI (hardware min = 1/16; 1/16..32/16 depending on actual gain ) */

/*****************************************************************************
 *Sensor Info
*****************************************************************************/

static IsiSensorMode_t pox08b40_mode_info[] = {
    {
        .index     = 0,
        .size      ={
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
        .fps       = 5 * ISI_FPS_QUANTIZE,
        .hdrMode  = ISI_SENSOR_MODE_HDR_NATIVE,
        .nativeMode = ISI_SENSOR_NATIVE_DCG_SPD_VS,
        .bitWidth = 12,
        .compress.enable = 1,
        .compress.xBit  = 24,
        .compress.yBit  = 12,
        .bayerPattern = ISI_BPAT_BGGR,
        .afMode = ISI_SENSOR_AF_MODE_NOTSUPP,
    },
};

static RESULT Ox08b40_IsiReadRegIss(IsiSensorHandle_t handle, const uint16_t addr, uint16_t *pValue)
{
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s (enter)\n", __func__);

    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL || pOx08b40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalReadI2CReg(pOx08b40Ctx->isiCtx.halI2cHandle, addr, pValue);
    if (result != RET_SUCCESS) {
        TRACE(Ox08b40_ERROR, "%s: hal read sensor register error!\n", __func__);
        return (RET_FAILURE);
    }

    TRACE(Ox08b40_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT Ox08b40_IsiWriteRegIss(IsiSensorHandle_t handle, const uint16_t addr, const uint16_t value)
{
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s (enter)\n", __func__);

    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL || pOx08b40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = HalWriteI2CReg(pOx08b40Ctx->isiCtx.halI2cHandle, addr, value);
    if (result != RET_SUCCESS) {
        TRACE(Ox08b40_ERROR, "%s: hal write sensor register error!\n",__func__);
        return (RET_FAILURE);
    }

    TRACE(Ox08b40_INFO, "%s (exit) result = %d\n", __func__, result);
    return (result);
}

static RESULT Ox08b40_IsiGetModeIss(IsiSensorHandle_t handle, IsiSensorMode_t *pMode)
{
    TRACE(Ox08b40_INFO, "%s (enter)\n", __func__);
    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL) {
        return (RET_WRONG_HANDLE);
    }
    if (pMode == NULL) {
        return (RET_WRONG_HANDLE);
    }

    memcpy(pMode, &(pOx08b40Ctx->sensorMode), sizeof(pOx08b40Ctx->sensorMode));

    TRACE(Ox08b40_INFO, "%s (exit)\n", __func__);
    return ( RET_SUCCESS );
}

static  RESULT Ox08b40_IsiEnumModeIss(IsiSensorHandle_t handle, IsiSensorEnumMode_t *pEnumMode)
{
    TRACE(Ox08b40_INFO, "%s (enter)\n", __func__);
    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    if (pEnumMode->index >= (sizeof(pox08b40_mode_info)/sizeof(pox08b40_mode_info[0])))
        return RET_OUTOFRANGE;

    for (uint32_t i = 0; i < (sizeof(pox08b40_mode_info)/sizeof(pox08b40_mode_info[0])); i++) {
        if (pox08b40_mode_info[i].index == pEnumMode->index) {
            memcpy(&pEnumMode->mode, &pox08b40_mode_info[i],sizeof(IsiSensorMode_t));
            TRACE(Ox08b40_INFO, "%s (exit)\n", __func__);
            return RET_SUCCESS;
        }
    }

    return RET_NOTSUPP;
}

static RESULT Ox08b40_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps)
{
    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;

    RESULT result = RET_SUCCESS;

    TRACE(Ox08b40_INFO, "%s (enter)\n", __func__);

    if (pOx08b40Ctx == NULL) return (RET_WRONG_HANDLE);

    if (pCaps == NULL) {
        return (RET_NULL_POINTER);
    }

    pCaps->bitWidth          = pOx08b40Ctx->sensorMode.bitWidth;
    pCaps->mode              = ISI_MODE_BAYER;
    pCaps->bayerPattern      = pOx08b40Ctx->sensorMode.bayerPattern;
    pCaps->resolution.width  = pOx08b40Ctx->sensorMode.size.width;
    pCaps->resolution.height = pOx08b40Ctx->sensorMode.size.height;
    pCaps->mipiLanes         = ISI_MIPI_4LANES;
    pCaps->vinType           = ISI_ITF_TYPE_MIPI;

    if (pCaps->bitWidth == 10) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_10;
    } else if (pCaps->bitWidth == 12) {
        pCaps->mipiMode      = ISI_FORMAT_RAW_12;
    } else {
        pCaps->mipiMode      = ISI_MIPI_OFF;
    }

    TRACE(Ox08b40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT Ox08b40_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle)
{
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s (enter)\n", __func__);

    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) osMalloc(sizeof(Ox08b40_Context_t));
    if (!pOx08b40Ctx) {
        TRACE(Ox08b40_ERROR, "%s: Can't allocate ox08b40 context\n", __func__);
        return (RET_OUTOFMEM);
    }

    MEMSET(pOx08b40Ctx, 0, sizeof(Ox08b40_Context_t));

    pOx08b40Ctx->isiCtx.pSensor     = pConfig->pSensor;
    pOx08b40Ctx->groupHold          = BOOL_FALSE;
    pOx08b40Ctx->oldGain            = 0;
    pOx08b40Ctx->oldIntegrationTime = 0;
    pOx08b40Ctx->configured         = BOOL_FALSE;
    pOx08b40Ctx->streaming          = BOOL_FALSE;
    pOx08b40Ctx->testPattern        = BOOL_FALSE;
    pOx08b40Ctx->isAfpsRun          = BOOL_FALSE;
    pOx08b40Ctx->sensorMode.index   = 0;

    pOx08b40Ctx->i2cId                = pConfig->sensorBus.i2c.i2cBusNum;

    uint8_t busId = pConfig->sensorBus.i2c.i2cBusNum;
    IsiSensorSccbCfg_t sccbConfig;
    sccbConfig.slaveAddr = 0x6c;
    sccbConfig.addrByte  = 2;
    sccbConfig.dataByte  = 1;

    pOx08b40Ctx->isiCtx.halI2cHandle = HalI2cOpen(busId, sccbConfig.slaveAddr, sccbConfig.addrByte, sccbConfig.dataByte);
    if (pOx08b40Ctx->isiCtx.halI2cHandle == NULL) {
        TRACE(Ox08b40_ERROR, "%s: hal I2c open dev error!\n", __func__);
        return (RET_NULL_POINTER);
    }

   result = HalAddRef(pOx08b40Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);
   if (result != RET_SUCCESS) {
       osFree(pOx08b40Ctx);
       return (result);
   }

    *pHandle = (IsiSensorHandle_t) pOx08b40Ctx;

    TRACE(Ox08b40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT Ox08b40_AecSetModeParameters(IsiSensorHandle_t handle, Ox08b40_Context_t *pOx08b40Ctx)
{
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s%s: (enter)\n", __func__, pOx08b40Ctx->isAfpsRun ? "(AFPS)" : "");
    uint32_t exp_line = 0, again = 0, dgain = 0;
    uint16_t value = 0;


    pOx08b40Ctx->aecMinIntegrationTime       = pOx08b40Ctx->oneLineDCGExpTime * pOx08b40Ctx->minDCGIntegrationLine;
    pOx08b40Ctx->aecMaxIntegrationTime       = pOx08b40Ctx->oneLineDCGExpTime * pOx08b40Ctx->maxDCGIntegrationLine;
    TRACE(Ox08b40_DEBUG, "%s: AecMaxIntegrationTime = %f \n", __func__, pOx08b40Ctx->aecMaxIntegrationTime);

    pOx08b40Ctx->aecGainIncrement = Ox08b40_MIN_GAIN_STEP;
    pOx08b40Ctx->aecIntegrationTimeIncrement = pOx08b40Ctx->oneLineDCGExpTime;
    pOx08b40Ctx->oldGain               = 0;
    pOx08b40Ctx->oldIntegrationTime    = 0;

    //reflects the state of the sensor registers, must equal default settings
    //get again
    Ox08b40_IsiReadRegIss(handle, 0x3508, &value);//DCG
    again = (value & 0x0f) << 4;
    Ox08b40_IsiReadRegIss(handle, 0x3509, &value);
    again = again | ((value & 0xf0) >> 4);

    //get dgain
    Ox08b40_IsiReadRegIss(handle, 0x350a, &value);//DCG
    dgain = (value & 0x0f) << 10;
    Ox08b40_IsiReadRegIss(handle, 0x350b, &value);
    dgain = dgain | ((value & 0xff) << 2);
    Ox08b40_IsiReadRegIss(handle, 0x350c, &value);
    dgain = dgain | ((value & 0xc0) >> 6);
    pOx08b40Ctx->curGain.gain[0] = ((float32_t)again/16.0) * ((float32_t)dgain/1024.0);

    //get exp_line
    Ox08b40_IsiReadRegIss(handle, 0x3501, &value);//DCG
    exp_line = (value & 0xff) << 8;
    Ox08b40_IsiReadRegIss(handle, 0x3502, &value);
    exp_line = exp_line | (value & 0xff);
    pOx08b40Ctx->curIntTime.intTime[0] = exp_line * pOx08b40Ctx->oneLineDCGExpTime;


    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);

    return (result);
}

static RESULT Ox08b40_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode)
{
    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(Ox08b40_INFO, "%s (enter)\n", __func__);

    if (!pOx08b40Ctx) {
        TRACE(Ox08b40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOx08b40Ctx->streaming != BOOL_FALSE) {
        return RET_WRONG_STATE;
    }

    pOx08b40Ctx->sensorMode.index   = mode;
    IsiSensorMode_t *SensorDefaultMode = NULL;
    for (int i=0; i < sizeof(pox08b40_mode_info) / sizeof(IsiSensorMode_t); i++) {
        if (pox08b40_mode_info[i].index == pOx08b40Ctx->sensorMode.index) {
            SensorDefaultMode = &(pox08b40_mode_info[i]);
            break;
        }
    }

    if (SensorDefaultMode != NULL) {
        switch(SensorDefaultMode->index) {
            case 0:
                for (int i = 0; i<sizeof(Ox08b40_mipi4lane_8M_native4dol_init) / sizeof(Ox08b40_mipi4lane_8M_native4dol_init[0]); i++) {
                    if(Ox08b40_mipi4lane_8M_native4dol_init[i][0] == OX08B40_TABLE_WAIT){
                        osSleep(Ox08b40_mipi4lane_8M_native4dol_init[i][1]);
                    } else if (Ox08b40_mipi4lane_8M_native4dol_init[i][0] == OX08B40_TABLE_END){
                        break;
                    } else {
                        if(pOx08b40Ctx->i2cId == 0) {
                        Ox08b40_IsiWriteRegIss(handle, Ox08b40_mipi4lane_8M_native4dol_init[i][0], Ox08b40_mipi4lane_8M_native4dol_init[i][1]);
                    }
                }
                break;
            default:
                TRACE(Ox08b40_INFO, "%s:not support sensor mode %d\n", __func__, pOx08b40Ctx->sensorMode.index);
                osFree(pOx08b40Ctx);
                return RET_NOTSUPP;
                break;
    }
        }
        memcpy(&(pOx08b40Ctx->sensorMode), SensorDefaultMode, sizeof(IsiSensorMode_t));
    } else {
        TRACE(Ox08b40_ERROR,"%s: Invalid SensorDefaultMode\n", __func__);
        return (RET_NULL_POINTER);
    }

    switch(pOx08b40Ctx->sensorMode.index) {
        case 0:
            pOx08b40Ctx->oneLineDCGExpTime    = 0.0000448;
            pOx08b40Ctx->oneLineSPDExpTime    = 0.0000216;
            pOx08b40Ctx->oneLineVSExpTime     = 0.0000214;
            pOx08b40Ctx->frameLengthLines     = 0x8e0;//vts 5fps
            pOx08b40Ctx->curFrameLengthLines  = pOx08b40Ctx->frameLengthLines;
            pOx08b40Ctx->maxDCGIntegrationLine= pOx08b40Ctx->frameLengthLines - 13 - 2;
            pOx08b40Ctx->minDCGIntegrationLine= 2;
            pOx08b40Ctx->maxSPDIntegrationLine= pOx08b40Ctx->frameLengthLines - 13;
            pOx08b40Ctx->minSPDIntegrationLine= 2;
            pOx08b40Ctx->maxVSIntegrationLine = 35;
            pOx08b40Ctx->minVSIntegrationLine = 0;
            pOx08b40Ctx->aecMaxGain           = 240;
            pOx08b40Ctx->aecMinGain           = 1.0;
            pOx08b40Ctx->aGain.min            = 1.0;
            pOx08b40Ctx->aGain.max            = 15.5;
            pOx08b40Ctx->aGain.step           = (1.0f/16.0f);
            pOx08b40Ctx->dGain.min            = 1.0;
            pOx08b40Ctx->dGain.max            = 15.99;
            pOx08b40Ctx->dGain.step           = (1.0f/1024.0f);
            break;
        default:
            TRACE(Ox08b40_INFO, "%s:not support sensor mode %d\n", __func__, pOx08b40Ctx->sensorMode.index);
            return RET_NOTSUPP;
            break;
    }

    pOx08b40Ctx->maxFps  = pOx08b40Ctx->sensorMode.fps;
    pOx08b40Ctx->minFps  = 1 * ISI_FPS_QUANTIZE;
    pOx08b40Ctx->currFps = pOx08b40Ctx->maxFps;

    TRACE(Ox08b40_DEBUG, "%s: Ox08b40 System-Reset executed\n", __func__);
    osSleep(100);

    result = Ox08b40_AecSetModeParameters(handle, pOx08b40Ctx);
    if (result != RET_SUCCESS) {
        TRACE(Ox08b40_ERROR, "%s: SetupOutputWindow failed.\n", __func__);
        return (result);
    }

    pOx08b40Ctx->configured = BOOL_TRUE;


    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return 0;
}

static RESULT Ox08b40_IsiCloseIss(IsiSensorHandle_t handle)
{
    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s (enter)\n", __func__);

    if (pOx08b40Ctx == NULL) return (RET_WRONG_HANDLE);

    (void)Ox08b40_IsiSetStreamingIss(pOx08b40Ctx, BOOL_FALSE);

    TRACE(Ox08b40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT Ox08b40_IsiReleaseIss(IsiSensorHandle_t handle)
{
    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s (enter)\n", __func__);

    if (pOx08b40Ctx == NULL) return (RET_WRONG_HANDLE);

    HalI2cClose(pOx08b40Ctx->isiCtx.halI2cHandle);
    (void)HalDelRef(pOx08b40Ctx->isiCtx.halI2cHandle, HAL_DEV_I2C);

    MEMSET(pOx08b40Ctx, 0, sizeof(Ox08b40_Context_t));
    osFree(pOx08b40Ctx);
    TRACE(Ox08b40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT Ox08b40_IsiCheckConnectionIss(IsiSensorHandle_t handle)
{
    RESULT result = RET_SUCCESS;

    uint32_t sensor_id = 0;
    uint32_t correct_id = 0x580841;

    TRACE(Ox08b40_INFO, "%s (enter)\n", __func__);

    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL || pOx08b40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    result = Ox08b40_IsiGetRevisionIss(handle, &sensor_id);
    if (result != RET_SUCCESS) {
        TRACE(Ox08b40_ERROR, "%s: Read Sensor ID Error! \n",__func__);
        return (RET_FAILURE);
    }

    if (correct_id != sensor_id) {
        TRACE(Ox08b40_ERROR, "%s:ChipID =0x%x sensor_id=%x error! \n", __func__, correct_id, sensor_id);
        return (RET_FAILURE);
    }

    TRACE(Ox08b40_INFO,"%s ChipID = 0x%08x, sensor_id = 0x%08x, success! \n", __func__, correct_id, sensor_id);
    TRACE(Ox08b40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT Ox08b40_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue)
{
    RESULT result = RET_SUCCESS;
    uint16_t reg_val;
    uint32_t sensor_id;

    TRACE(Ox08b40_INFO, "%s (enter)\n", __func__);

    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL || pOx08b40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    reg_val   = 0;
    result    = Ox08b40_IsiReadRegIss(handle, 0x300a, &reg_val);
    sensor_id = (reg_val & 0xff) << 16;

    reg_val   = 0;
    result    |= Ox08b40_IsiReadRegIss(handle, 0x300b, &reg_val);
    sensor_id |= ((reg_val & 0xff) << 8);

    reg_val   = 0;
    result    |= Ox08b40_IsiReadRegIss(handle, 0x300c, &reg_val);
    sensor_id |= (reg_val & 0xff);

    *pValue = sensor_id;
    TRACE(Ox08b40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT Ox08b40_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on)
{
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s (enter)\n", __func__);

    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL || pOx08b40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOx08b40Ctx->configured != BOOL_TRUE)
        return RET_WRONG_STATE;

    result = Ox08b40_IsiWriteRegIss(handle, 0x0100, on);
    if (result != RET_SUCCESS) {
        TRACE(Ox08b40_ERROR, "%s: set sensor streaming error! \n",__func__);
        return (RET_FAILURE);
    }

    pOx08b40Ctx->streaming = on;

    TRACE(Ox08b40_INFO, "%s (exit)\n", __func__);
    return (result);
}

static RESULT Ox08b40_pIsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo)
{
    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    RESULT result = RET_SUCCESS;

    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);

    if (pOx08b40Ctx == NULL) {
        TRACE(Ox08b40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pAeBaseInfo == NULL) {
        TRACE(Ox08b40_ERROR, "%s: NULL pointer received!!\n");
        return (RET_NULL_POINTER);
    }

    //get time limit and total gain limit
    pAeBaseInfo->longGain.min        = pOx08b40Ctx->aecMinGain;
    pAeBaseInfo->longGain.max        = pOx08b40Ctx->aecMaxGain;
    pAeBaseInfo->longIntTime.min     = pOx08b40Ctx->aecMinIntegrationTime;
    pAeBaseInfo->longIntTime.max     = pOx08b40Ctx->aecMaxIntegrationTime;

    //get again/dgain info
    pAeBaseInfo->aLongGain           = pOx08b40Ctx->aGain;
    pAeBaseInfo->dLongGain           = pOx08b40Ctx->dGain;

    //get current intTime and gain
    pAeBaseInfo->curIntTime  = pOx08b40Ctx->curIntTime;
    pAeBaseInfo->curGain     = pOx08b40Ctx->curGain;

    pAeBaseInfo->aecGainStep = pOx08b40Ctx->aecGainIncrement;
    pAeBaseInfo->aecIntTimeStep = pOx08b40Ctx->aecIntegrationTimeIncrement;
    pAeBaseInfo->nativeMode  = pOx08b40Ctx->sensorMode.nativeMode;
    pAeBaseInfo->nativeHdrRatio[0] = 16.72;
    pAeBaseInfo->nativeHdrRatio[1] = 14.49;
    pAeBaseInfo->nativeHdrRatio[2] = 9.2;
    pAeBaseInfo->conversionGainDCG = 4.18;

    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);
    return (result);
}

RESULT Ox08b40_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);
    uint32_t again = 0;

    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL || pOx08b40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if(pSensorAGain->gain[ISI_LINEAR_PARAS] < pOx08b40Ctx->aGain.min){
        TRACE(Ox08b40_WARN, "%s: invalid too small again parameter!\n", __func__);
        pSensorAGain->gain[ISI_LINEAR_PARAS] = pOx08b40Ctx->aGain.min;
    }

    if(pSensorAGain->gain[ISI_LINEAR_PARAS] > pOx08b40Ctx->aGain.max){
        TRACE(Ox08b40_WARN, "%s: invalid too big again parameter!\n", __func__);
        pSensorAGain->gain[ISI_LINEAR_PARAS] = pOx08b40Ctx->aGain.max;
    }

    //set HCG analog gain, base exp, conversion gain = HCG/LCG = 4.18
    again = (uint32_t)(pSensorAGain->gain[ISI_LINEAR_PARAS] * 16);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x3508, (again & 0xf0) >> 4);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x3509, (again & 0x0f) << 4);
    pOx08b40Ctx->curAgain.gain[0] = (float32_t)again/16.0f;

    //set LCG analog gain
    again = (uint32_t)(pSensorAGain->gain[ISI_LINEAR_PARAS] / 4 * 16);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x3588, (again & 0xf0) >> 4);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x3589, (again & 0x0f) << 4);
    pOx08b40Ctx->curAgain.gain[1] = (float32_t)again/16.0f;

    //set S analog gain, base exp
    again = (uint32_t)(pSensorAGain->gain[ISI_LINEAR_PARAS] / 2 * 16);
    result = Ox08b40_IsiWriteRegIss(handle, 0x3548, (again & 0xf0) >> 4);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x3549, (again & 0x0f) << 4);
    pOx08b40Ctx->curAgain.gain[2] = (float32_t)again/16.0f;

    //set VS analog gain
    again = (uint32_t)(pSensorAGain->gain[ISI_LINEAR_PARAS] / 8 * 16);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x35c8, (again & 0xf0) >> 4);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x35c9, (again & 0x0f) << 4);
    pOx08b40Ctx->curAgain.gain[3] = (float32_t)again/16.0f;

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT Ox08b40_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);
    uint32_t dgain = 0;

    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL || pOx08b40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if(pSensorDGain->gain[ISI_LINEAR_PARAS] < pOx08b40Ctx->dGain.min){
        TRACE(Ox08b40_WARN, "%s: invalid too small dgain parameter!\n", __func__);
        pSensorDGain->gain[ISI_LINEAR_PARAS] = pOx08b40Ctx->dGain.min;
    }

    if(pSensorDGain->gain[ISI_LINEAR_PARAS] > pOx08b40Ctx->dGain.max){
        TRACE(Ox08b40_WARN, "%s: invalid too big dgain parameter!\n", __func__);
        pSensorDGain->gain[ISI_LINEAR_PARAS] = pOx08b40Ctx->dGain.max;
    }

    //set HCG digital gain, base exp
    dgain = (uint32_t)(pSensorDGain->gain[ISI_LINEAR_PARAS] * 1024);
    result = Ox08b40_IsiWriteRegIss(handle, 0x350a, (dgain >> 10) & 0x0f);
    result |= Ox08b40_IsiWriteRegIss(handle,0x350b, (dgain >> 2) & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle,0x350c, (dgain & 0x3) << 6);
    pOx08b40Ctx->curDgain.gain[0] = (float32_t)dgain/1024.0f;
    pOx08b40Ctx->curGain.gain[0] = pOx08b40Ctx->curAgain.gain[0] * pOx08b40Ctx->curDgain.gain[0];

    //set LCG digital gain
    dgain = (uint32_t)(pSensorDGain->gain[ISI_LINEAR_PARAS] * 1024);
    result = Ox08b40_IsiWriteRegIss(handle, 0x358a, (dgain >> 10) & 0x0f);
    result |= Ox08b40_IsiWriteRegIss(handle,0x358b, (dgain >> 2) & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle,0x358c, (dgain & 0x3) << 6);
    pOx08b40Ctx->curDgain.gain[1] = (float32_t)dgain/1024.0f;
    pOx08b40Ctx->curGain.gain[1] = pOx08b40Ctx->curAgain.gain[1] * pOx08b40Ctx->curDgain.gain[1];

    //set S digital gain
    dgain = (uint32_t)(pSensorDGain->gain[ISI_LINEAR_PARAS] * 1024);
    result = Ox08b40_IsiWriteRegIss(handle, 0x354a, (dgain >> 10) & 0x0f);
    result |= Ox08b40_IsiWriteRegIss(handle,0x354b, (dgain >> 2) & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle,0x354c, (dgain & 0x3) << 6);
    pOx08b40Ctx->curDgain.gain[2] = (float32_t)dgain/1024.0f;
    pOx08b40Ctx->curGain.gain[2] = pOx08b40Ctx->curAgain.gain[2] * pOx08b40Ctx->curDgain.gain[2];

    //set VS digital gain
    dgain = (uint32_t)(pSensorDGain->gain[ISI_LINEAR_PARAS] * 1024);
    result = Ox08b40_IsiWriteRegIss(handle, 0x35ca, (dgain >> 10) & 0x0f);
    result |= Ox08b40_IsiWriteRegIss(handle,0x35cb, (dgain >> 2) & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle,0x35cc, (dgain & 0x3) << 6);
    pOx08b40Ctx->curDgain.gain[3] = (float32_t)dgain/1024.0f;
    pOx08b40Ctx->curGain.gain[3] = pOx08b40Ctx->curAgain.gain[3] * pOx08b40Ctx->curDgain.gain[3];

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT Ox08b40_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain)
{
    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);

    if (pOx08b40Ctx == NULL) {
        TRACE(Ox08b40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorAGain == NULL) {
        return (RET_NULL_POINTER);
    }

    *pSensorAGain       = pOx08b40Ctx->curAgain;

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT Ox08b40_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain)
{
    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);

    if (pOx08b40Ctx == NULL) {
        TRACE(Ox08b40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (pSensorDGain == NULL) {
        return (RET_NULL_POINTER);
    }

    *pSensorDGain = pOx08b40Ctx->curDgain;

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT Ox08b40_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);
    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;

    if (!pOx08b40Ctx) {
        TRACE(Ox08b40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (pOx08b40Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        result = Ox08b40_SetIntTime(handle, pSensorIntTime->intTime[ISI_LINEAR_PARAS]);
        if (result != RET_SUCCESS) {
            TRACE(Ox08b40_INFO, "%s: set sensor IntTime[ISI_LINEAR_PARAS] error!\n", __func__);
            return RET_FAILURE;
        }

    } else {
        TRACE(Ox08b40_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT Ox08b40_SetIntTime(IsiSensorHandle_t handle, float32_t newIntegrationTime)
{
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);
    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    uint32_t exp_line = 0;
    float32_t sIntegrationTime = 0, vsIntegrationTime = 0;

    if (!pOx08b40Ctx) {
        TRACE(Ox08b40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    //set DCG(HCG & LCG) exp
    exp_line = newIntegrationTime / pOx08b40Ctx->oneLineDCGExpTime;
    exp_line = MIN(pOx08b40Ctx->maxDCGIntegrationLine, MAX(pOx08b40Ctx->minDCGIntegrationLine, exp_line));
    TRACE(Ox08b40_DEBUG, "%s: set DCG exp_line = 0x%04x\n", __func__, exp_line);
    result =  Ox08b40_IsiWriteRegIss(handle, 0x3501,(exp_line >> 8) & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x3502,(exp_line & 0xff));
    pOx08b40Ctx->curIntTime.intTime[0] = exp_line * pOx08b40Ctx->oneLineDCGExpTime;
    pOx08b40Ctx->curIntTime.intTime[1] = exp_line * pOx08b40Ctx->oneLineDCGExpTime;

    //set SPD exp
    sIntegrationTime = newIntegrationTime/4;
    exp_line = sIntegrationTime / pOx08b40Ctx->oneLineSPDExpTime;
    exp_line = MIN(pOx08b40Ctx->maxDCGIntegrationLine, MAX(pOx08b40Ctx->minDCGIntegrationLine, exp_line));
    TRACE(Ox08b40_DEBUG, "%s: set S exp_line = 0x%04x\n", __func__, exp_line);
    result = Ox08b40_IsiWriteRegIss(handle, 0x3541,(exp_line >> 8) & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x3542,(exp_line & 0xff));
    pOx08b40Ctx->curIntTime.intTime[2] = exp_line * pOx08b40Ctx->oneLineSPDExpTime;

    //set VS exp
    vsIntegrationTime = newIntegrationTime/4/16;
    exp_line = vsIntegrationTime / pOx08b40Ctx->oneLineVSExpTime;
    exp_line = MIN(pOx08b40Ctx->maxVSIntegrationLine,MAX(pOx08b40Ctx->minVSIntegrationLine, exp_line));
    result |= Ox08b40_IsiWriteRegIss(handle, 0x35c1,(exp_line >> 8) & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x35c2,(exp_line & 0xff));
    pOx08b40Ctx->curIntTime.intTime[3] = exp_line * pOx08b40Ctx->oneLineVSExpTime;

    TRACE(Ox08b40_DEBUG, "%s: set IntTime = %f\n", __func__, pOx08b40Ctx->curIntTime.intTime[0]);
    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return (result);
}
RESULT Ox08b40_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime)
{
    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);

    if (!pOx08b40Ctx) {
        TRACE(Ox08b40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n",__func__);
        return (RET_WRONG_HANDLE);
    }

    if (!pSensorIntTime) return (RET_NULL_POINTER);


    if (pOx08b40Ctx->sensorMode.hdrMode == ISI_SENSOR_MODE_HDR_NATIVE) {
        *pSensorIntTime = pOx08b40Ctx->curIntTime;

    } else {
        TRACE(Ox08b40_INFO, "%s:not support this ExpoFrmType.\n", __func__);
        return RET_NOTSUPP;
    }

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT Ox08b40_IsiGetFpsIss(IsiSensorHandle_t handle, uint32_t *pFps)
{
    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);

    if (pOx08b40Ctx == NULL) {
        TRACE(Ox08b40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    *pFps = pOx08b40Ctx->currFps;

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT Ox08b40_IsiSetFpsIss(IsiSensorHandle_t handle, uint32_t fps)
{
    RESULT result = RET_SUCCESS;
    int32_t NewVts = 0;
    uint32_t vs_exp = 0;
    uint16_t value = 0;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);

    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL) {
        TRACE(Ox08b40_ERROR,"%s: Invalid sensor handle (NULL pointer detected)\n", __func__);
        return (RET_WRONG_HANDLE);
    }

    if (fps > pOx08b40Ctx->maxFps) {
        TRACE(Ox08b40_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n", __func__, fps, pOx08b40Ctx->maxFps, pOx08b40Ctx->minFps,pOx08b40Ctx->maxFps);
        fps = pOx08b40Ctx->maxFps;
    }
    if (fps < pOx08b40Ctx->minFps) {
        TRACE(Ox08b40_ERROR,"%s: set fps(%d) out of range, correct to %d (%d, %d)\n",__func__, fps, pOx08b40Ctx->minFps, pOx08b40Ctx->minFps,pOx08b40Ctx->maxFps);
        fps = pOx08b40Ctx->minFps;
    }

    NewVts = pOx08b40Ctx->frameLengthLines*pOx08b40Ctx->sensorMode.fps / fps;
    result  =  Ox08b40_IsiWriteRegIss(handle, 0x380e, NewVts >> 8);
    result |=  Ox08b40_IsiWriteRegIss(handle, 0x380f, NewVts & 0xff);
    pOx08b40Ctx->currFps              = fps;
    pOx08b40Ctx->curFrameLengthLines  = NewVts;
    result  =  Ox08b40_IsiReadRegIss(handle, 0x35c1, &value);
    vs_exp = (value & 0xff) << 8;
    result =  Ox08b40_IsiReadRegIss(handle, 0x38c2, &value);
    vs_exp |= (value & 0xff);
    pOx08b40Ctx->maxDCGIntegrationLine = pOx08b40Ctx->curFrameLengthLines - 13 - vs_exp;
    pOx08b40Ctx->maxSPDIntegrationLine = pOx08b40Ctx->curFrameLengthLines - 13;
    pOx08b40Ctx->aecMaxIntegrationTime = pOx08b40Ctx->maxDCGIntegrationLine * pOx08b40Ctx->oneLineDCGExpTime;

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT Ox08b40_IsiGetIspStatusIss(IsiSensorHandle_t handle, IsiIspStatus_t *pIspStatus)
{
    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL || pOx08b40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);

    pIspStatus->useSensorAE  = false;
    pIspStatus->useSensorBLC = true;
    pIspStatus->useSensorAWB = true;

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return RET_SUCCESS;
}

RESULT Ox08b40_IsiSetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t tpg)
{
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);

    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL || pOx08b40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_NULL_POINTER;
    }

    if (pOx08b40Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (tpg.enable == 0) {
        result = Ox08b40_IsiWriteRegIss(handle, 0x5004, 0x1e);
        result = Ox08b40_IsiWriteRegIss(handle, 0x5005, 0x1e);
        result = Ox08b40_IsiWriteRegIss(handle, 0x5006, 0x1e);
        result = Ox08b40_IsiWriteRegIss(handle, 0x5007, 0x1e);

        result = Ox08b40_IsiWriteRegIss(handle, 0x5240, 0x00);
        result = Ox08b40_IsiWriteRegIss(handle, 0x5440, 0x00);
        result = Ox08b40_IsiWriteRegIss(handle, 0x5640, 0x00);
        result = Ox08b40_IsiWriteRegIss(handle, 0x5840, 0x00);
    } else {
        result = Ox08b40_IsiWriteRegIss(handle, 0x5004, 0x1f);
        result = Ox08b40_IsiWriteRegIss(handle, 0x5005, 0x1f);
        result = Ox08b40_IsiWriteRegIss(handle, 0x5006, 0x1f);
        result = Ox08b40_IsiWriteRegIss(handle, 0x5007, 0x1f);

        result = Ox08b40_IsiWriteRegIss(handle, 0x5240, 0x01);
        result = Ox08b40_IsiWriteRegIss(handle, 0x5440, 0x01);
        result = Ox08b40_IsiWriteRegIss(handle, 0x5640, 0x01);
        result = Ox08b40_IsiWriteRegIss(handle, 0x5840, 0x01);
    }

    pOx08b40Ctx->testPattern = tpg.enable;

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT Ox08b40_IsiGetTpgIss(IsiSensorHandle_t handle, IsiSensorTpg_t *pTpg)
{
    RESULT result = RET_SUCCESS;
    uint16_t hcgValue = 0, lcgValue = 0, spdValue = 0, vsValue = 0;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);

    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL || pOx08b40Ctx->isiCtx.halI2cHandle == NULL || pTpg== NULL) {
        return RET_NULL_POINTER;
    }

    if (pOx08b40Ctx->configured != BOOL_TRUE)  return RET_WRONG_STATE;

    if (!Ox08b40_IsiReadRegIss(handle, 0x5240, &hcgValue) & !Ox08b40_IsiReadRegIss(handle, 0x5440, &lcgValue)
        & !Ox08b40_IsiReadRegIss(handle, 0x5640, &spdValue) & !Ox08b40_IsiReadRegIss(handle, 0x5840, &vsValue)) {
        pTpg->enable = (((hcgValue & 0x01) != 0) && ((lcgValue & 0x01) != 0) && ((spdValue & 0x01) != 0) && ((vsValue & 0x01) != 0)) ? 1 : 0;
        if (pTpg->enable) {
            pTpg->pattern = (0xff & hcgValue);
        }
      pOx08b40Ctx->testPattern = pTpg->enable;
    }

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT Ox08b40_IsiSetWBIss(IsiSensorHandle_t handle, const IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);

    uint32_t b_gain, gb_gain, gr_gain, r_gain;
    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL || pOx08b40Ctx->isiCtx.halI2cHandle == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)   return RET_NULL_POINTER;

    b_gain = (uint32_t)(pWb->bGain  * 1024);
    gb_gain = (uint32_t)(pWb->gbGain * 1024);
    gr_gain = (uint32_t)(pWb->grGain * 1024);
    r_gain  = (uint32_t)(pWb->rGain  * 1024);

    //set HCG channel awb gain
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5280, (b_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5281, b_gain & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5282, (gb_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5283, gb_gain & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5284, (gr_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5285, gr_gain & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5286, (r_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5287, r_gain & 0xff);

    //set LCG channel awb gain
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5480, (b_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5481, b_gain & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5482, (gb_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5483, gb_gain & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5484, (gr_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5485, gr_gain & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5486, (r_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5487, r_gain & 0xff);

    //set S channel awb gain
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5680, (b_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5681, b_gain & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5682, (gb_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5683, gb_gain & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5684, (gr_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5685, gr_gain & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5686, (r_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5687, r_gain & 0xff);

    //set VS channel awb gain
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5880, (b_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5881, b_gain & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5882, (gb_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5883, gb_gain & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5884, (gr_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5885, gr_gain & 0xff);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5886, (r_gain >> 8) & 0x7f);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x5887, r_gain & 0xff);

    memcpy(&pOx08b40Ctx->sensorWb, pWb, sizeof(IsiSensorWb_t));

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return result;
}

static RESULT Ox08b40_IsiGetWBIss(IsiSensorHandle_t handle, IsiSensorWb_t *pWb)
{
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);

    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL ) {
        return RET_WRONG_HANDLE;
    }

    if (pWb == NULL)  return RET_NULL_POINTER;

    memcpy(pWb, &pOx08b40Ctx->sensorWb, sizeof(IsiSensorWb_t));

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

static RESULT Ox08b40_IsiSetBlcIss(IsiSensorHandle_t handle, const IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    uint16_t blcGain = 0;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);

    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)  return RET_NULL_POINTER;

    blcGain = pBlc->red;
    //set HCG blc
    result = Ox08b40_IsiWriteRegIss(handle, 0x4026, (blcGain >> 8) & 0x03);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x4027, blcGain & 0xff);
    //set LCG blc
    result |= Ox08b40_IsiWriteRegIss(handle, 0x4028, (blcGain >> 8) & 0x03);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x4029, blcGain & 0xff);
    //set S blc
    result |= Ox08b40_IsiWriteRegIss(handle, 0x402a, (blcGain >> 8) & 0x03);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x402b, blcGain & 0xff);
    //set VS blc
    result |= Ox08b40_IsiWriteRegIss(handle, 0x402c, (blcGain >> 8) & 0x03);
    result |= Ox08b40_IsiWriteRegIss(handle, 0x402d, blcGain & 0xff);

    pOx08b40Ctx->sensorBlc = *pBlc;

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return result;
}

static RESULT Ox08b40_IsiGetBlcIss(IsiSensorHandle_t handle, IsiSensorBlc_t *pBlc)
{
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);

    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL) {
        return RET_WRONG_HANDLE;
    }

    if (pBlc == NULL)  return RET_NULL_POINTER;

    *pBlc = pOx08b40Ctx->sensorBlc;

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return result;
}


static RESULT Ox08b40_IsiGetExpandCurveIss(IsiSensorHandle_t handle, IsiSensorCompandCurve_t *pCurve)
{
    RESULT result = RET_SUCCESS;
    TRACE(Ox08b40_INFO, "%s: (enter)\n", __func__);

    Ox08b40_Context_t *pOx08b40Ctx = (Ox08b40_Context_t *) handle;
    if (pOx08b40Ctx == NULL) {
        return RET_NULL_POINTER;
    }

    //suppose isp pipeline is 24bit, expand_px left shift 12bit
    uint8_t expand_px[64] = {22, 20, 12, 20, 20, 20, 20, 19, 19, 19, 19, 19, 18, 18, 18, 18, 18,
                            18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 17, 17, 17, 17, 12};//dx_exp[i], index number, x[i]=x[i-1]+2^dx_exp[i]
    memcpy(pCurve->compandPx, expand_px, sizeof(expand_px));

    pCurve->compandXData[0] = 0;//x value in expand curve
    pCurve->compandYData[0] = 0;//y value in expand curve
    for(int i=1; i<65; i++){
        if (pCurve->compandXData[i-1] == 0 && pCurve->compandPx[i-1] > 0){
            pCurve->compandXData[i] = pCurve->compandXData[i-1] + ((1 << pCurve->compandPx[i-1]) - 1);
        } else if (pCurve->compandXData[i-1] > 0 && pCurve->compandPx[i-1] > 0){
            pCurve->compandXData[i] = pCurve->compandXData[i-1] + (1 << pCurve->compandPx[i-1]);
        } else if (pCurve->compandXData[i-1] > 0 && pCurve->compandPx[i-1] == 0){
            pCurve->compandXData[i] = pCurve->compandXData[i-1];
        } else {
            TRACE(Ox08b40_INFO, "%s: invalid paramter\n", __func__);
            return RET_INVALID_PARM;
        }
    }

    uint16_t expandXValue[34]={0, 1023, 1279, 1279, 1535, 1791, 2047, 2303, 2431, 2559, 2687, 2815, 2943, 3007, 3071,
                            3135, 3199, 3263, 3327, 3391, 3455, 3519, 3583, 3647, 3711, 3775, 3839, 3903, 3967, 3999,
                            4031, 4063, 4095, 4095};
    uint32_t expandYValue[34]={0, 1023, 2047, 2047, 4095, 8191, 12287, 16383, 20479, 24575, 32767, 40959, 49151, 57343,
                            65535, 81919, 98303, 114687, 131071, 163839, 196607, 262143, 393215, 524287, 786431, 1048575,
                            1572863, 2097151, 3145727, 4194303, 8388607, 12582911, 16777215, 16777215};
    float32_t slope[33] = {0};
    for(int i=0; i<33; i++){
        slope[i] = (expandYValue[i+1] - expandYValue[i])/(expandXValue[i+1] - expandXValue[i]);
    }

    for(int i=1; i<65; i++){
        for(int j=1; j<34; j++){
            if(pCurve->compandXData[i] >= expandXValue[j-1] && pCurve->compandXData[i] < expandXValue[j]){
                pCurve->compandYData[i] = expandYValue[j-1] + (pCurve->compandXData[i] - expandXValue[j-1]) * slope[j-1];
            }
        }
    }

    TRACE(Ox08b40_INFO, "%s: (exit)\n", __func__);
    return (result);
}

RESULT Ox08b40_IsiGetSensorIss(IsiSensor_t *pIsiSensor)
{
    RESULT result = RET_SUCCESS;
    static const char SensorName[16] = "Ox08b40";
    TRACE( Ox08b40_INFO, "%s (enter)\n", __func__);

    if ( pIsiSensor != NULL ) {
        pIsiSensor->pszName                             = SensorName;
        pIsiSensor->pIsiCreateIss                       = Ox08b40_IsiCreateIss;
        pIsiSensor->pIsiOpenIss                         = Ox08b40_IsiOpenIss;
        pIsiSensor->pIsiCloseIss                        = Ox08b40_IsiCloseIss;
        pIsiSensor->pIsiReleaseIss                      = Ox08b40_IsiReleaseIss;
        pIsiSensor->pIsiReadRegIss                      = Ox08b40_IsiReadRegIss;
        pIsiSensor->pIsiWriteRegIss                     = Ox08b40_IsiWriteRegIss;
        pIsiSensor->pIsiGetModeIss                      = Ox08b40_IsiGetModeIss;
        pIsiSensor->pIsiEnumModeIss                     = Ox08b40_IsiEnumModeIss;
        pIsiSensor->pIsiGetCapsIss                      = Ox08b40_IsiGetCapsIss;
        pIsiSensor->pIsiCheckConnectionIss              = Ox08b40_IsiCheckConnectionIss;
        pIsiSensor->pIsiGetRevisionIss                  = Ox08b40_IsiGetRevisionIss;
        pIsiSensor->pIsiSetStreamingIss                 = Ox08b40_IsiSetStreamingIss;

        /* AEC */
        pIsiSensor->pIsiGetAeBaseInfoIss                = Ox08b40_pIsiGetAeBaseInfoIss;
        pIsiSensor->pIsiGetAGainIss                     = Ox08b40_IsiGetAGainIss;
        pIsiSensor->pIsiSetAGainIss                     = Ox08b40_IsiSetAGainIss;
        pIsiSensor->pIsiGetDGainIss                     = Ox08b40_IsiGetDGainIss;
        pIsiSensor->pIsiSetDGainIss                     = Ox08b40_IsiSetDGainIss;
        pIsiSensor->pIsiGetIntTimeIss                   = Ox08b40_IsiGetIntTimeIss;
        pIsiSensor->pIsiSetIntTimeIss                   = Ox08b40_IsiSetIntTimeIss;
        pIsiSensor->pIsiGetFpsIss                       = Ox08b40_IsiGetFpsIss;
        pIsiSensor->pIsiSetFpsIss                       = Ox08b40_IsiSetFpsIss;

        /* SENSOR ISP */
        pIsiSensor->pIsiGetIspStatusIss                 = Ox08b40_IsiGetIspStatusIss;
        pIsiSensor->pIsiSetWBIss                        = Ox08b40_IsiSetWBIss;
        pIsiSensor->pIsiGetWBIss                        = Ox08b40_IsiGetWBIss;
        pIsiSensor->pIsiSetBlcIss                       = Ox08b40_IsiSetBlcIss;
        pIsiSensor->pIsiGetBlcIss                       = Ox08b40_IsiGetBlcIss;

        /* SENSOE OTHER FUNC*/
        pIsiSensor->pIsiSetTpgIss                       = Ox08b40_IsiSetTpgIss;
        pIsiSensor->pIsiGetTpgIss                       = Ox08b40_IsiGetTpgIss;
        pIsiSensor->pIsiGetExpandCurveIss               = Ox08b40_IsiGetExpandCurveIss;

        /* AF */
        pIsiSensor->pIsiFocusCreateIss                  = NULL;
        pIsiSensor->pIsiFocusReleaseIss                 = NULL;
        pIsiSensor->pIsiFocusGetCalibrateIss            = NULL;
        pIsiSensor->pIsiFocusSetIss                     = NULL;
        pIsiSensor->pIsiFocusGetIss                     = NULL;

    } else {
        result = RET_NULL_POINTER;
    }

    TRACE( Ox08b40_INFO, "%s (exit)\n", __func__);
    return ( result );
}

/*****************************************************************************
* each sensor driver need declare this struct for isi load
*****************************************************************************/
IsiCamDrvConfig_t Ox08b40_IsiCamDrvConfig = {
    .cameraDriverID      = 0x580841,
    .pIsiGetSensorIss    = Ox08b40_IsiGetSensorIss,
};
