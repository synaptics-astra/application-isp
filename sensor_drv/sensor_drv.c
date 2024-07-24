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
extern IsiCamDrvConfig_t IMX415_IsiCamDrvConfig;
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
        {"imx327" ,  &IMX327_IsiCamDrvConfig },
        {"ov13b10" , &OV13b10_IsiCamDrvConfig },
        {"ov50a40" , &OV50a40_IsiCamDrvConfig },
        {"gc5603" , &GC5603_IsiCamDrvConfig },
        {"sc1330t" , &SC1330T_IsiCamDrvConfig },
        {"os04a10" , &OS04a10_IsiCamDrvConfig },
#ifdef DOLPHIN
        {"imx258" , &IMX258_IsiCamDrvConfig },
        {"imx415" , &IMX415_IsiCamDrvConfig },
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

