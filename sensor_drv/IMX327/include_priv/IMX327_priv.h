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
 * @file IMX327_priv.h
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
 * @defgroup IMX327_priv
 * @{
 *
 */
#ifndef __IMX327_PRIV_H__
#define __IMX327_PRIV_H__

#include <ebase/types.h>
#include <common/return_codes.h>
#include <hal/hal_api.h>
#include <isi/isi_common.h>
#include <isi/isi_vvsensor.h>



#ifdef __cplusplus
extern "C"
{
#endif

typedef struct IMX327_Context_s
{
    IsiSensorContext_t     isiCtx;                 /**< common context of ISI and ISI driver layer; @note: MUST BE FIRST IN DRIVER CONTEXT */

    IsiSensorMode_t        sensorMode;

    uint32_t               maxFps;
    uint32_t               minFps;
    uint32_t               currFps;

    bool_t                 configured;             /**< flags that config was applied to sensor */
    bool_t                 streaming;              /**< flags that csensor is streaming data */
    bool_t                 testPattern;            /**< flags that sensor is streaming test-pattern */
    bool_t                 isAfpsRun;              /**< if true, just do anything required for Afps parameter calculation, but DON'T access SensorHW! */

    float                  oneLineExpTime;
    uint16_t               maxIntegrationLine;
    uint16_t               minIntegrationLine;
    uint16_t               minVSIntegrationLine;
    uint16_t               maxVSIntegrationLine;

    uint16_t               frameLengthLines;       /**< frame line length */
    uint16_t               curFrameLengthLines;

    float                  aecMinGain;
    float                  aecMaxGain;
    float                  aecMinIntegrationTime;
    float                  aecMaxIntegrationTime;
    float                  aecMinVSIntegrationTime;
    float                  aecMaxVSIntegrationTime;

    float                  aecIntegrationTimeIncrement; /**< _smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application) */
    float                  aecGainIncrement;            /**< _smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application) */

    float                  aecCurGain;
    float                  aecCurIntegrationTime;
    float                  aecCurVSGain;
    float                  aecCurVSIntegrationTime;
    float                  curAgain;
    float                  curDgain;
    float                  curVSAgain;
    float                  curVSDgain;

    bool                   groupHold;
    uint32_t               oldGain;
    uint32_t               oldIntegrationTime;

    IsiGainInfo_t          aGain;
    IsiGainInfo_t          aVSGain;
    IsiGainInfo_t          dGain;
    IsiGainInfo_t          dVSGain;

} IMX327_Context_t;

static RESULT IMX327_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle);
static RESULT IMX327_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode);
static RESULT IMX327_IsiCloseIss(IsiSensorHandle_t handle);
static RESULT IMX327_IsiReleaseIss(IsiSensorHandle_t handle);
static RESULT IMX327_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t * pCaps);
static RESULT IMX327_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on);
static RESULT IMX327_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue);
static RESULT IMX327_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo);
static RESULT IMX327_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain);
static RESULT IMX327_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain);
static RESULT IMX327_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain);
static RESULT IMX327_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain);
static RESULT IMX327_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime);
static RESULT IMX327_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime);
static RESULT IMX327_SetIntTime(IsiSensorHandle_t handle, float newIntegrationTime);
static RESULT IMX327_SetVSIntTime(IsiSensorHandle_t handle, float newIntegrationTime);

static uint16_t IMX327_mipi4lane_1080p_init[][2] =
{
    {0x3000, 0x01},
    {0x3002, 0x01},
    {0x3005, 0x01},
    {0x3007, 0x00},
    {0x3009, 0x12},
    {0x300a, 0xf0},
    {0x3018, 0x65},
    {0x3019, 0x04},
    {0x301c, 0x30},
    {0x303c, 0x00},
    {0x303d, 0x00},
    {0x303e, 0x50},
    {0x303f, 0x04},
    {0x3040, 0x00},
    {0x3041, 0x00},
    {0x3042, 0x9c},
    {0x3043, 0x07},
    {0x301d, 0x11},
    {0x3046, 0x01},
    {0x3048, 0x10},
    {0x305C, 0x18},
    {0x305D, 0x03},
    {0x305E, 0x20},
    {0x305F, 0x01},
    {0x3011, 0x0a},
    {0x3011, 0x02},
    {0x3012, 0x64},
    {0x3013, 0x00},
    {0x309e, 0x4a},
    {0x309f, 0x4a},
    {0x30d2, 0x19},
    {0x30d7, 0x03},
    {0x3128, 0x04},
    {0x313b, 0x41},
    {0x313b, 0x61},
    {0x3129, 0x00},
    {0x315e, 0x1a},
    {0x3164, 0x1a},
    {0x317c, 0x00},
    {0x31ec, 0x0e},
    {0x3480, 0x49},
    {0x3405, 0x20},
    {0x3407, 0x03},
    {0x3414, 0x0a},
    {0x3418, 0x49},
    {0x3419, 0x04},
    {0x3441, 0x0c},
    {0x3442, 0x0c},
    {0x3443, 0x03},
    {0x3444, 0x20},
    {0x3445, 0x25},
    {0x3446, 0x47},
    {0x3447, 0x00},
    {0x3448, 0x1f},
    {0x3449, 0x00},
    {0x344A, 0x17},
    {0x344B, 0x00},
    {0x344C, 0x0F},
    {0x344D, 0x00},
    {0x344E, 0x17},
    {0x344F, 0x00},
    {0x3450, 0x47},
    {0x3451, 0x00},
    {0x3452, 0x0F},
    {0x3453, 0x00},
    {0x3454, 0x0f},
    {0x3455, 0x00},
    {0x3472, 0x9C},
    {0x3473, 0x07},
    {0x3020, 0x00},
    {0x3021, 0x00},
    {0x3014, 0x00},
    {0x304b, 0x0a},
};

#ifdef __cplusplus
}
#endif

/* @} IMX327priv */

#endif    /* __IMX327PRIV_H__ */

