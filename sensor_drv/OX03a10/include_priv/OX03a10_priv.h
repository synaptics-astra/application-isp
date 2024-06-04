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
/**
 * @file OX03a10_priv.h
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
 * @defgroup ox03a10_priv
 * @{
 *
 */
#ifndef __OX03a10_PRIV_H__
#define __OX03a10_PRIV_H__

#include <ebase/types.h>
#include <common/return_codes.h>
#include <hal/hal_api.h>
#include <isi/isi_common.h>
#include "vvsensor.h"



#ifdef __cplusplus
extern "C"
{
#endif


typedef struct OX03a10_Context_s
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

    float               one_line_exp_time;
    uint16_t            MaxIntegrationLine;
    uint16_t            MinIntegrationLine;
    uint32_t            gain_accuracy;

    uint16_t            FrameLengthLines;       /**< frame line length */
    uint16_t            CurFrameLengthLines;

    float               AecMinGain;
    float               AecMaxGain;
    float               AecMinIntegrationTime;
    float               AecMaxIntegrationTime;

    float               AecIntegrationTimeIncrement; /**< _smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application) */
    float               AecGainIncrement;            /**< _smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application) */

    float               AecCurIntegrationTime;
    float               AecCurVSIntegrationTime;
    float               AecCurLongIntegrationTime;
    float               AecCurGain;
    float               AecCurVSGain;
    float               AecCurLongGain;

    uint32_t            LastExpLine;
    uint32_t            LastVsExpLine;
    uint32_t            LastLongExpLine;

    uint32_t            LastGain;
    uint32_t            LastVsGain;
    uint32_t            LastLongGain;

    bool                GroupHold;
    uint32_t            OldGain;
    uint32_t            OldVsGain;
    uint32_t            OldIntegrationTime;
    uint32_t            OldVsIntegrationTime;
    uint32_t            OldGainHcg;
    uint32_t            OldAGainHcg;
    uint32_t            OldGainLcg;
    uint32_t            OldAGainLcg;
    int                 subdev;
    uint8_t             pattern;

    float               CurHdrRatio;
} OX03a10_Context_t;

static RESULT OX03a10_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig);

static RESULT OX03a10_IsiReleaseIss(IsiSensorHandle_t handle);

static RESULT OX03a10_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps);

static RESULT OX03a10_IsiSetupIss(IsiSensorHandle_t handle);

static RESULT OX03a10_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on);

static RESULT OX03a10_IsiSetPowerIss(IsiSensorHandle_t handle, bool_t on);

static RESULT OX03a10_IsiGetRevisionIss(IsiSensorHandle_t handle,
                               uint32_t *pValue);

static RESULT OX03a10_IsiGetGainLimitsIss(IsiSensorHandle_t handle,
                             float *pMinGain,
                             float *pMaxGain);

static RESULT OX03a10_IsiGetIntegrationTimeLimitsIss(IsiSensorHandle_t
                                 handle,
                                 float *pMinIntegrationTime,
                                 float *pMaxIntegrationTime);

static RESULT OX03a10_IsiExposureControlIss(IsiSensorHandle_t handle,
                            float NewGain,
                            float NewIntegrationTime,
                            uint8_t *pNumberOfFramesToSkip,
                            float *pSetGain,
                            float *pSetIntegrationTime,
                            float *hdr_ratio);

static RESULT OX03a10_IsiGetGainIss(IsiSensorHandle_t handle,
                        float *pSetGain);

static RESULT OX03a10_IsiGetVSGainIss(IsiSensorHandle_t handle,
                          float *pSetGain);

static RESULT OX03a10_IsiGetGainIncrementIss(IsiSensorHandle_t handle,
                             float *pIncr);

static RESULT OX03a10_IsiSetGainIss(IsiSensorHandle_t handle,
                        float NewGain, float *pSetGain,
                        float *hdr_ratio);

static RESULT OX03a10_IsiSetVSGainIss(IsiSensorHandle_t handle,
                          float NewIntegrationTime,
                          float NewGain, float *pSetGain,
                          float *hdr_ratio);

static RESULT OX03a10_IsiGetIntegrationTimeIss(IsiSensorHandle_t handle,
                               float *pSetIntegrationTime);

static RESULT OX03a10_IsiGetVSIntegrationTimeIss(IsiSensorHandle_t
                             handle,
                             float *pSetIntegrationTime);

static RESULT OX03a10_IsiGetIntegrationTimeIncrementIss(IsiSensorHandle_t handle,
                             float *pIncr);

static RESULT OX03a10_IsiSetIntegrationTimeIss(IsiSensorHandle_t handle,
                               float NewIntegrationTime,
                               float *pSetIntegrationTime,
                               uint8_t *pNumberOfFramesToSkip,
                               float *hdr_ratio);

static RESULT OX03a10_IsiSetVSIntegrationTimeIss(IsiSensorHandle_t
                             handle,
                             float NewIntegrationTime,
                             float *pSetIntegrationTime,
                             uint8_t *pNumberOfFramesToSkip,
                             float *hdr_ratio);


#ifdef __cplusplus
}
#endif

/* @} ox03a10priv */

#endif    /* __OX03a10PRIV_H__ */

