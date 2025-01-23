/******************************************************************************\
|* Copyright (c) <2024> by VeriSilicon Holdings Co., Ltd. ("VeriSilicon")     *|
|* All Rights Reserved.                                                       *|
|*                                                                            *|
|* The material in this file is confidential and contains trade secrets       *|
|* of VeriSilicon.  This is proprietary information owned or licensed by      *|
|* VeriSilicon.  No part of this work may be disclosed, reproduced, copied,   *|
|* transmitted, or used in any way for any purpose, without the express       *|
|* written permission of VeriSilicon.                                         *|
|*                                                                            *|
\******************************************************************************/

/* VeriSilicon 2020 */

/**
 * @file    mipi.h
 *
 * @brief   Definition of MIPI CSI-2 spec. stuff.
 *
 *****************************************************************************/
#ifndef __MIPI_H__
#define __MIPI_H__

#include <ebase/types.h>

#if defined (__cplusplus)
extern "C" {
#endif


#define MIPI_OUTPUT_ITF_MAX 8

/******************************************************************************/
/**
 * @brief MIPI virtual channels.
 *
 ******************************************************************************/
typedef enum MipiVirtualChannel_e
{
    MIPI_VIRTUAL_CHANNEL_0          = 0x00,     //!< virtual channel 0
    MIPI_VIRTUAL_CHANNEL_1          = 0x01,     //!< virtual channel 1
    MIPI_VIRTUAL_CHANNEL_2          = 0x02,     //!< virtual channel 2
    MIPI_VIRTUAL_CHANNEL_3          = 0x03,     //!< virtual channel 3

    MIPI_VIRTUAL_CHANNEL_MAX
} MipiVirtualChannel_t;

/******************************************************************************/
/**
 * @brief MIPI lane index.
 *
 ******************************************************************************/
typedef enum MipiLaneId_e {
    MIPI_LANE_0       = 0,
    MIPI_LANE_1,
    MIPI_LANE_2,
    MIPI_LANE_3,
    MIPI_LANE_4,
    MIPI_LANE_5,
    MIPI_LANE_6,
    MIPI_LANE_7,
    MIPI_LANE_MAX
}MipiLaneId_t;

/******************************************************************************/
/**
 * @brief MIPI data types.
 *
 ******************************************************************************/
typedef enum MipiDataType_e
{
    MIPI_DATA_TYPE_FSC              = 0x00,     //!< frame start code
    MIPI_DATA_TYPE_FEC              = 0x01,     //!< frame end code
    MIPI_DATA_TYPE_LSC              = 0x02,     //!< line start code
    MIPI_DATA_TYPE_LEC              = 0x03,     //!< line end code

                                                //   0x04 .. 0x07 reserved

    MIPI_DATA_TYPE_GSPC1            = 0x08,     //!< gerneric short packet code 1
    MIPI_DATA_TYPE_GSPC2            = 0x09,     //!< gerneric short packet code 2
    MIPI_DATA_TYPE_GSPC3            = 0x0A,     //!< gerneric short packet code 3
    MIPI_DATA_TYPE_GSPC4            = 0x0B,     //!< gerneric short packet code 4
    MIPI_DATA_TYPE_GSPC5            = 0x0C,     //!< gerneric short packet code 5
    MIPI_DATA_TYPE_GSPC6            = 0x0D,     //!< gerneric short packet code 6
    MIPI_DATA_TYPE_GSPC7            = 0x0E,     //!< gerneric short packet code 7
    MIPI_DATA_TYPE_GSPC8            = 0x0F,     //!< gerneric short packet code 8

    MIPI_DATA_TYPE_NULL             = 0x10,     //!< null
    MIPI_DATA_TYPE_BLANKING         = 0x11,     //!< blanking data
    MIPI_DATA_TYPE_EMBEDDED         = 0x12,     //!< embedded 8-bit non image data

                                                //   0x13 .. 0x17 reserved

    MIPI_DATA_TYPE_YUV420_8         = 0x18,     //!< YUV 420 8-Bit
    MIPI_DATA_TYPE_YUV420_10        = 0x19,     //!< YUV 420 10-Bit
    MIPI_DATA_TYPE_LEGACY_YUV420_8  = 0x1A,     //!< YUV 420 8-Bit
                                                //   0x1B reserved
    MIPI_DATA_TYPE_YUV420_8_CSPS    = 0x1C,     //!< YUV 420 8-Bit (chroma shifted pixel sampling)
    MIPI_DATA_TYPE_YUV420_10_CSPS   = 0x1D,     //!< YUV 420 10-Bit (chroma shifted pixel sampling)
    MIPI_DATA_TYPE_YUV422_8         = 0x1E,     //!< YUV 422 8-Bit
    MIPI_DATA_TYPE_YUV422_10        = 0x1F,     //!< YUV 422 10-Bit

    MIPI_DATA_TYPE_RGB444           = 0x20,     //!< RGB444
    MIPI_DATA_TYPE_RGB555           = 0x21,     //!< RGB555
    MIPI_DATA_TYPE_RGB565           = 0x22,     //!< RGB565
    MIPI_DATA_TYPE_RGB666           = 0x23,     //!< RGB666
    MIPI_DATA_TYPE_RGB888           = 0x24,     //!< RGB888

                                                //   0x25 .. 0x27 reserved

    MIPI_DATA_TYPE_RAW_8            = 0x28,     //!< RAW8
    MIPI_DATA_TYPE_RAW_10           = 0x29,     //!< RAW10
    MIPI_DATA_TYPE_RAW_12           = 0x2A,     //!< RAW12
    MIPI_DATA_TYPE_RAW_14           = 0x2B,     //!< RAW14
    MIPI_DATA_TYPE_RAW_16           = 0x2C,     //!< RAW16
    MIPI_DATA_TYPE_RAW_20           = 0x2D,     //!< RAW20
    MIPI_DATA_TYPE_RAW_24           = 0x2E,     //!< RAW24

                                                //  0x2F reserved

    MIPI_DATA_TYPE_USER_1           = 0x30,     //!< user defined 1
    MIPI_DATA_TYPE_USER_2           = 0x31,     //!< user defined 2
    MIPI_DATA_TYPE_USER_3           = 0x32,     //!< user defined 3
    MIPI_DATA_TYPE_USER_4           = 0x33,     //!< user defined 4
    MIPI_DATA_TYPE_USER_5           = 0x34,     //!< user defined 5
    MIPI_DATA_TYPE_USER_6           = 0x35,     //!< user defined 6
    MIPI_DATA_TYPE_USER_7           = 0x36,     //!< user defined 7
    MIPI_DATA_TYPE_USER_8           = 0x37,     //!< user defined 8

    MIPI_DATA_TYPE_MAX
} MipiDataType_t;


/******************************************************************************/
/**
 * @brief MIPI compression schemes.
 *
 ******************************************************************************/
typedef enum MipiDataCompressionScheme_e
{
    MIPI_DATA_COMPRESSION_SCHEME_NONE    = 0,   //!< NONE
    MIPI_DATA_COMPRESSION_SCHEME_12_8_12 = 1,   //!< 12_8_12
    MIPI_DATA_COMPRESSION_SCHEME_12_7_12 = 2,   //!< 12_7_12
    MIPI_DATA_COMPRESSION_SCHEME_12_6_12 = 3,   //!< 12_6_12
    MIPI_DATA_COMPRESSION_SCHEME_10_8_10 = 4,   //!< 10_8_10
    MIPI_DATA_COMPRESSION_SCHEME_10_7_10 = 5,   //!< 10_7_10
    MIPI_DATA_COMPRESSION_SCHEME_10_6_10 = 6,   //!< 10_6_10

    MIPI_DATA_COMPRESSION_SCHEME_MAX
} MipiDataCompressionScheme_t;


/******************************************************************************/
/**
 * @brief MIPI predictor blocks.
 *
 ******************************************************************************/
typedef enum MipiPredictorBlock_e
{
    MIPI_PREDICTOR_BLOCK_INVALID = 0,   //!< invalid

    MIPI_PREDICTOR_BLOCK_1       = 1,   //!< Predictor1 (simple algorithm)
    MIPI_PREDICTOR_BLOCK_2       = 2,   //!< Predictor2 (more complex algorithm)

    MIPI_PREDICTOR_BLOCK_MAX
} MipiPredictorBlock_t;

/******************************************************************************/
/**
 * @brief MIPI output interface bit mode.
 *
 ******************************************************************************/
typedef enum MipiOutItfBitMode_e
{
    MIPI_OUT_ITF_16Bit_MODE          = 0,     //!< IPI mode 16bit
    MIPI_OUT_ITF_48Bit_MODE          = 1,     //!< IPI mode 48bit
    MIPI_OUT_ITF_MODE_MAX
} MipiOutItfBitMode_t;

typedef struct MipiOutItfHorizonTime_s{
    uint32_t hsa;   /* horizontal synchronism activate (pixel cycle) */
    uint32_t hbp;   /* horizontal back porch (pixel cycle) */
    uint32_t hfp;   /* horizontal front porch (pixel cycle) */
    uint32_t hline;   /* overall time for each line (pixel cycle) */
} MipiOutItfHorizonTime_t;

typedef struct MipiOutItfVerticalTime_s{
    uint32_t vsa;   /* vertical synchronism activate (hline) */
    uint32_t vbp;   /* vertical back porch (hline) */
    uint32_t vfp;   /* vertical front porch (hline) */
    uint32_t vactivateLines;   /* vertical resolution of video */
} MipiOutItfVerticalTime_t;

typedef struct MipiImageRect_s{
    uint32_t left;
    uint32_t top;
    uint32_t width;
    uint32_t height;
} MipiImageRect_t;

/******************************************************************************/
/**
 * @brief mipi output interface configuration structure.
 *
 ******************************************************************************/
typedef struct MipiOutItfConfig_s{

    bool enable;
    bool embeddedEn;
    MipiVirtualChannel_t     vcid;
    MipiOutItfBitMode_t      bitMode;
    MipiOutItfHorizonTime_t  hTime;
    MipiOutItfVerticalTime_t vTime;
} MipiOutItfConfig_t;


#if defined (__cplusplus)
}
#endif

#endif /* __MIPI_H__*/
