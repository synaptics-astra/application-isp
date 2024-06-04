/****************************************************************************
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2022 Vivante Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************/


/**
 * @file hal_common.h
 *
 * @brief
 *
 *
 * @note
 *
 *****************************************************************************/

#ifndef __HAL_COMMON_H__
#define __HAL_COMMON_H__

#include <ebase/trace.h>
#include <ebase/dct_assert.h>
#include <oslayer/oslayer.h>
#include <common/buf_defs.h>
#include <common/mipi.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* MACRO for MI sub-system. */
#define HAL_MSS_REG_SIZE        0x10000
#define HAL_MSS_REG_MIS_ADDR    0x16f0
#define HAL_MSS_REG_ICR_ADDR    0x16f4

/* Set this macro to 1 enable register dump. */
#define ENABLE_REGISTER_DUMP 0
/******************************************************************************
 * HAL device base addresses
 *****************************************************************************/
// HalRegs:
#define HAL_BASEADDR_MARVIN    0x00000000 //!< Base address of MARVIN module.
#define HAL_BASEADDR_MARVIN_2  0x00010000 //!< Base address of MARVIN module of 2nd channel.
#define HAL_BASEADDR_MIPI_1    0x00000000 //!< Base address of MIPI module.
#define HAL_BASEADDR_MIPI_2    0x00010000 //!< Base address of MIPI module of 2nd channel.
#define HAL_BASEADDR_VDU       0x00008000 //!< Base address of VDU module.
#define HAL_BASEADDR_MARVIN_DEC 0x00600000 //!< Base address of DEC module.

/*****************************************************************************/
/**
 * @brief   hal cmodel buffer tranfer type
 *****************************************************************************/
#define HAL_CMODEL_BUF_TRANS_DMA        0x0001
#define HAL_CMODEL_BUF_TRANS_MP         0x0002
#define HAL_CMODEL_BUF_TRANS_SP         0x0004
#define HAL_CMODEL_BUF_TRANS_SP2_BP     0x0008
#define HAL_CMODEL_BUF_TRANS_PP         0x00010

#define HAL_CMODEL_BUF_TRANS_HDR_RET    0x00020
#define HAL_CMODEL_BUF_TRANS_JDP        0x00040
#define HAL_CMODEL_BUF_TRANS_MP_RAW     0x00080

#define HAL_CMODEL_BUF_TRANS_MASK       0x000FF

/** Interrupt handling mode switch.
  * If set to zero (0), the interrupt facilities built into the kernel driver
  * will not be used. Instead, the user-space application will poll for
  * interrupts by reading the masked interrupt status register repeatedly.
  * If set to one (1), the kernel driver will handle the interrupt requests by
  * the hardware. This is much more efficient, but requires the IRQ logic in the
  * hardware to work correctly. */
#define FPGA_USE_KERNEL_IRQ_HANDLING 0

/*****************************************************************************/
/**
 * @brief   hal device type enum
 *****************************************************************************/

typedef enum HalDevType_e{
    HAL_DEV_ISP = 0,
    HAL_DEV_MEM,
    HAL_DEV_I2C,
    HAL_DEV_MIPI,
    HAL_DEV_VIDEO_IN,
    HAL_DEV_MSS,
}HalDevType_t;

/*****************************************************************************/
/**
 * @brief   irq context
 *****************************************************************************/
typedef struct IrqContext_s
{
    uint32_t misRegAddress;
    uint32_t mis;
    osEvent      *pIsr_irq_event;        //notify observer interrupt event
    osQueue      *pIsr_mis_queue;        // mis value queue
} IrqContext_t;

/*****************************************************************************
 * @brief Generic command type.
 *
 *****************************************************************************/
typedef struct HalIrqNotifierCmd_s
{
    uint8_t    cmdId;
    void                *pCmdCtx;
} HalIrqNotifierCmd_t;

/*****************************************************************************/
/**
 * @brief   hal interrupt type enum
 *****************************************************************************/
typedef enum HalInterruptType_e{
    HAL_INT_MI_DMA_READY    =0x0000,
    HAL_INT_MI_FRAME_END_MP,
    HAL_INT_MI_FRAME_END_SP,
    HAL_INT_MI_FRAME_END_MP_RAW,
	HAL_INT_MI_FRAME_END_SP2_BP,
    HAL_INT_MI_RAW_FRAME_END_SP2_BP,
    HAL_INT_MI2_FRAME_END_PPW,
    HAL_INT_ISP_OFF,
    HAL_INT_ISP_HistM,
    HAL_INT_ISP_ExpM,
    HAL_INT_ISP_WbM,
    HAL_INT_ISP_AfmM,
    HAL_INT_ISP_VsmM,
    HAL_INT_MI_FRAME_END_JDP,
    HAL_INT_ISP_FRAME_DOWN,
    HAL_INTMax,
}HalInterruptType_t;

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
typedef enum HalIsrSrc_e
{
    eHalIsrSrcIspIrq         = 0x0000,
    eHalIsrSrcJpeStatusIrq,
    eHalIsrSrcJpeErrorIrq,
    eHalIsrSrcCamIcMiIrq,
    eHalIsrSrcCamIcMi1Irq,
    eHalIsrSrcCamIcMi2Irq,
    eHalIsrSrcCamIcMi3Irq,
    eHalIsrSrcMcmRdmaIrq,
    eHalIsrSrcMcmWrRaw0Irq,
    eHalIsrSrcMcmWrRaw1Irq,
    eHalIsrSrcMcmG2WrRaw0Irq,
    eHalIsrSrcMcmG2WrRaw1Irq,
    eHalIsrSrcCamIcMipiIrq,
    eHalIsrSrcCamIcIspStitchingIrq,
    eHalIsrSrcCamIcHdrIrq,
    eHalIsrSrcCamIcHdr1Irq,
    eHalIsrSrcCamIcFEIrq,
    eHalIsrSrcCamIcJdpIrq,
    eHalIsrSrcCamIcFusaEcc1Irq,
    eHalIsrSrcCamIcFusaEcc2Irq,
    eHalIsrSrcCamIcFusaEcc3Irq,
    eHalIsrSrcCamIcFusaEcc4Irq,
    eHalIsrSrcCamIcFusaEcc5Irq,
    eHalIsrSrcCamIcFusaEcc6Irq,
    eHalIsrSrcCamIcFusaDupIrq,
    eHalIsrSrcCamIcFusaParityIrq,
    eHalIsrSrcCamIcFusaLv1Irq,
    eHalIsrMax
} HalIsrSrc_t;


typedef enum HalChId_s
{
    HAL_CH_ID0 = 0, //!<isp channel id 0.
    HAL_CH_ID1 = 1, //!<isp channel id 1.
    HAL_CH_ID2 = 2, //!<isp channel id 2.
    HAL_CH_ID3 = 3, //!<isp channel id 3.
    HAL_CH_NUM      //!<isp channel number.
}HalChId_e;

/*****************************************************************************/
/**
 * @brief   hal cmodel output path enum
 *****************************************************************************/
typedef enum  HalCmOutputPath_e {
    eHalCmodelOutputMainPath = 0,
    eHalCmodelOutputSelfPath ,
    eHalCmodelOutputBpSelfPath2,        //in miv1 this is bapyer patern path.miv2 is self path2.
    eHalCmodelOutputMainPathRAW,
    eHalCmodelOutputPostProcess,
    eHalCmodelOutputJdp,
    eHalCmodelOutputPathMax
}HalCmOutputPath_t;

/*****************************************************************************/
/**
 * @brief   hal video in mcm path irq enum
 *****************************************************************************/
typedef enum HalVideoInIsrSrc_e
{
    eHalVideoInIsrSrcPath0Irq = 0x0000,
    eHalVideoInIsrSrcPath1Irq,
    eHalVideoInIsrSrcPath2Irq,
    eHalVideoInIsrSrcPath3Irq,
    eHalVideoInIsrSrcPath4Irq,
    eHalVideoInIsrSrcPath5Irq,
    eHalVideoInIsrSrcPath6Irq,
    eHalVideoInIsrSrcPath7Irq,
    eHalVideoInIsrSrcPath8Irq,
    eHalVideoInIsrSrcPath9Irq,
    eHalVideoInIsrSrcPath10Irq,
    eHalVideoInIsrSrcPath11Irq,
    eHalVideoInIsrSrcPath12Irq,
    eHalVideoInIsrSrcPath13Irq,
    eHalVideoInIsrSrcPath14Irq,
    eHalVideoInIsrSrcPath15Irq,
    eHalVideoInIsrSrcPathMax,
} HalVideoInIsrSrc_t;
/******************************************************************************
 * local type definitions
 *****************************************************************************/
#if defined ( HAL_ALTERA )

/* IRQ handle type. */
typedef struct _fpga_irq_handle {
#if FPGA_USE_KERNEL_IRQ_HANDLING
        int __dummy;
#else
        uint32_t mis_addr;
        uint32_t cis_addr;
        uint32_t timeout;
        volatile int cancel;
        osMutex poll_mutex;
#endif
} fpga_irq_handle_t;


#endif

/*****************************************************************************/
/**
 * @brief   hal irq context
 *****************************************************************************/
struct HalIrqCtx_s                                  // note: a forward declaration was given in this file before!
{
    HalHandle_t         HalHandle;                  /**< hal handle this context belongs to; must be set by callee prior connection of irq! */
    uint32_t            misRegAddress;              /**< address of the masked interrupt status register (MIS); must be set by callee prior connection of irq! */
    uint32_t            icrRegAddress;              /**< address of the interrupt clear register (ICR); must be set by callee prior connection of irq! */

    osInterrupt         OsIrq;                      /**< os layer abstraction for the interrupt */
    uint32_t            misValue;                   /**< value of the MIS-Register */
    uint32_t            statisticMisValue;          /**< value of the statistics MIS-Register */

#if defined ( HAL_ALTERA )
    fpga_irq_handle_t   AlteraIrqHandle;            /**< handle for multiple interrupt handler */
#endif

    HalIsrSrc_t   irq_src;
    void*         hBinder;                      /**< handle for binder context*/

};

/*****************************************************************************/
/**
 * @brief   hal irq context
 *****************************************************************************/
typedef struct HalIrqCtx_s HalIrqCtx_t; // implicit forward declaration of struct HalIrqCtx_s

/*****************************************************************************/
/**
 * @brief   different ways of how mapping of data from the memory into local memory is done
 *****************************************************************************/
typedef enum HalMapMemType_s
{
    HAL_MAPMEM_READWRITE = 0,   //!< Maps memory for read/write access.
    HAL_MAPMEM_READONLY,        //!< Maps memory for read access only, system action on write access is undefined
    HAL_MAPMEM_WRITEONLY        //!< Maps memory for write access only, system action on read access is undefined
} HalMapMemType_t;

/*****************************************************************************/
/**
 * @brief   hal memory mapping configuration struct
 *****************************************************************************/
typedef struct HalMemMap_s
{
    uint32_t        phyAddr;       //!< Hardware memory address.
    uint32_t        size;          //!< Size of mapped buffer.
    HalMapMemType_t mappingType;   //!< How the buffer is mapped.
    void            *pBufbase;     //!< Base of allocated buffer.
} HalMemMap_t;

/*****************************************************************************/
/**
 * @brief   hal camera configuration struct
 *****************************************************************************/
typedef struct HalCamConfig_s
{
    bool_t configured;      //!< Mark whether this config was set.
    bool_t powerLowActive;    //!< Power on is low-active.
    bool_t resetLowActive;    //!< Reset is low-active.
    //bool_t negedge;         //!< Capture data on negedge.
} HalCamConfig_t;

/*****************************************************************************/
/**
 * @brief   hal camera physical configuration struct
 *****************************************************************************/
typedef struct HalCamPhyConfig_s
{
    bool_t configured;      //!< Mark whether this config was set.
    bool_t powerLowActive;    //!< Power on is low-active.
    bool_t resetLowActive;    //!< Reset is low-active.
} HalCamPhyConfig_t;

/******************************************************************************
 * HalCmodelBufConfig_t
 *****************************************************************************/
typedef struct HalCmodelBufConfig_s{
    //DMA input buffer configurations
    PicBufType_t cmDmaBufType;
    PicBufLayout_t cmDmaBufLayout;
    uint32_t cmDmaBaseY;
    uint32_t cmDmaWidthY;
    uint32_t cmDmaHeightY;
    uint32_t cmDmaBaseCb;
    uint32_t cmDmaBaseCr;
    uint32_t cmDmaIntEnabled;
    uint32_t cmDmaLineLen;

    //tobe expand later for y_burstlength, c_burstlength

    //Main Path buffer configurations
    PicBufType_t cmOutputBufType[eHalCmodelOutputPathMax];
    PicBufLayout_t cmOutputBufLayout[eHalCmodelOutputPathMax];
    uint32_t cmOutputBaseY[eHalCmodelOutputPathMax];
    uint32_t cmOutputSizeY[eHalCmodelOutputPathMax];
    uint32_t cmOutputOffsY[eHalCmodelOutputPathMax];
    uint32_t cmOutputPicWidthPixelY[eHalCmodelOutputPathMax];
    uint32_t cmOutputPicHeightPixelY[eHalCmodelOutputPathMax];
    uint32_t cmOutputPicLineLength[eHalCmodelOutputPathMax];

    uint32_t cmOutputBaseCb[eHalCmodelOutputPathMax];
    uint32_t cmOutputSizeCb[eHalCmodelOutputPathMax];
    uint32_t cmOutputOffsCb[eHalCmodelOutputPathMax];
    uint32_t cmOutputPicWidthPixelCb[eHalCmodelOutputPathMax];
    uint32_t cmOutputPicHeightPixelCb[eHalCmodelOutputPathMax];

    uint32_t cmOutputBaseCr[eHalCmodelOutputPathMax];
    uint32_t cmOutputSizeCr[eHalCmodelOutputPathMax];
    uint32_t cmOutputOffsCr[eHalCmodelOutputPathMax];
    uint32_t cmOutputPicWidthPixelCr[eHalCmodelOutputPathMax];
    uint32_t cmOutputPicHeightPixelCr[eHalCmodelOutputPathMax];
    uint32_t cmOutputIntEnabled[eHalCmodelOutputPathMax];
    uint8_t cmOutputYuvBitWidth[eHalCmodelOutputPathMax];      //Yuv output bit width
    uint8_t cmOutputAlignMode[eHalCmodelOutputPathMax];        // align mode

    //shd register
    uint32_t cmOutputBaseYShd[eHalCmodelOutputPathMax];
    uint32_t cmOutputSizeYShd[eHalCmodelOutputPathMax];
    uint32_t cmOutputOffsYShd[eHalCmodelOutputPathMax];
    uint32_t cmOutputPicWidthPixelYShd[eHalCmodelOutputPathMax];
    uint32_t cmOutputPicHeightPixelYShd[eHalCmodelOutputPathMax];
    uint32_t cmOutputPicLineLengthShd[eHalCmodelOutputPathMax];

    uint32_t cmOutputBaseCbShd[eHalCmodelOutputPathMax];
    uint32_t cmOutputSizeCbShd[eHalCmodelOutputPathMax];
    uint32_t cmOutputOffsCbShd[eHalCmodelOutputPathMax];
    uint32_t cmOutputPicWidthPixelCbShd[eHalCmodelOutputPathMax];
    uint32_t cmOutputPicHeightPixelCbShd[eHalCmodelOutputPathMax];

    uint32_t cmOutputBaseCrShd[eHalCmodelOutputPathMax];
    uint32_t cmOutputSizeCrShd[eHalCmodelOutputPathMax];
    uint32_t cmOutputOffsCrShd[eHalCmodelOutputPathMax];
    uint32_t cmOutputPicWidthPixelCrShd[eHalCmodelOutputPathMax];
    uint32_t cmOutputPicHeightPixelCrShd[eHalCmodelOutputPathMax];

#ifdef ISP_MI_BP
//bayper Pattern buffer configurations
    PicBufType_t cmBpBufType;
    PicBufLayout_t cmBpBufLayout;
    uint32_t cmBpBaseR;
    uint32_t cmBpSizeR;
    uint32_t cmBpOffsR;

    uint32_t cmBpBaseGr;
    uint32_t cmBpSizeGr;
    uint32_t cmBpOffsGr;

    uint32_t cmBpBaseGb;
    uint32_t cmBpSizeGb;
    uint32_t cmBpOffsGb;


    uint32_t cmBpBaseB;
    uint32_t cmBpSizeB;
    uint32_t cmBpOffsB;

    uint32_t cmBpPicWrOffsCntInit;
    uint32_t cmBpPicWrIrqOffsInit;
    uint32_t cmBpPicWrSizeInit;

    uint32_t cmBpPicWrLlength;
    uint32_t cmBpPicWrWidth;
    uint32_t cmBpPicWrHeight;
    uint32_t cmBpPicSize;
    uint32_t cmBpIntEnabled;
//Shd register
    uint32_t cmBpBaseRShd;
    uint32_t cmBpSizeRShd;
    uint32_t cmBpOffsRShd;

    uint32_t cmBpBaseGrShd;
    uint32_t cmBpSizeGrShd;
    uint32_t cmBpOffsGrShd;

    uint32_t cmBpBaseGbShd;
    uint32_t cmBpSizeGbShd;
    uint32_t cmBpOffsGbShd;

    uint32_t cmBpBaseBShd;
    uint32_t cmBpSizeBShd;
    uint32_t cmBpOffsBShd;

#endif

    int frame_cnt;
}HalCmodelBufConfig_t;

/*****************************************************************************/
/**
 * @brief   handle to hal instance
 *****************************************************************************/
typedef void* HalHandle_t;
typedef void* HalIspHandle_t;
typedef void* HalMemHandle_t;
typedef void* HalI2cHandle_t;
typedef void* HalMssHandle_t;
typedef void* ExtMemHandle_t;

/*****************************************************************************/
/**
 * @brief   hal ISP configuration struct
 *****************************************************************************/
typedef struct HalIspContext_s {
    uint32_t            refCount;               //!< internal ref count
    uint8_t             devId;
    int32_t             fd;
    osMutex             refMutex;               //!< common short term mutex; e.g. for read-modify-write accesses

    uint32_t*           regVirtualBase;
    unsigned long       regPhySize;
    HalCamConfig_t      cam1Config;             //!< configuration for CAM1; set at runtime
    HalCamPhyConfig_t   camPhy1Config;          //!< configuration for CAMPHY1; set at runtime
#if ENABLE_REGISTER_DUMP
    FILE*               regDumpFile;
#endif
    char*               regDynDumpName;         //!< register dump name.
    FILE*               regDynDumpFp;           //!< dynamic dump handle
    void               *pIrqNotifier;
    volatile int        irqCancelAll;
    osDpcFunc           halIsrFunTable[eHalIsrMax];  //!< hal Isr source and function table
    HalCmodelBufConfig_t halCmodelbufCfg;
} HalIspContext_t;

/*****************************************************************************/
/**
 * @brief   hal memory configuration struct
 *****************************************************************************/
typedef struct HalMemContext_s {
    uint32_t            refCount;               //!< internal ref count
    uint8_t             devId;
    int32_t             fd;
    osMutex             refMutex;               //!< common short term mutex; e.g. for read-modify-write accesses

    uint8_t*            externMemVirtualBase;
    unsigned long       reservedMemBase;
    unsigned long       reservedMemSize;
    uint8_t*            resMemVirtBaseForUsr;
    unsigned long       resMemBaseForUsr;
    unsigned long       resMemSizeForUsr;
    HalCmodelBufConfig_t halCmodelbufCfg;

    ExtMemHandle_t      extMemHandle;
} HalMemContext_t;

/*****************************************************************************/
/**
 * @brief   hal I2C configuration struct
 *****************************************************************************/
typedef struct HalI2cContext_s {
    uint32_t            refCount;               //!< internal ref count
    uint8_t             devId;
    int32_t             fd;
    osMutex             refMutex;               //!< common short term mutex; e.g. for read-modify-write accesses

    uint8_t             slaveAddr;
    uint8_t             regWidth;
    uint8_t             dataWidth;
} HalI2cContext_t;


/*****************************************************************************/
/**
 * @brief   hal vindeo in configuration struct
 *****************************************************************************/
typedef struct HalVideoInContext_s {
    uint32_t            refCount;               //!< internal ref count
    int32_t             fd;
    osMutex             refMutex;               //!< common short term mutex; e.g. for read-modify-write accesses

    uint32_t*           regVirtualBase;
    unsigned long       regPhySize;
    FILE*               regDynDumpFp;           //!< dynamic dump handle

    osDpcFunc           halIsrFunTable[eHalVideoInIsrSrcPathMax];  //!< hal Isr source and function table
    void *pIrqNotifier;
} HalVideoInContext_t;


/*****************************************************************************/
/**
 * @brief   hal MI sub-system configuration struct
 *****************************************************************************/
typedef struct HalMssContext_s {
    uint32_t            refCount;               //!< internal ref count
    int32_t             fd;
    osMutex             refMutex;               //!< common short term mutex; e.g. for read-modify-write accesses

    uint32_t*           regVirtualBase;
    unsigned long       regPhySize;

    osDpcFunc           halIsrFunction;         //!< hal Isr source and function
} HalMssContext_t;


/******************************************************************************
 * HalDynRegDumpOpen()
 *****************************************************************************/
RESULT HalDynRegDumpOpen( HalHandle_t HalHandle, char* regDynDumpName );

/******************************************************************************
 * HalDynRegDumpClose()
 *****************************************************************************/
RESULT HalDynRegDumpClose( HalHandle_t HalHandle );

#ifdef __cplusplus
}
#endif
#endif /* __HAL_COMMON_H__ */
