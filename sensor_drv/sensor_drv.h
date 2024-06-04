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


#ifndef __SENSOR_DRV_H__
#define __SENSOR_DRV_H__

#include "isi_iss.h"


#ifdef __cplusplus
extern "C"
{
#endif

typedef struct SensorDrvConfig_s
{
    const char *pSensorName;
    IsiCamDrvConfig_t *pSensorConfig;
} SensorDrvConfig_t;

/*****************************************************************************/
/**
 *          SensorDrvConfigMapping
 *
 * @brief   sensor config mapping.
 *
 * @param   pSensorName      Pointer to the sensor name
 * @param   pSensorConfig    Pointer to the isi sensor driver config
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
RESULT SensorDrvConfigMapping
(
    const char *pSensorName,
    IsiCamDrvConfig_t **pSensorConfig
);

#ifdef __cplusplus
}
#endif

#endif    // __SENSOR_DRV_H__

