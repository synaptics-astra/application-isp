// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Synaptics Incorporated

#ifndef __DUMMYSENSOR_PRIV_H__
#define __DUMMYSENSOR_PRIV_H__

#include <ebase/types.h>
#include <common/return_codes.h>
#include <hal/hal_api.h>
#include <isi/isi_common.h>
#include <isi/isi_vvsensor.h>
#include "isi_priv.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct DummySensor_Context_s
{
    IsiSensorContext_t        isiCtx;                 /**< common context of ISI and ISI driver layer; @note: MUST BE FIRST IN DRIVER CONTEXT */
    IsiSensorMode_t           sensorMode;

    uint32_t                  maxFps;
    uint32_t                  minFps;
    uint32_t                  currFps;
    bool_t                    configured;             /**< flags that config was applied to sensor */
    bool_t                    streaming;              /**< flags that csensor is streaming data */
    bool_t                    testPattern;            /**< flags that sensor is streaming test-pattern */
    bool_t                    isAfpsRun;              /**< if true, just do anything required for Afps parameter calculation, but DON'T access SensorHW! */

    float                     oneLineExpTime;
    uint16_t                  maxIntegrationLine;
    uint16_t                  minIntegrationLine;
    uint16_t                  minVSIntegrationLine;
    uint16_t                  maxVSIntegrationLine;

    uint16_t                  frameLengthLines;       /**< frame line length */
    uint16_t                  curFrameLengthLines;

    float                     aecMinGain;
    float                     aecMaxGain;
    float                     aecMinIntegrationTime;
    float                     aecMaxIntegrationTime;
    float                     aecMinVSIntegrationTime;
    float                     aecMaxVSIntegrationTime;
    float                     aecIntegrationTimeIncrement; /**< _smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application) */
    float                     aecGainIncrement;            /**< _smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application) */

    float                     aecCurLongIntegrationTime;
    float                     aecCurIntegrationTime;
    float                     aecCurVSIntegrationTime;
    float                     aecCurLongGain;
    float                     aecCurGain;
    float                     aecCurVSGain;
    float                     curLongAgain;
    float                     curLongDgain;
    float                     curAgain;
    float                     curDgain;
    float                     curVSAgain;
    float                     curVSDgain;

    bool                      groupHold;
    uint32_t                  oldGain;
    uint32_t                  oldIntegrationTime;

    IsiGainInfo_t             aGain;
    IsiGainInfo_t             aVSGain;
    IsiGainInfo_t             dGain;
    IsiGainInfo_t             dVSGain;

    IsiSensorWb_t             sensorWb;
    IsiSensorBlc_t            sensorBlc;
} DummySensor_Context_t;

static RESULT DummySensor_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle);
static RESULT DummySensor_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode);
static RESULT DummySensor_IsiCloseIss(IsiSensorHandle_t handle);
static RESULT DummySensor_IsiReleaseIss(IsiSensorHandle_t handle);
static RESULT DummySensor_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps);
static RESULT DummySensor_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on);
static RESULT DummySensor_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue);
static RESULT DummySensor_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo);
static RESULT DummySensor_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain);
static RESULT DummySensor_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain);
static RESULT DummySensor_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain);
static RESULT DummySensor_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain);
static RESULT DummySensor_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime);
static RESULT DummySensor_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime);
static RESULT DummySensor_SetIntTime(IsiSensorHandle_t handle, float newIntegrationTime);

#ifdef __cplusplus
}
#endif

/* @} DummySensorpriv */

#endif    /* __DUMMYSENSORPRIV_H__ */
