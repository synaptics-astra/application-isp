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
 * @file hal_api.h
 *
 * @brief   Hardware Abstraction Layer
 *
 *          Encapsulates and abstracts services from different hardware platforms.
 *
 *****************************************************************************/

#ifndef __HAL_API_H__
#define __HAL_API_H__

#include <ebase/types.h>
#include <ebase/dct_assert.h>

#include <common/return_codes.h>
#include <oslayer/oslayer.h>

/******************************************************************************
 * inline implementations of API
 *****************************************************************************/
#include "hal_common.h"
#if defined( HAL_ALTERA )
#include "fpga/altera_fpga.h"
#elif defined (HAL_CMODEL)
#include <cmodel_wrapper/cmodel_wrapper.h>
#endif

/**
 * @defgroup HAL_API Hardware Abstraction Layer interface
 * @{
 * @brief  Encapsulates and abstracts services from different hardware platforms
 *         in a hardware platform independent way.
 *****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************/
/**
 * @brief   open the low level driver
 * @param   ispId    isp device id
 * @return  handle of driver; NULL on failure
 *
 * @note    Create isp handle and set internal reference count to 1.
 *****************************************************************************/
HalIspHandle_t HalIspOpen(uint8_t ispId);

/*****************************************************************************/
/**
 * @brief   close the low level driver
 * @param   HalIspHandle   Handle to HAL session as returned by @ref HalIspOpen.
 * @return  Result of operation.
 *
 * @note    When reference count is 1, close the isp device file, and when count
 *          is greater than 1, return a status error code.
 *****************************************************************************/
RESULT HalIspClose(HalIspHandle_t HalIspHandle);

/*****************************************************************************/
/**
 * @brief   open the low level driver
 * @return  handle of driver; NULL on failure
 *
 * @note    Only supports a single memory device instance.
 *****************************************************************************/
HalMemHandle_t HalMemOpen(void);

/*****************************************************************************/
/**
 * @brief   close the low level driver
 * @param   HalMemHandle   Handle to HAL session as returned by @ref HalMemOpen.
 * @return  Result of operation.
 *
 * @note    Decrements internal ref count and closes low level driver if ref count reached zero.
 *****************************************************************************/
RESULT HalMemClose(HalMemHandle_t HalMemHandle);

/*****************************************************************************/
/**
 * @brief   allocates the given amount of hardware memory
 * @param   HalMemHandle    Handle to HAL session as returned by @ref HalMemOpen.
 * @param   size            Amount of memory to allocate.
 * @param   pVirtAddr       virtual address got through mmap
 * @return  mem_address     Memory block start address in hardware memory; 0 on failure.
 *
 * @note    Chunks of n*4K byte with addresses aligned to 4K are used internally.
 *          The allocator behind the scenes may be very simple, but still will recombine
 *          free'd adjacent blocks. It may perform reasonably well only for at most a
 *          few dozen allocs active at any time. Allocating/freeing video/audio buffers in
 *          realtime at framerate should nevertheless be posssible without noticeable
 *          performance penalties then.
 *****************************************************************************/
uint32_t HalAllocMemory(HalMemHandle_t HalMemHandle, uint32_t size, void **pVirtAddr);

/*****************************************************************************/
/**
 * @brief   frees the given block of hardware memory
 * @param   HalMemHandle    Handle to HAL session as returned by @ref HalMemOpen.
 * @param   phyAddr         physical address
 * @return  Result of operation.
 *
 * @note    Chunks of n*4K byte with addresses aligned to 4K are used internally.
 *****************************************************************************/
RESULT HalFreeMemory(HalMemHandle_t HalMemHandle, uint32_t phyAddr);

/*****************************************************************************/
/**
 * @brief   reads a number of data from the memory to a buffer starting a the given address
 * @param   HalMemHandle    Handle to HAL session as returned by @ref HalMemOpen.
 * @param   phyAddr         Source start address in hardware memory.
 * @param   pData           Pointer to local memory holding the data being read.
 * @param   size            Amount of data to read.
 * @return  Result of operation.
 *
 * @note    Certain implementation dependent limitations regarding alignment of
 *          both addresses and transfer size exist!
 *****************************************************************************/
RESULT HalReadMemory(HalMemHandle_t HalMemHandle, uint32_t phyAddr, void *pData, uint32_t size);

/*****************************************************************************/
/**
 * @brief   writes a number of data from a buffer to the memory starting a the given address
 * @param   HalMemHandle    Handle to HAL session as returned by @ref HalMemOpen.
 * @param   phyAddr         Target start address in hardware memory.
 * @param   pData           Pointer to local memory holding the data to be written.
 *                          Undefined on failure.
 * @param   size            Amount of data to write.
 * @return  Result of operation.
 *
 * @note    Certain implementation dependent limitations regarding alignment of
 *          both addresses and transfer size exist!
 *****************************************************************************/
RESULT HalWriteMemory(HalMemHandle_t HalMemHandle, uint32_t phyAddr, void *pData, uint32_t size);

/*****************************************************************************/
/**
 * @brief   maps a number of data from the memory into local memory starting at the given address
 * @param   HalMemHandle    Handle to HAL session as returned by @ref HalMemOpen.
 * @param   phyAddr         Source start address in hardware memory.
 * @param   size            Amount of data to map.
 * @param   mappingType     The way the mapping is performed.
 * @param   pVirtAddr       Reference to pointer to the mapped local memory.
 * @return  Result of operation.
 *
 * @note    Certain implementation dependent limitations regarding alignment of
 *          both addresses and transfer size exist!
 *****************************************************************************/
RESULT HalMapMemory(HalMemHandle_t HalMemHandle, uint32_t phyAddr, uint32_t size, HalMapMemType_t mappingType, void **pVirtAddr);


/*****************************************************************************/
/**
 * @brief   unmaps previously mapped memory from local memory
 * @param   HalMemHandle    Handle to HAL session as returned by @ref HalMemOpen.
 * @param   pMappedBuf      Pointer to local memory to unmap as returned by
 *                          @ref HalMapMemory().
 * @return  Result of operation.
 *
 * @note    Certain implementation dependent limitations regarding alignment of
 *          both addresses and transfer size exist!
 *****************************************************************************/
RESULT HalUnMapMemory(HalMemHandle_t HalMemHandle, uint32_t size, void *pVirtAddr);

/*****************************************************************************/
/**
 * @brief   Make the data in memory and device consistent.
 * @param   HalMemHandle    Handle to HAL session as returned by @ref HalMemOpen.
 * @param   phyAddr         Source start address in hardware memory.
 * @param   size            Data size.
 * @return  Result of operation.
 *
 * @note    Called before DMA moves the data from DDR to device, so that device
 *          can see the latest data.
 *****************************************************************************/
RESULT HalMemCacheFlush(HalMemHandle_t HalMemHandle, uint32_t phyAddr, uint32_t size);

/*****************************************************************************/
/**
 * @brief   Make the data in memory and CPU consistent.
 * @param   HalMemHandle    Handle to HAL session as returned by @ref HalMemOpen.
 * @param   phyAddr         Source start address in hardware memory.
 * @param   size            Data size.
 * @return  Result of operation.
 *
 * @note    Called before CPU accesses DDR, so that CPU can see the latest data.
 *****************************************************************************/
RESULT HalMemCacheInvalid(HalMemHandle_t HalMemHandle, uint32_t phyAddr, uint32_t size);

/*****************************************************************************/
/**
 * @brief   open the low level driver
 * @param   busId       i2c bus id
 * @param   slaveAddr   slave address
 * @param   regWidth    register width
 * @param   dataWidth   data width
 * 
 * @return  handle of driver; NULL on failure
 *
 * @note    Create i2c handle and set internal reference count to 1.
 *****************************************************************************/
HalI2cHandle_t HalI2cOpen(uint8_t busId, uint8_t slaveAddr, uint8_t regWidth, uint8_t dataWidth);

/*****************************************************************************/
/**
 * @brief   close the low level driver
 * @param   HalI2cHandle   Handle to HAL session as returned by @ref HalI2cOpen.
 * @return  Result of operation.
 *
 * @note    When reference count is 1, close the i2c device file, and when count
 *          is greater than 1, return a status error code.
 *****************************************************************************/
RESULT HalI2cClose(HalI2cHandle_t HalI2cHandle);

/*****************************************************************************/
/**
 * @brief   reads a number of data from the memory to a buffer starting a the given address
 * @param   HalI2cHandle    Handle to HAL session as returned by @ref HalI2cOpen.
 * @param   regAddr         Address of register to read.
 * @param   data            Pointer to local memory holding the register data being read.
 * @return  Result of operation.
 *
 * @note    The register data gets read starting with the least significant byte!
 *****************************************************************************/
RESULT HalReadI2CReg(HalI2cHandle_t HalI2cHandle, uint16_t regAddr, uint16_t *data);


/*****************************************************************************/
/**
 * @brief   writes a number of data from a buffer to the memory starting a the given address
 * @param   HalI2cHandle    Handle to HAL session as returned by @ref HalI2cOpen.
 * @param   regAddr         Address of register to write.
 * @param   data            Register data to be written.
 * @return  Result of operation.
 *
 * @note    The register data gets written starting with the least significant byte!
 *****************************************************************************/
RESULT HalWriteI2CReg(HalI2cHandle_t HalI2cHandle, uint16_t regAddr, uint16_t data);


/*****************************************************************************/
/**
 * @brief   open the low level driver
 * 
 * @return  handle of driver; NULL on failure
 *
 * @note    Create MI sub-system handle and set internal reference count to 1.
 *****************************************************************************/
HalMssHandle_t HalMssOpen(void);

/*****************************************************************************/
/**
 * @brief   close the low level driver
 * @param   HalMssHandle   Handle to HAL session as returned by @ref HalMssOpen.
 * @return  Result of operation.
 *
 * @note    When reference count is 1, close the MI sub-system device file, and 
 *          when count is greater than 1, return a status error code.
 *****************************************************************************/
RESULT HalMssClose(HalMssHandle_t HalMssHandle);

/*****************************************************************************/
/**
 * @brief   reset MI sub-system
 * @param   HalMssHandle   Handle to HAL session as returned by @ref HalMssOpen.
 * @return  Result of operation.
 *
 * @note    reset MI sub-system
 *****************************************************************************/
RESULT HalMssReset(HalMssHandle_t HalMssHandle);

/*****************************************************************************/
/**
 * @brief   read data from register
 * @param   HalMssHandle    Handle to HAL session as returned by @ref HalMssOpen.
 * @param   addr            Address of register to read.
 * @return  Result of operation.
 *
 * @note    This is for MI sub-system.
 *****************************************************************************/
uint32_t HalMssReadReg(HalMssHandle_t HalMssHandle, uint32_t addr);

/*****************************************************************************/
/**
 * @brief   write data to register
 * @param   HalMssHandle    Handle to HAL session as returned by @ref HalMssOpen.
 * @param   addr            Address of register to write.
 * @param   val             Data value.
 * @return  Result of operation.
 *
 * @note    This is for MI sub-system.
 *****************************************************************************/
RESULT HalMssWriteReg(HalMssHandle_t HalMssHandle, uint32_t addr, uint32_t val);

/*****************************************************************************/
/**
 *          HalMssConnectIrq()
 *
 *  @brief  Register interrupt service routine with system software.
 *
 *  @param  HalMssHandle    Handle to HAL session as returned by @ref HalMssOpen
 *  @param  pIrqCtx         Reference of hal irq context structure that represent this connection
 *  @param  int_src         Number of the interrupt source, set to 0 if not needed
 *  @param  IsrFunction     First interrupt routine, (first level handler) set to NULL if not needed
 *  @param  DpcFunction     Second interrupt routine (second level handler)
 *  @param  pContext        It is provided when the interrupt routines are called
 *
 *  @warning Add platform specific code to connect to your local interrupt source
 *
 *  @return                     Status of operation
 *  @retval RET_SUCCESS         ISR registered successfully
 *  @retval RET_FAILURE et al.  ISR not registered
 *
 *****************************************************************************/
RESULT HalMssConnectIrq
(
    HalMssHandle_t HalMssHandle,
    HalIrqCtx_t *pIrqCtx,
    uint32_t    int_src,
    osIsrFunc   IsrFunction,
    osDpcFunc   DpcFunction,
    void*       pContext
);

/*****************************************************************************/
/**
 *          HalMssDisconnectIrq()
 *
 *  @brief  Deregister interrupt service routine from system software.
 *
 *  @param  pIrqCtx             Reference of hal irq context structure that represent this connection
 *
 *  @return                     Status of operation
 *  @retval RET_SUCCESS         ISR deregistered successfully
 *  @retval RET_FAILURE et al.  ISR not properly deregistered
 *
 *****************************************************************************/
RESULT HalMssDisconnectIrq
(
    HalIrqCtx_t *pIrqCtx
);

/*****************************************************************************/
/**
 * @brief   tell HAL about another user of low level driver
 * @param   HalHandle   Handle to HAL session.
 * @param   devType     Device type.
 * @return  Result of operation.
 *
 * @note
 *****************************************************************************/
RESULT HalAddRef(HalHandle_t HalHandle, uint8_t devType);

/*****************************************************************************/
/**
 * @brief   tell HAL about gone user of low level driver
 * @param   HalHandle   Handle to HAL session.
 * @param   devType     Device type.
 * @return  Result of operation.
 *
 * @note    If the internal ref count is zero, the HAL will be closed as well.
 *****************************************************************************/
RESULT HalDelRef(HalHandle_t HalHandle, uint8_t devType);

/*****************************************************************************/
/**
 * @brief   Enables/Disables reset of given devices. HAL takes care of polarity!
 *          See @ref HalSetCamConfig() for details regarding CAM devices,
 *          and @ref HalSetCamPhyConfig() for details regarding CAMPHY devices.
 * @param   HalHandle   Handle to HAL session as returned by @ref HalOpen.
 * @param   dev_mask    Mask of devices to change reset state.
 * @return  Result of operation.
 *
 * @note    Device mask is bitwise OR (|) of HAL_DEVID_xxx.
 *****************************************************************************/
RESULT HalSetReset(HalHandle_t HalHandle, uint32_t dev_mask, bool_t activate);

/*****************************************************************************/
/**
 * @brief   Enables/Disables power of given devices. HAL takes care of polarity!
 *          See @ref HalSetCamConfig() for details regarding CAM devices,
 *          and @ref HalSetCamPhyConfig() for details regarding CAMPHY devices.
 * @param   HalHandle   Handle to HAL session as returned by @ref HalOpen.
 * @param   dev_mask    Mask of devices to change power state.
 * @return  Result of operation.
 *
 * @note    Device mask is bitwise OR (|) of HAL_DEVID_xxx.
 *****************************************************************************/
RESULT HalSetPower(HalHandle_t HalHandle, uint32_t dev_mask, bool_t activate);

/*****************************************************************************/
/**
 * @brief   reads a value from a given address
 * @param   HalHandle   Handle to HAL session as returned by @ref HalOpen.
 * @param   reg_address Address of register to read.
 * @return  Register value.
 *
 * @note    It is required to pass in the full address (base address + offset).
 *****************************************************************************/
INLINE uint32_t HalReadReg( HalHandle_t HalHandle, uint32_t reg_address )
{
    uint32_t read;
    (void) HalHandle;
    HalIspContext_t * HalHandleCtx = (HalIspContext_t *)HalHandle;
    DCT_ASSERT(HalHandleCtx != NULL);

#if defined (HAL_ALTERA)
    read = AlteraFPGABoard_ReadReg(HalHandleCtx->fd, reg_address);
#elif defined (HAL_CMODEL)
    uint8_t ispId = HalHandleCtx->devId;
    readRegister(ispId, reg_address, &read);
#endif

#if ENABLE_REGISTER_DUMP
    if (HalHandleCtx->regDumpFile)
    {
        fprintf(HalHandleCtx->regDumpFile, "1 %08x %08x\n", reg_address, read);
    }
#endif
    // printf("%s 1 %08x %08x\n", __func__, reg_address, read);

    if (HalHandleCtx->regDynDumpFp)
    {
        fprintf(HalHandleCtx->regDynDumpFp, "1 %08x %08x\n", (unsigned int)reg_address, (unsigned int)read);
    }

    return read;
}

/*****************************************************************************/
/**
 * @brief   writes a value to the given address
 * @param   HalHandle   Handle to HAL session as returned by @ref HalOpen.
 * @param   reg_address Address of register to write.
 * @param   value       Value to write into register.
 * @return  none
 *
 * @note    It is required to pass in the full address (base address + offset).
 *****************************************************************************/
INLINE RESULT HalWriteReg( HalHandle_t HalHandle, uint32_t reg_address, uint32_t value )
{
    DCT_ASSERT(HalHandle != NULL);
    HalIspContext_t * HalHandleCtx = (HalIspContext_t *)HalHandle;

#if ENABLE_REGISTER_DUMP
    if (HalHandleCtx->regDumpFile)
    {
        fprintf(HalHandleCtx->regDumpFile, "0 %08x %08x\n", reg_address, value);
    }
#endif
    if (HalHandleCtx->regDynDumpFp)
    {
        fprintf(HalHandleCtx->regDynDumpFp, "0 %08x %08x\n", (unsigned int)reg_address, (unsigned int)value);
    }
    // printf("%s 0 %08x %08x\n", __func__, reg_address, value);
#if defined (HAL_ALTERA)
    (void) AlteraFPGABoard_WriteReg(HalHandleCtx->fd, reg_address, value );
#elif defined (HAL_CMODEL)
    uint8_t ispId = HalHandleCtx->devId;
    writeRegister(ispId, reg_address, value);
#endif
    return RET_SUCCESS;
}

/*****************************************************************************/
/**
 *          HalConnectIrq()
 *
 *  @brief  Register interrupt service routine with system software.
 *
 *  @param  Handle to HAL session as returned by @ref HalOpen
 *  @param  Reference of hal irq context structure that represent this connection
 *  @param  Interrupt object @ref osInterrupt
 *  @param  Number of the interrupt source, set to 0 if not needed
 *  @param  First interrupt routine, (first level handler) set to NULL if not needed
 *  @param  Second interrupt routine (second level handler)
 *  @param  Context provided when the interrupt routines are called
 *
 *  @warning Add platform specific code to connect to your local interrupt source
 *
 *  @return                     Status of operation
 *  @retval RET_SUCCESS         ISR registered successfully
 *  @retval RET_FAILURE et al.  ISR not registered
 *
 *****************************************************************************/
RESULT HalConnectIrq(HalHandle_t HalHandle, HalIrqCtx_t *pIrqCtx, uint32_t int_src, osIsrFunc IsrFunction, osDpcFunc DpcFunction, void* pContext);

/*****************************************************************************/
/**
 *          HalDisconnectIrq()
 *
 *  @brief  Deregister interrupt service routine from system software.
 *
 *  @param  Reference of hal irq context structure that represent this connection
 *
 *  @return                     Status of operation
 *  @retval RET_SUCCESS         ISR deregistered successfully
 *  @retval RET_FAILURE et al.  ISR not properly deregistered
 *
 *****************************************************************************/
RESULT HalDisconnectIrq(HalIrqCtx_t *pIrqCtx);

#if defined (HAL_CMODEL)
/*****************************************************************************/
/**
 *          HalCmHwInit()
 *
 *  @brief  Cmodel specific interface: Setup register initial value
 *
 *  @param  ISP instance type
 *
 *  @return                     Status of operation
 *  @retval RET_SUCCESS         Input set successfully
 *  @retval RET_FAILURE et al.  Input set fails
 *
 *****************************************************************************/
RESULT HalCmHwInit(HalHandle_t HalHandle, uint32_t isp_type, const char* iniRegFileName, uint8_t ispId);

/*****************************************************************************/
/**
 *          HalCmInputInfoSetup()
 *
 *  @brief  Cmodel specific interface: Setup input h and v size
 *
 *  @param  h_size, v_size: image size of input port.
 *
 *  @return                     Status of operation
 *  @retval RET_SUCCESS         Input set successfully
 *  @retval RET_FAILURE et al.  Input set fails
 *
 *****************************************************************************/
// Set h and v size for simulation
RESULT HalCmInputInfoSetup(unsigned char id, unsigned int h_size, unsigned int v_size);

/*****************************************************************************/
/**
 *          HalCmBufTransfer()
 *
 *  @brief  Cmodel specific interface: Buffer transfer for DMA
 *
 *  @param
 *
 *  @return                     Status of operation
 *  @retval RET_SUCCESS         Input set successfully
 *  @retval RET_FAILURE et al.  Input set fails
 *
 *****************************************************************************/
RESULT HalCmBufTransfer(HalHandle_t HalHandle, uint32_t trans_ctrl);

/*****************************************************************************/
/**
 *          HalCmTilemodeBufTransfer()
 *
 *  @brief  Cmodel specific interface: Buffer transfer for DMA in tilemode
 *
 *  @param  HalHandle    Handle to HAL session as returned by @ref HalOpen
 *  @param  trans_ctrl   buffer transfer control variable
 *
 *  @return                     Status of operation
 *  @retval RET_SUCCESS         Input set successfully
 *  @retval RET_FAILURE et al.  Input set fails
 *
 *****************************************************************************/
RESULT HalCmTilemodeBufTransfer(HalHandle_t HalHandle, uint32_t trans_ctrl);

#ifdef ISP_GCMONO
/*****************************************************************************/
/**
 *          HalCmGcmonoBufFill()
 *
 *  @brief  Cmodel specific interface: Buffer transfer for GCMONO
 *
 *  @param
 *
 *  @return                     Status of operation
 *  @retval RET_SUCCESS         Input set successfully
 *  @retval RET_FAILURE et al.  Input set fails
 *
 *****************************************************************************/
RESULT HalCmGcmonoBufFill(HalHandle_t HalHandle, uint8_t *gc_lut, unsigned int lut_length);
#endif

/******************************************************************************
 * Cmodel Specific API: HalCmShdRegisterUpdate
 *****************************************************************************/
// Cmodel Shd register update process
RESULT HalCmShdRegisterUpdate(HalHandle_t HalHandle);


/*****************************************************************************/
/**
 *          HalCmBufConfig()
 *
 *  @brief  Cmodel specific interface: Buffer transfer Configuration
 *
 *  @param
 *
 *  @return                     Status of operation
 *  @retval RET_SUCCESS         Input set successfully
 *  @retval RET_FAILURE et al.  Input set fails
 *
 *****************************************************************************/
RESULT HalCmGetBufConfig(HalHandle_t HalHandle, HalCmodelBufConfig_t **pCmBufCfg);


/*****************************************************************************/
/**
 *          HalCmIspPipeline()
 *
 *  @brief  Cmodel specific interface: Hal ISP8000Process
 *
 *  @param  none
 *
 *  @return                     Status of operation
 *    none
 *
 *****************************************************************************/
void HalCmIspPipeline(HalHandle_t HalHandle, uint8_t ispId);

/*****************************************************************************/
/**
 *          HalCmDumpBuf()
 *
 *  @brief  Cmodel specific interface: memory dump functions
 *
 *  @param  buf: base address of memory
 *          len: length of memory
 *          str: debug info
 *
 *  @return                     None
 *
 *****************************************************************************/
void HalCmDumpBuf(char *buf, int len, char *str);

/*****************************************************************************/
/**
 *          HalCmDumpReFill()
 *
 *  @brief  Cmodel specific interface: from Cmodel memory dump
 *      re-pick up and fill buffer
 *
 *  @param  buffer: base address of memory
 *          file: cmodel dump file
 *          buf_len: buffer limitation
 *
 *  @return                     None
 *
 *****************************************************************************/
RESULT HalCmDumpReFill(unsigned char *buffer, const char *file, unsigned int buf_len);

/*****************************************************************************/
/**
 *          HalCmDumpReFillPostProcess()
 *
 *  @brief  Cmodel specific interface: from Cmodel memory dump
 *      re-pick up and fill buffer
 *
 *  @param  pHalCtx: Hal Context
 *          file: cmodel dump file
 *
 *  @return                     None
 *
 *****************************************************************************/
RESULT HalCmReFillPostProcess(HalIspContext_t *pHalCtx, const char *file);


#endif

/*****************************************************************************/
/**
 * @brief   get the device file identifier
 * @param   HalHandle   Handle to HAL session.
 * @param   devType     Device type.
 * @return  Device file identifier.
 *
 * @note    When fd is -1, it indicates an incorrect fd.
 *****************************************************************************/
int32_t HalGetFdHandle(HalHandle_t HalHandle, uint8_t devType);

/*****************************************************************************/
/**
 *          HalEventGenerator()
 *
 *  @brief  Cmodel specific interface: Hal ISR and Event generator
 *
 *  @param
 *
 *  @return                     Status of operation
 *  @retval RET_SUCCESS         Input set successfully
 *  @retval RET_FAILURE et al.  Input set fails
 *
 *****************************************************************************/
RESULT HalEventGenerator(HalIrqCtx_t *pIrqCtx, HalIsrSrc_t isr_src, uint32_t misValue);

#if defined ( HAL_ALTERA )
/*****************************************************************************/
/**
 *          HalFpgaDumpBuf()
 *
 *  @brief  FPGA online dump function: memory dump functions
 *
 *  @param  buf: base address of memory
 *          len: length of memory
 *          str: debug info
 *
 *  @return                     None
 *
 *****************************************************************************/
void HalFpgaDumpBuf (char *buf, int len, char *str);


#endif

/******************************************************************************
 * stuff below here requires the inline API implementations being included
 * as it requires some more hal variant depended header files being loaded
 *****************************************************************************/



#ifdef __cplusplus
}
#endif

//!@} defgroup HAL_API

#endif /* __HAL_API_H__ */
