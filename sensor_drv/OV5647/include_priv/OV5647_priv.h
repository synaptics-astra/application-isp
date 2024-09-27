// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Synaptics Incorporated

#ifndef __OV5647_PRIV_H__
#define __OV5647_PRIV_H__

#include <ebase/types.h>
#include <common/return_codes.h>
#include <hal/hal_api.h>
#include <isi/isi_common.h>
#include <isi/isi_vvsensor.h>



#ifdef __cplusplus
extern "C"
{
#endif

typedef struct OV5647_Context_s
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
} OV5647_Context_t;

static RESULT OV5647_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle);
static RESULT OV5647_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode);
static RESULT OV5647_IsiCloseIss(IsiSensorHandle_t handle);
static RESULT OV5647_IsiReleaseIss(IsiSensorHandle_t handle);
static RESULT OV5647_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t *pCaps);
static RESULT OV5647_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on);
static RESULT OV5647_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue);
static RESULT OV5647_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo);
static RESULT OV5647_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain);
static RESULT OV5647_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain);
static RESULT OV5647_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain);
static RESULT OV5647_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain);
static RESULT OV5647_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime);
static RESULT OV5647_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime);
static RESULT OV5647_SetIntTime(IsiSensorHandle_t handle, float newIntegrationTime);

static uint16_t OV5647_mipi2lane_480p_init[][2] =
{
    {0x0100, 0x00},
    {0x0103, 0x01},
    {0x3035, 0x11},
    {0x3036, 0x46},
    {0x303c, 0x11},
    {0x3821, 0x07},// mirror
    {0x3820, 0x42},// flip
    {0x370c, 0x03},
    {0x3612, 0x59},
    {0x3618, 0x00},
    {0x5000, 0x06},
    {0x5003, 0x0a},
    {0x5a00, 0x08},
    {0x3000, 0xff},
    {0x3001, 0xff},
    {0x3002, 0xff},
    {0x301d, 0xf0},
    {0x3a18, 0x00},
    {0x3a19, 0xf8},
    {0x3c01, 0x80},
    {0x3b07, 0x0c},
    {0x380c, 0x07},// timing HTS MSb
    {0x380d, 0x3c},// timing HTS LSb
    {0x380e, 0x01},// timing VTS MSb
    {0x380f, 0xf8},// timing VTS LSb
    {0x3814, 0x71},
    {0x3815, 0x71},
    {0x3708, 0x64},
    {0x3709, 0x52},
    {0x3808, 0x02},// 640 MSb
    {0x3809, 0x80},// 640 LSb
    {0x380a, 0x01},// 480 MSb
    {0x380b, 0xe0},// 480 LSb
    {0x3800, 0x00},
    {0x3801, 0x10},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x0a},
    {0x3805, 0x2f},
    {0x3806, 0x07},
    {0x3807, 0x9f},
    {0x3630, 0x2e},
    {0x3632, 0xe2},
    {0x3633, 0x23},
    {0x3634, 0x44},
    {0x3620, 0x64},
    {0x3621, 0xe0},
    {0x3600, 0x37},
    {0x3704, 0xa0},
    {0x3703, 0x5a},
    {0x3715, 0x78},
    {0x3717, 0x01},
    {0x3731, 0x02},
    {0x370b, 0x60},
    {0x3705, 0x1a},
    {0x3f05, 0x02},
    {0x3f06, 0x10},
    {0x3f01, 0x0a},
    {0x3a08, 0x01},
    {0x3a09, 0x2e},
    {0x3a0a, 0x00},
    {0x3a0b, 0xfb},
    {0x3a0d, 0x02},
    {0x3a0e, 0x01},
    {0x3a0f, 0x58},
    {0x3a10, 0x50},
    {0x3a1b, 0x58},
    {0x3a1e, 0x50},
    {0x3a11, 0x60},
    {0x3a1f, 0x28},
    {0x4001, 0x02},
    {0x4004, 0x02},
    {0x4000, 0x09},
    {0x3000, 0x00},
    {0x3001, 0x00},
    {0x3002, 0x00},
    {0x301c, 0xfc},
    {0x3636, 0x06},
    {0x3016, 0x08},
    {0x3827, 0xec},
    {0x3018, 0x44},
    {0x3035, 0x21},
    {0x3106, 0xf5},
    {0x3034, 0x1a},
    {0x301c, 0xf8},
    {0x4800, 0x34},// line synchronization ON
    {0x3503, 0x03},// enable manual gain & manual exposure
    {0x350b, 0x64},// gain MSb
    {0x3501, 0x50},// exposure MSb
};

//1920x1080
static uint16_t OV5647_mipi2lane_1080p_init[][2] =
{
    // TODO: Fix 2k playback issue
    {0x0100, 0x00},
    {0x0103, 0x01},
    {0x3034, 0x1a},
    {0x3035, 0x21},
    {0x3036, 0x62},
    {0x303c, 0x11},
    {0x3106, 0xf5},
    {0x3821, 0x06},
    {0x3820, 0x00},
    {0x3827, 0xec},
    {0x370c, 0x03},
    {0x3612, 0x5b},
    {0x3618, 0x04},
    {0x5000, 0x06},
    {0x5002, 0x41},
    {0x5003, 0x08},
    {0x5a00, 0x08},
    {0x3000, 0x00},
    {0x3001, 0x00},
    {0x3002, 0x00},
    {0x3016, 0x08},
    {0x3017, 0xe0},
    {0x3018, 0x44},
    {0x301c, 0xf8},
    {0x301d, 0xf0},
    {0x3a18, 0x00},
    {0x3a19, 0xf8},
    {0x3c01, 0x80},
    {0x3b07, 0x0c},
    {0x380c, 0x09},
    {0x380d, 0x70},
    {0x380e, 0x04},// timing VTS MSb
    {0x380f, 0x50},// timing VTS LSb
    {0x3814, 0x11},
    {0x3815, 0x11},
    {0x3708, 0x64},
    {0x3709, 0x12},
    {0x3808, 0x07},
    {0x3809, 0x80},
    {0x380a, 0x04},
    {0x380b, 0x38},
    {0x3800, 0x01},
    {0x3801, 0x5c},
    {0x3802, 0x01},
    {0x3803, 0xb2},
    {0x3804, 0x08},
    {0x3805, 0xe3},
    {0x3806, 0x05},
    {0x3807, 0xf1},
    {0x3811, 0x04},
    {0x3813, 0x02},
    {0x3630, 0x2e},
    {0x3632, 0xe2},
    {0x3633, 0x23},
    {0x3634, 0x44},
    {0x3636, 0x06},
    {0x3620, 0x64},
    {0x3621, 0xe0},
    {0x3600, 0x37},
    {0x3704, 0xa0},
    {0x3703, 0x5a},
    {0x3715, 0x78},
    {0x3717, 0x01},
    {0x3731, 0x02},
    {0x370b, 0x60},
    {0x3705, 0x1a},
    {0x3f05, 0x02},
    {0x3f06, 0x10},
    {0x3f01, 0x0a},
    {0x3a08, 0x01},
    {0x3a09, 0x4b},
    {0x3a0a, 0x01},
    {0x3a0b, 0x13},
    {0x3a0d, 0x04},
    {0x3a0e, 0x03},
    {0x3a0f, 0x58},
    {0x3a10, 0x50},
    {0x3a1b, 0x58},
    {0x3a1e, 0x50},
    {0x3a11, 0x60},
    {0x3a1f, 0x28},
    {0x4001, 0x02},
    {0x4004, 0x04},
    {0x4000, 0x09},
    {0x4837, 0x19},
    {0x4800, 0x34},
    {0x3503, 0x03},
    {0x350b, 0x64},// gain MSb
    {0x3501, 0x50},// exposure MSb
};

//2560x1440
static uint16_t OV5647_mipi2lane_1440p_init[][2] =
{
    // TODO: Add support for 2560x1440
    {0x0100, 0x00},
};
#if 0
//2592x1944
static uint16_t OV5647_mipi2lane_1944p_init[][2] =
{
    {0x0100, 0x00},
};
#endif

#ifdef __cplusplus
}
#endif

/* @} ov5647priv */

#endif    /* __OV5647PRIV_H__ */

