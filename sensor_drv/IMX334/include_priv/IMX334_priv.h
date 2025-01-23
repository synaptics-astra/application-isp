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

/**
 * @file IMX334_priv.h
 *
 * @brief Interface description for image sensor specific implementation (iss).
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup IMX334_priv
 * @{
 *
 */
#ifndef __IMX334_PRIV_H__
#define __IMX334_PRIV_H__

#include <ebase/types.h>
#include <common/return_codes.h>
#include <hal/hal_api.h>
#include <isi/isi_common.h>
#include "vvsensor.h"
#include "isi_priv.h"


#ifdef __cplusplus
extern "C"
{
#endif

typedef struct IMX334_Context_s
{
    IsiSensorContext_t  IsiCtx;                 /**< common context of ISI and ISI driver layer; @note: MUST BE FIRST IN DRIVER CONTEXT */

    struct vvsensor_mode_s SensorMode;
    uint32_t            KernelDriverFlag;
    char                SensorRegCfgFile[64];

    uint32_t              HdrMode;
    uint32_t              Resolution;
    uint32_t              MaxFps;
    uint32_t              MinFps;
    uint32_t              CurrFps;
    //// modify below here ////

    IsiCaps_t           CapsConfig;                 /**< sensor configuration */
    bool_t              Configured;             /**< flags that config was applied to sensor */
    bool_t              Streaming;              /**< flags that csensor is streaming data */
    bool_t              TestPattern;            /**< flags that sensor is streaming test-pattern */

    bool_t              isAfpsRun;              /**< if true, just do anything required for Afps parameter calculation, but DON'T access SensorHW! */

    float32_t               one_line_exp_time;
    uint16_t            MaxIntegrationLine;
    uint16_t            MinIntegrationLine;
    uint32_t            gain_accuracy;

    uint16_t            FrameLengthLines;       /**< frame line length */
    uint16_t            CurFrameLengthLines;

    float32_t               AecMinGain;
    float32_t               AecMaxGain;
    float32_t               AecMinIntegrationTime;
    float32_t               AecMaxIntegrationTime;

    float32_t               AecIntegrationTimeIncrement; /**< _smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application) */
    float32_t               AecGainIncrement;            /**< _smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application) */

    float32_t               AecCurGain;
    float32_t               AecCurIntegrationTime;
    float32_t               AecCurGainSEF1;
    float32_t               AecCurIntegrationTimeSEF1;

    bool                GroupHold;
    uint32_t            OldGain;
    uint32_t            OldIntegrationTime;
    uint32_t            OldGainSEF1;
    uint32_t            OldIntegrationTimeSEF1;

    int                 subdev;
    bool                enableHdr;
    uint8_t             pattern;
} IMX334_Context_t;

static RESULT IMX334_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig);
static RESULT IMX334_IsiReleaseIss(IsiSensorHandle_t handle);
static RESULT IMX334_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t * pCaps);
static RESULT IMX334_IsiSetupIss(IsiSensorHandle_t handle);
static RESULT IMX334_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on);
static RESULT IMX334_IsiSetPowerIss(IsiSensorHandle_t handle, bool_t on);
static RESULT IMX334_IsiGetRevisionIss(IsiSensorHandle_t handle, 
								uint32_t *pValue);
static RESULT IMX334_IsiGetGainLimitsIss(IsiSensorHandle_t handle,
								float32_t *pMinGain, 
								float32_t *pMaxGain);
static RESULT IMX334_IsiGetIntegrationTimeLimitsIss(IsiSensorHandle_t handle, 
								float32_t *pMinIntegrationTime,
								float32_t *pMaxIntegrationTime);
static RESULT IMX334_IsiExposureControlIss(IsiSensorHandle_t handle,
								float32_t NewGain, 
								float32_t NewIntegrationTime,
								uint8_t *pNumberOfFramesToSkip,
                                float32_t *pSetGain, 
								float32_t *pSetIntegrationTime,
								float32_t *hdr_ratio);
static RESULT IMX334_IsiGetGainIss(IsiSensorHandle_t handle,float32_t *pSetGain);

static RESULT IMX334_IsiGetSEF1GainIss(IsiSensorHandle_t handle,float32_t *pSetGain);

static RESULT IMX334_IsiGetGainIncrementIss(IsiSensorHandle_t handle,float32_t *pIncr);

static RESULT IMX334_IsiSetGainIss(IsiSensorHandle_t handle,
								float32_t NewGain, 
								float32_t *pSetGain,
								float32_t *hdr_ratio);

static RESULT IMX334_IsiSetSEF1GainIss(IsiSensorHandle_t handle,
								float32_t NewIntegrationTime,
								float32_t NewGain, 
								float32_t *pSetGain,
								float32_t *hdr_ratio);

static RESULT IMX334_IsiGetIntegrationTimeIss(IsiSensorHandle_t handle,
								float32_t *pSetIntegrationTime);

static RESULT IMX334_IsiGetSEF1IntegrationTimeIss(IsiSensorHandle_t handle,
								float32_t *pSetIntegrationTime);

static RESULT IMX334_IsiGetIntegrationTimeIncrementIss(IsiSensorHandle_t handle,
								float32_t *pIncr);

static RESULT IMX334_IsiSetIntegrationTimeIss(IsiSensorHandle_t handle,
								float32_t NewIntegrationTime, 
								float32_t *pSetIntegrationTime,
								uint8_t *pNumberOfFramesToSkip,
								float32_t *hdr_ratio);

static RESULT IMX334_IsiSetSEF1IntegrationTimeIss(IsiSensorHandle_t handle,
								float32_t NewIntegrationTime,
								float32_t *pSetIntegrationTime,
								uint8_t *pNumberOfFramesToSkip,
								float32_t *hdr_ratio);


#ifdef __cplusplus
}
#endif

/* @} IMX334priv */

#endif    /* __IMX334PRIV_H__ */

