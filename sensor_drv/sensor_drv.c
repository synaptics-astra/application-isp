/******************************************************************************\
|* Copyright (c) 2023 by VeriSilicon Holdings Co., Ltd. ("VeriSilicon")       *|
|* All Rights Reserved.                                                       *|
|*                                                                            *|
|* The material in this file is confidential and contains trade secrets of    *|
|* of VeriSilicon.  This is proprietary information owned or licensed by      *|
|* VeriSilicon.  No part of this work may be disclosed, reproduced, copied,   *|
|* transmitted, or used in any way for any purpose, without the express       *|
|* written permission of VeriSilicon.                                         *|
|*                                                                            *|
\******************************************************************************/

/* VeriSilicon 2023 */

/**
 * @file sensor_drv.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/


#include "sensor_drv.h"

#include <ebase/types.h>
#include <ebase/trace.h>
#include <common/return_codes.h>
#include <string.h>


CREATE_TRACER(SENSOR_DRV_INFO , "SENSOR_INFO: ", INFO, 1);
CREATE_TRACER(SENSOR_DRV_ERROR , "SENSOR_ERROR: ", ERROR, 1);


extern IsiCamDrvConfig_t OS08a20_IsiCamDrvConfig;
extern IsiCamDrvConfig_t IMX681_IsiCamDrvConfig;
extern IsiCamDrvConfig_t OV16E10_IsiCamDrvConfig;
extern IsiCamDrvConfig_t OV2775_IsiCamDrvConfig;
extern IsiCamDrvConfig_t OV2778_IsiCamDrvConfig;
extern IsiCamDrvConfig_t AR0820_IsiCamDrvConfig;
extern IsiCamDrvConfig_t OV10652_IsiCamDrvConfig;
extern IsiCamDrvConfig_t OX08a4Y_IsiCamDrvConfig;
//extern IsiCamDrvConfig_t OX03a10_IsiCamDrvConfig;
//extern IsiCamDrvConfig_t OS02k10_IsiCamDrvConfig;
//extern IsiCamDrvConfig_t OS02c10_IsiCamDrvConfig;
//extern IsiCamDrvConfig_t IMX334_IsiCamDrvConfig;
extern IsiCamDrvConfig_t IMX327_IsiCamDrvConfig;
extern IsiCamDrvConfig_t OV13b10_IsiCamDrvConfig;
extern IsiCamDrvConfig_t OV50a40_IsiCamDrvConfig;
extern IsiCamDrvConfig_t GC5603_IsiCamDrvConfig;
extern IsiCamDrvConfig_t SC1330T_IsiCamDrvConfig;
extern IsiCamDrvConfig_t OS04a10_IsiCamDrvConfig;
#ifdef DOLPHIN
extern IsiCamDrvConfig_t IMX258_IsiCamDrvConfig;
#endif

RESULT SensorDrvConfigMapping
(
    const char *pSensorName,
    IsiCamDrvConfig_t **pSensorConfig
)
{

    TRACE( SENSOR_DRV_INFO, "%s: (enter)\n", __func__);

    if ( pSensorName == NULL ) {
        return ( RET_NULL_POINTER );
    }

    SensorDrvConfig_t sensorConfig[] = {
        {"os08a20", &OS08a20_IsiCamDrvConfig},
        {"imx681" , &IMX681_IsiCamDrvConfig },
        {"ov16e10", &OV16E10_IsiCamDrvConfig},
        {"ov2775" , &OV2775_IsiCamDrvConfig },
        {"ov2778" , &OV2778_IsiCamDrvConfig },
        {"ar0820" , &AR0820_IsiCamDrvConfig },
        {"ov10652", &OV10652_IsiCamDrvConfig},
        {"ox08a4y", &OX08a4Y_IsiCamDrvConfig},
        //{"ox03a10", &OX03a10_IsiCamDrvConfig},
        //{"os02k10", &OS02k10_IsiCamDrvConfig},
        //{"os02c10", &OS02c10_IsiCamDrvConfig},
        //{"imx334" , &IMX334_IsiCamDrvConfig },
        {"imx327" ,  &IMX327_IsiCamDrvConfig },
        {"ov13b10" , &OV13b10_IsiCamDrvConfig },
        {"ov50a40" , &OV50a40_IsiCamDrvConfig },
        {"gc5603" , &GC5603_IsiCamDrvConfig },
        {"sc1330t" , &SC1330T_IsiCamDrvConfig },
        {"os04a10" , &OS04a10_IsiCamDrvConfig },
#ifdef DOLPHIN
        {"imx258" , &IMX258_IsiCamDrvConfig },
#endif
    };

    for (int i = 0; i < (int)(sizeof(sensorConfig)/sizeof(sensorConfig[0])); i++) {
        if (strcmp(pSensorName, sensorConfig[i].pSensorName) == 0) {
            *pSensorConfig = sensorConfig[i].pSensorConfig;
            TRACE( SENSOR_DRV_INFO, "%s: i=%d, match sensor name: %s success!!\n", __func__, i, sensorConfig[i].pSensorName);
            return RET_SUCCESS;
        }
    }

    TRACE(SENSOR_DRV_ERROR, "%s: Unsupport sensor %s !\n", __func__,pSensorName);
    return RET_NOTSUPP;
}


//RESULT HalGetSensorName(HalHandle_t HalHandle, char pSensorName[], uint16_t arraySize)
//{
//    RESULT result;
//    AdaptSensorInfo_t sensorInfo;
//    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
//    if(pHalCtx == NULL || pSensorName == NULL)
//    {
//        return RET_NULL_POINTER;
//    }
//
//    result = AdaptGetSensorInfo(pHalCtx->adaptHandle, &sensorInfo);
//    if(result != RET_SUCCESS)
//    {
//        TRACE(HAL_ERROR, "%s: get sensor name error in hal!\n", __func__);
//        return result;
//    }
//
//    snprintf(pSensorName, arraySize, "%s", sensorInfo.pSensorName);
//    return RET_SUCCESS;
//}
//
//RESULT HalGetSensorDrvName(HalHandle_t HalHandle, char pSensorDrvName[], uint16_t arraySize)
//{
//    RESULT result;
//    AdaptSensorInfo_t sensorInfo;
//    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
//    if(pHalCtx == NULL || pSensorDrvName == NULL)
//    {
//        return RET_NULL_POINTER;
//    }
//
//    result = AdaptGetSensorInfo(pHalCtx->adaptHandle, &sensorInfo);
//    if(result != RET_SUCCESS)
//    {
//        TRACE(HAL_ERROR, "%s: get sensor drv name error in hal!\n", __func__);
//        return result;
//    }
//
//    snprintf(pSensorDrvName, arraySize, "%s", sensorInfo.pSensorDrvName);
//    return RET_SUCCESS;
//}
//
//RESULT HalGetSensorCalibXmlName(HalHandle_t HalHandle, char pSensorCalibXmlName[], uint16_t arraySize)
//{
//    RESULT result;
//    AdaptSensorInfo_t sensorInfo;
//    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
//    if(pHalCtx == NULL || pSensorCalibXmlName == NULL)
//    {
//        return RET_NULL_POINTER;
//    }
//    result = AdaptGetSensorInfo(pHalCtx->adaptHandle, &sensorInfo);
//    if(result != RET_SUCCESS)
//    {
//        TRACE(HAL_ERROR, "%s: get sensor calibration XML name error in hal!\n", __func__);
//        return result;
//    }
//
//    snprintf(pSensorCalibXmlName, arraySize, "%s", sensorInfo.pSensorCalibXmlName);
//    return RET_SUCCESS;
//
//}
//
//RESULT HalGetSensorDefaultMode(HalHandle_t HalHandle, uint32_t *pMode)
//{
//    RESULT result;
//    AdaptSensorInfo_t sensorInfo;
//    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
//    if(pHalCtx == NULL || pMode == NULL)
//    {
//        return RET_NULL_POINTER;
//    }
//    result = AdaptGetSensorInfo(pHalCtx->adaptHandle, &sensorInfo);
//    if(result != RET_SUCCESS)
//    {
//        TRACE(HAL_ERROR, "%s: get sensor default mode error in hal!\n", __func__);
//        return result;
//    }
//
//    *pMode = sensorInfo.sensorDefaultMode;
//    return RET_SUCCESS;
//}
//
//RESULT HalGetSensorCurrMode(HalHandle_t HalHandle, uint32_t *pMode)
//{
//    RESULT result;
//    AdaptSensorInfo_t sensorInfo;
//    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
//    if(pHalCtx == NULL || pMode == NULL)
//    {
//        return RET_NULL_POINTER;
//    }
//    result = AdaptGetSensorInfo(pHalCtx->adaptHandle, &sensorInfo);
//    if(result != RET_SUCCESS)
//    {
//        TRACE(HAL_ERROR, "%s: get sensor current mode error in hal!\n", __func__);
//        return result;
//    }
//
//    *pMode = sensorInfo.sensorCurrMode;
//    return RET_SUCCESS;
//}
//
//RESULT HalGetSensorCurrHdrMode(HalHandle_t HalHandle, uint32_t *pMode)
//{
//    RESULT result;
//    AdaptSensorInfo_t sensorInfo;
//    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
//    if(pHalCtx == NULL || pMode == NULL)
//    {
//        return RET_NULL_POINTER;
//    }
//    result = AdaptGetSensorInfo(pHalCtx->adaptHandle, &sensorInfo);
//    if(result != RET_SUCCESS)
//    {
//        TRACE(HAL_ERROR, "%s: get sensor current hdr mode error in hal!\n", __func__);
//        return result;
//    }
//
//    *pMode = sensorInfo.sensorHdrEnable;
//    return RET_SUCCESS;
//}
//
//
//RESULT HalSetSensorMode(HalHandle_t HalHandle, uint32_t mode)
//{
//    RESULT result;
//    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
//    if(pHalCtx == NULL)
//    {
//        return RET_NULL_POINTER;
//    }
//    result = AdaptSetSensorMode(pHalCtx->adaptHandle, mode);
//    if(result != RET_SUCCESS)
//    {
//        TRACE(HAL_ERROR, "%s: set sensor mode error in hal!\n", __func__);
//        return result;
//    }
//    return RET_SUCCESS;
//}
//
//RESULT HalSetSensorCalibXmlName(HalHandle_t HalHandle, const char* CalibXmlName)
//{
//    RESULT result;
//    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
//    if(pHalCtx == NULL || CalibXmlName == NULL)
//    {
//        return RET_NULL_POINTER;
//    }
//    result = AdaptSetSensorCalibXmlName(pHalCtx->adaptHandle, CalibXmlName);
//    if(result != RET_SUCCESS)
//    {
//        TRACE(HAL_ERROR, "%s: set sensor CalibXmlName error in hal!\n", __func__);
//        return result;
//    }
//    return RET_SUCCESS;
//}
//
//RESULT HaSensorModeLock(HalHandle_t HalHandle)
//{
//    RESULT result;
//    HalContext_t *pHalCtx = (HalContext_t *)HalHandle;
//    if(pHalCtx == NULL)
//    {
//        return RET_NULL_POINTER;
//    }
//    result = AdaptSensorModeLock(pHalCtx->adaptHandle);
//    if(result != RET_SUCCESS)
//    {
//        TRACE(HAL_ERROR, "%s: sensor mode lock error in hal!\n", __func__);
//        return result;
//    }
//    return RET_SUCCESS;
//}

