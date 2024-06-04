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
 * @file SC1330T_priv.h
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
 * @defgroup sc1330t_priv
 * @{
 *
 */
#ifndef __SC1330T_PRIV_H__
#define __SC1330T_PRIV_H__

#include <ebase/types.h>
#include <common/return_codes.h>
#include <hal/hal_api.h>
#include <isi/isi_common.h>
#include <isi/isi_vvsensor.h>



#ifdef __cplusplus
extern "C"
{
#endif

typedef struct SC1330T_Context_s
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

    uint16_t               frameLengthLines;       /**< frame line length */
    uint16_t               curFrameLengthLines;

    float                  aecMinGain;
    float                  aecMaxGain;
    float                  aecMinIntegrationTime;
    float                  aecMaxIntegrationTime;

    float                  aecIntegrationTimeIncrement; /**< _smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application) */
    float                  aecGainIncrement;            /**< _smallest_ increment the sensor/driver can handle (e.g. used for sliders in the application) */

    float                  aecCurGain;
    float                  aecCurIntegrationTime;
    float                  curAgain;
    float                  curDgain;

    bool                   groupHold;
    uint32_t               oldGain;
    uint32_t               oldIntegrationTime;

    IsiGainInfo_t          aGain;
    IsiGainInfo_t          dGain;

} SC1330T_Context_t;

static RESULT SC1330T_IsiCreateIss(IsiSensorInstanceConfig_t *pConfig, IsiSensorHandle_t *pHandle);
static RESULT SC1330T_IsiOpenIss(IsiSensorHandle_t handle, uint32_t mode);
static RESULT SC1330T_IsiCloseIss(IsiSensorHandle_t handle);
static RESULT SC1330T_IsiReleaseIss(IsiSensorHandle_t handle);
static RESULT SC1330T_IsiGetCapsIss(IsiSensorHandle_t handle, IsiCaps_t * pCaps);
static RESULT SC1330T_IsiSetStreamingIss(IsiSensorHandle_t handle, bool_t on);
static RESULT SC1330T_IsiGetRevisionIss(IsiSensorHandle_t handle, uint32_t *pValue);
static RESULT SC1330T_IsiGetAeBaseInfoIss(IsiSensorHandle_t handle, IsiAeBaseInfo_t *pAeBaseInfo);
static RESULT SC1330T_IsiGetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain);
static RESULT SC1330T_IsiGetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain);
static RESULT SC1330T_IsiSetAGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorAGain);
static RESULT SC1330T_IsiSetDGainIss(IsiSensorHandle_t handle, IsiSensorGain_t *pSensorDGain);
static RESULT SC1330T_IsiGetIntTimeIss(IsiSensorHandle_t handle, IsiSensorIntTime_t *pSensorIntTime);
static RESULT SC1330T_IsiSetIntTimeIss(IsiSensorHandle_t handle, const IsiSensorIntTime_t *pSensorIntTime);


static uint16_t SC1330T_mipi1lane_1280_960_10fps_init[][2] =
{
    {0x0103, 0x01},
    {0x0100, 0x00},
    {0x36e9, 0x80},
    {0x36f9, 0x80},
    {0x3018, 0x12},
    {0x3019, 0x0e},
    {0x301f, 0x90},
    {0x3031, 0x0a},
    {0x3037, 0x20},
    {0x320e, 0x07},
    {0x320f, 0xd0},
    {0x3211, 0x06},
    {0x3251, 0x98},
    {0x3253, 0x08},
    {0x325f, 0x0a},
    {0x3304, 0x40},
    {0x3306, 0x70},
    {0x3309, 0x70},
    {0x330a, 0x01},
    {0x330b, 0xf0},
    {0x330d, 0x30},
    {0x3310, 0x0e},
    {0x3314, 0x92},
    {0x331e, 0x31},
    {0x331f, 0x61},
    {0x335d, 0x60},
    {0x3364, 0x5e},
    {0x3396, 0x08},
    {0x3397, 0x18},
    {0x3398, 0x38},
    {0x3399, 0x0c},
    {0x339a, 0x10},
    {0x339b, 0x1e},
    {0x339c, 0x70},
    {0x33af, 0x38},
    {0x360f, 0x21},
    {0x3621, 0xe8},
    {0x3632, 0x68},
    {0x3633, 0x33},
    {0x3634, 0x23},
    {0x3635, 0x20},
    {0x3637, 0x19},
    {0x3638, 0x08},
    {0x363b, 0x04},
    {0x363c, 0x06},
    {0x3670, 0x42},
    {0x3671, 0x05},
    {0x3672, 0x15},
    {0x3673, 0x15},
    {0x3674, 0xc0},
    {0x3675, 0x84},
    {0x3676, 0x88},
    {0x367a, 0x48},
    {0x367b, 0x58},
    {0x367c, 0x48},
    {0x367d, 0x58},
    {0x3699, 0x00},
    {0x369a, 0x00},
    {0x369b, 0x1f},
    {0x36a2, 0x48},
    {0x36a3, 0x58},
    {0x36a6, 0x48},
    {0x36a7, 0x58},
    {0x36ab, 0xc0},
    {0x36ac, 0x84},
    {0x36ad, 0x88},
    {0x36d0, 0x40},
    {0x36db, 0x04},
    {0x36dc, 0x14},
    {0x36dd, 0x14},
    {0x36de, 0x48},
    {0x36df, 0x58},
    {0x36ea, 0x3a},
    {0x36eb, 0x15},
    {0x36ec, 0x05},
    {0x36ed, 0x14},
    {0x36fa, 0x37},
    {0x36fb, 0x00},
    {0x36fc, 0x11},
    {0x36fd, 0x14},
    //{0x3902, 0x95}, //TPG
    {0x3e01, 0x7c},
    {0x3e02, 0xc0},//0x7cc0
    //{0x4501, 0xbc}, //TPG
    {0x450a, 0x71},
    {0x4800, 0x44},
    {0x4819, 0x04},
    {0x481b, 0x03},
    {0x481d, 0x08},
    {0x481f, 0x02},
    {0x4821, 0x07},
    {0x4823, 0x02},
    {0x4825, 0x02},
    {0x4827, 0x02},
    {0x4829, 0x03},
    {0x578a, 0x18},
    {0x578b, 0x10},
    {0x5793, 0x18},
    {0x5794, 0x10},
    {0x5799, 0x00},
    {0x36e9, 0x24},
    {0x36f9, 0x34},
};

static uint16_t SC1330T_mipi1lane_1280_960_15fps_init[][2] =
{
    {0x0103, 0x01},
    {0x0100, 0x00},
    {0x36e9, 0x80},
    {0x36f9, 0x80},
    {0x3018, 0x12},
    {0x3019, 0x0e},
    {0x301f, 0x91},
    {0x3031, 0x0a},
    {0x3037, 0x20},
    {0x320c, 0x05},
    {0x320d, 0xdc},
    {0x320e, 0x05},
    {0x320f, 0x00},
    {0x3211, 0x06},
    {0x3251, 0x98},
    {0x3253, 0x08},
    {0x325f, 0x0a},
    {0x3304, 0x40},
    {0x3306, 0x70},
    {0x3309, 0x70},
    {0x330a, 0x01},
    {0x330b, 0xf0},
    {0x330d, 0x30},
    {0x3310, 0x0e},
    {0x3314, 0x92},
    {0x331e, 0x31},
    {0x331f, 0x61},
    {0x335d, 0x60},
    {0x3364, 0x5e},
    {0x3396, 0x08},
    {0x3397, 0x18},
    {0x3398, 0x38},
    {0x3399, 0x0c},
    {0x339a, 0x10},
    {0x339b, 0x1e},
    {0x339c, 0x70},
    {0x33af, 0x38},
    {0x360f, 0x21},
    {0x3621, 0xe8},
    {0x3632, 0x68},
    {0x3633, 0x33},
    {0x3634, 0x23},
    {0x3635, 0x20},
    {0x3637, 0x19},
    {0x3638, 0x08},
    {0x363b, 0x04},
    {0x363c, 0x06},
    {0x3670, 0x42},
    {0x3671, 0x05},
    {0x3672, 0x15},
    {0x3673, 0x15},
    {0x3674, 0xc0},
    {0x3675, 0x84},
    {0x3676, 0x88},
    {0x367a, 0x48},
    {0x367b, 0x58},
    {0x367c, 0x48},
    {0x367d, 0x58},
    {0x3699, 0x00},
    {0x369a, 0x00},
    {0x369b, 0x1f},
    {0x36a2, 0x48},
    {0x36a3, 0x58},
    {0x36a6, 0x48},
    {0x36a7, 0x58},
    {0x36ab, 0xc0},
    {0x36ac, 0x84},
    {0x36ad, 0x88},
    {0x36d0, 0x40},
    {0x36db, 0x04},
    {0x36dc, 0x14},
    {0x36dd, 0x14},
    {0x36de, 0x48},
    {0x36df, 0x58},
    {0x36ea, 0x3a},
    {0x36eb, 0x15},
    {0x36ec, 0x05},
    {0x36ed, 0x14},
    {0x36fa, 0x37},
    {0x36fb, 0x00},
    {0x36fc, 0x11},
    {0x36fd, 0x14},
    {0x3e01, 0x4f},
    {0x3e02, 0xc0},
    {0x450a, 0x71},
    {0x4800, 0x44},
    {0x4819, 0x04},
    {0x481b, 0x03},
    {0x481d, 0x08},
    {0x481f, 0x02},
    {0x4821, 0x07},
    {0x4823, 0x02},
    {0x4825, 0x02},
    {0x4827, 0x02},
    {0x4829, 0x03},
    {0x578a, 0x18},
    {0x578b, 0x10},
    {0x5793, 0x18},
    {0x5794, 0x10},
    {0x5799, 0x00},
    {0x36e9, 0x24},
    {0x36f9, 0x34},
};


#ifdef __cplusplus
}
#endif

/* @} sc1330tpriv */

#endif    /* __SC1330TPRIV_H__ */

