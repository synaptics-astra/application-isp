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


#ifdef DOLPHIN
extern IsiCamDrvConfig_t IMX258_IsiCamDrvConfig;
extern IsiCamDrvConfig_t IMX415_IsiCamDrvConfig;
extern IsiCamDrvConfig_t OV5647_IsiCamDrvConfig;
extern IsiCamDrvConfig_t IMX477_IsiCamDrvConfig;
extern IsiCamDrvConfig_t DummySensor_IsiCamDrvConfig;
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
#ifdef DOLPHIN
        {"imx258" , &IMX258_IsiCamDrvConfig },
        {"imx415" , &IMX415_IsiCamDrvConfig },
        {"ov5647" , &OV5647_IsiCamDrvConfig },
        {"imx477" , &IMX477_IsiCamDrvConfig },
        {"dummysensor" , &DummySensor_IsiCamDrvConfig },
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
