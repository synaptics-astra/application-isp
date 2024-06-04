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

#ifndef __CAMERA_DEVICE_BUF_DEFS_H__
#define __CAMERA_DEVICE_BUF_DEFS_H__

#include <ebase/types.h>

#define UNIQUE_ENUM_NAME(u)     assert_static__ ## u
#define GET_ENUM_NAME(x)        UNIQUE_ENUM_NAME(x)
#define DCT_ASSERT_STATIC(e)    enum { GET_ENUM_NAME(__LINE__) = 1/(e) }

#define BUFF_POOL_MAX_INPUT_BUF_NUMBER 8
#define BUFF_POOL_MAX_OUTPUT_BUF_NUMBER 16

#define PIC_BUFFER_ALIGN            (0x400)
#define PIC_WIDTH_ALIGN             16
#define ALIGN_16BYTES(width)         (((width) + (0xF)) & (~(0xF)))


#define PIC_BUFFER_MEM_SIZE_MAX     (512 * 1024 * 1024)

#define PIC_BUFFER_NUM_INPUT        (1)
#define PIC_BUFFER_SIZE_INPUT       (110 * 1024 * 1024)    // 55MPixel@12bit => 110MByte

//The Software does not need too many large buffer, or will out of memory in cmodel test.
#ifdef SW_TEST
#define PIC_BUFFER_NUM_LARGE_IMAGE   (4)
#else
#define PIC_BUFFER_NUM_LARGE_IMAGE   (7)
#endif
#define PIC_BUFFER_SIZE_LARGE_IMAGE  (42 * 1024 * 1024)    // 21MPixel@12bit => 42MByte
#define PIC_BUFFER_NUM_SMALL_IMAGE   (13)
#define PIC_BUFFER_SIZE_SMALL_IMAGE  (8 * 1024 * 1024)     // 2MPixel@4:4:4 10bit  => 8MByte

DCT_ASSERT_STATIC(((PIC_BUFFER_NUM_INPUT*PIC_BUFFER_SIZE_INPUT) + (PIC_BUFFER_NUM_LARGE_IMAGE*PIC_BUFFER_SIZE_LARGE_IMAGE) + (PIC_BUFFER_NUM_SMALL_IMAGE*PIC_BUFFER_SIZE_SMALL_IMAGE)) < PIC_BUFFER_MEM_SIZE_MAX);

#ifdef ISP_SINGLE_DOM_OVER_MP
#define PIC_BUFFER_NUM_MAIN_SENSOR  (10)
#define PIC_BUFFER_SIZE_MAIN_SENSOR (16 * 1024 * 1024)    //  2MPixel@4:4:4  => 6MByte
#define PIC_BUFFER_NUM_SELF1_SENSOR  (0)
#define PIC_BUFFER_SIZE_SELF1_SENSOR (6 * 1024 * 1024)     // 2MPixel@4:4:4  => 6MByte, no buffer for sp
#define PIC_BUFFER_NUM_SELF2_SENSOR  (0)
#define PIC_BUFFER_SIZE_SELF2_SENSOR (6 * 1024 * 1024)     // 2MPixel@4:4:4  => 6MByte, no buffer for sp
#define PIC_BUFFER_NUM_MCMWR_SENSOR  (6)
#define PIC_BUFFER_SIZE_MCMWR_SENSOR (8 * 1024 * 1024)

#ifdef ISP_MI_HDR
#define PIC_BUFFER_NUM_HDR_SENSOR  (6)
#define PIC_BUFFER_SIZE_HDR_SENSOR (8 * 1024 * 1024)
#endif
#define PIC_BUFFER_NUM_PP_PATH_RAW_SENSOR  (0)
#define PIC_BUFFER_SIZE_PP_PATH_RAW_SENSOR (6 * 1024 * 1024)     // 2MPixel@4:4:4  => 6MByte, no buffer for sp

#define PIC_BUFFER_NUM_MAIN_SENSOR_IMGSTAB  ( 10 )
#define PIC_BUFFER_SIZE_MAIN_SENSOR_IMGSTAB ( 6 * 1024 * 1024 )    // ISI_RES_4416_3312 => 14MPixel@12bit => 28MByte
#define PIC_BUFFER_NUM_SELF_SENSOR_IMGSTAB  ( 10 )
#define PIC_BUFFER_SIZE_SELF_SENSOR_IMGSTAB ( 6 * 1024 * 1024 )    // ISI_RES_4416_3312 => 14MPixel@12bit => 28MByte

DCT_ASSERT_STATIC(((PIC_BUFFER_NUM_MAIN_SENSOR*PIC_BUFFER_SIZE_MAIN_SENSOR) + (PIC_BUFFER_NUM_SELF1_SENSOR*PIC_BUFFER_SIZE_SELF1_SENSOR) ) < PIC_BUFFER_MEM_SIZE_MAX);
#else
#define PIC_BUFFER_NUM_MAIN_SENSOR  (10)
#define PIC_BUFFER_SIZE_MAIN_SENSOR (36 * 1024 * 1024)    // ISI_RES_4416_3312 => 14MPixel@12bit => 28MByte
#define PIC_BUFFER_NUM_SELF1_SENSOR  (8)
#define PIC_BUFFER_SIZE_SELF1_SENSOR (6 * 1024 * 1024)     // 2MPixel@4:4:4  => 6MByte
#define PIC_BUFFER_NUM_SELF2_SENSOR  (2)
#define PIC_BUFFER_SIZE_SELF2_SENSOR (6 * 1024 * 1024)     // 2MPixel@4:4:4  => 6MByte
#define PIC_BUFFER_NUM_MCMWR_SENSOR  (6)
#define PIC_BUFFER_SIZE_MCMWR_SENSOR (8 * 1024 * 1024)
#ifdef ISP_MI_HDR
#define PIC_BUFFER_NUM_HDR_SENSOR  (6)
#define PIC_BUFFER_SIZE_HDR_SENSOR (8 * 1024 * 1024)
#endif
#define PIC_BUFFER_NUM_METADATA      (10)
#define PIC_BUFFER_SIZE_METADATA (8* 1024 * 1024)    //  2MPixel@4:4:4  => 6MByte

#define PIC_BUFFER_NUM_PP_PATH_RAW_SENSOR  (2)
#define PIC_BUFFER_SIZE_PP_PATH_RAW_SENSOR (6 * 1024 * 1024)     // 2MPixel@4:4:4  => 6MByte

#define PIC_BUFFER_NUM_MAIN_SENSOR_IMGSTAB  ( 10 )
#define PIC_BUFFER_SIZE_MAIN_SENSOR_IMGSTAB ( 6 * 1024 * 1024 )    // ISI_RES_4416_3312 => 14MPixel@12bit => 28MByte
#define PIC_BUFFER_NUM_SELF_SENSOR_IMGSTAB  ( 10 )
#define PIC_BUFFER_SIZE_SELF_SENSOR_IMGSTAB ( 6 * 1024 * 1024 )    // ISI_RES_4416_3312 => 14MPixel@12bit => 28MByte

DCT_ASSERT_STATIC(((PIC_BUFFER_NUM_MAIN_SENSOR*PIC_BUFFER_SIZE_MAIN_SENSOR) + (PIC_BUFFER_NUM_SELF1_SENSOR*PIC_BUFFER_SIZE_SELF1_SENSOR) ) < PIC_BUFFER_MEM_SIZE_MAX);
#endif

typedef enum _BUFF_MODE_ {
    BUFF_MODE_INVALID     = 0,
    BUFF_MODE_USRPTR      = 1,
    BUFF_MODE_RESMEM      = 2,
    BUFF_MODE_MAX
} BUFF_MODE;

#define MEDIA_BUF_ALIGN(buf, align) (((buf)+((align)-1U)) & ~((align)-1U))

/*Typedef HAL handle*/
typedef void *HalHandle_t;

typedef struct _BufIdentity_ {
    uint32_t buff_size;     // common: biffer size
    int width;              // common: image width
    int height;             // common: image height
    int format;             // common: image format enum
    int widthBytes;         // for yuvxxxP/yuvxxxSP , it means widthBytes of y channel;for yuvxxxI , it means widthBytes of yuv channel
    int cbCrWidthBytes;     // yuvxxxSP: width bytes of uv line
    int cbCrHeightPixel;    // yuvxxxSP: num of uv line
    int cbWidthBytes;       // yuvxxxP: width bytes of u line
    int cbHeightPixel;      // yuvxxxP: num of u line
    int crWidthBytes;       // yuvxxxP: width bytes of v line
    int crHeightPixel;      // yuvxxxP: num of v line
    int yuvOrder;           // RGB888: y,u,v order
    int alignMode;          // common: align mode enmu
    uint8_t bitWidth;       // common: bit width enmu
    uint8_t * bufferInstance;
    uint32_t buff_address;  // common: buffer physical address
} BufIdentity;

typedef void* BufMgmtHandle_t;


// Pic buffers

/*****************************************************************************/
/**
 *          PicBufType_t
 *
 * @brief   The type of image data a picture buffer holds.
 *
 * @note    MVDU_FXQuad requires PIC_BUF_TYPE_YCbCr422 in PIC_BUF_LAYOUT_SEMIPLANAR mode.
 *
 *****************************************************************************/
typedef enum PicBufType_e {
    PIC_BUF_TYPE_INVALID  = 0,
    PIC_BUF_TYPE_JPEG     = 2,
    PIC_BUF_TYPE_YCbCr444 = 3,
    PIC_BUF_TYPE_YCbCr422 = 4,
    PIC_BUF_TYPE_YCbCr420 = 5,
    PIC_BUF_TYPE_YCbCr400 = 6,
    PIC_BUF_TYPE_RGB888   = 7,
    PIC_BUF_TYPE_RGB666   = 8,  // R, G & B are LSBit aligned!
    PIC_BUF_TYPE_RGB565   = 9,  // TODO: don't know the memory layout right now, investigate!
    PIC_BUF_TYPE_RAW8     = 10,
    PIC_BUF_TYPE_RAW12    = 11,   // includes: 12bits, MSBit aligned, LSByte first!
    PIC_BUF_TYPE_RAW10    = 13,   // includes: 10bits, MSBit aligned, LSByte first!
    PIC_BUF_TYPE_RAW14    = 14,   // includes: 14bits, MSBit aligned, LSByte first!
    PIC_BUF_TYPE_RAW16    = 15,   // includes: 9..16bits, MSBit aligned, LSByte first!
    PIC_BUF_TYPE_RAW20    = 16,   // includes: 20bits, compressed to 16bit MSBit aligned, LSByte first!
    PIC_BUF_TYPE_RAW20_COMPRESS    = 17,   // includes: 20bits compressed to 16bit MSBit aligned, LSByte first!
    PIC_BUF_TYPE_RAW24    = 18,   // includes: 24bits, MSBit aligned, LSByte first!
    PIC_BUF_TYPE_RAW24_COMPRESS    = 19,   // includes: 24bits, compressed to 16bit MSBit aligned, LSByte first!
    PIC_BUF_TYPE_META     = 20,
    PIC_BUF_TYPE_DATA     = 21,   // just some sequential data
    PIC_BUF_TYPE_YCbCr32  = 22,
    // PIC_BUF_TYPE_YCbCr400 = 0x33, // "Black&White"
    PIC_BUF_TYPE_RGB32    = 23,
    _PIC_BUF_TYPE_DUMMY_
} PicBufType_t;


/*****************************************************************************/
/**
 *          PicBufLayout_t
 *
 * @brief   The layout of the image data a picture buffer holds.
 *
 * @note    MVDU_FXQuad requires PIC_BUF_TYPE_YCbCr422 in PIC_BUF_LAYOUT_SEMIPLANAR mode.
 *
 *****************************************************************************/
typedef enum PicBufLayout_e {
    PIC_BUF_LAYOUT_INVALID         = 0,

    PIC_BUF_LAYOUT_COMBINED        = 0x10,  // PIC_BUF_TYPE_DATA:      Data: D0 D1 D2...
                                            // PIC_BUF_TYPE_RAW8:      Data: D0 D1 D2...
                                            // PIC_BUF_TYPE_RAW16/10:  Data: D0L D0H D1L D1H...
                                            // PIC_BUF_TYPE_JPEG:      Data: J0 J1 J2...
                                            // PIC_BUF_TYPE_YCbCr444:  Data: Y0 Cb0 Cr0 Y1 Cb1Cr1...
                                            // PIC_BUF_TYPE_YCbCr422:  Data: Y0 Cb0 Y1 Cr0 Y2 Cb1 Y3 Cr1...
                                            // PIC_BUF_TYPE_YCbCr32:   Data: Cr0 Cb0 Y0 A0 Cr1 Cb1 Y1 A1...
                                            // PIC_BUF_TYPE_RGB888:    Data: R0 G0 B0 R1 B2 G1...
                                            // PIC_BUF_TYPE_RGB666:    Data: {00,R0[5:0]} {00,G0[5:0]} {00,B0[5:0]} {00,R1[5:0]} {00,G2[5:0]} {00,B3[5:0]}...
                                            // PIC_BUF_TYPE_RGB565:    Data: {R0[4:0],G0[5:3]} {G0[2:0],B0[4:0]} {R1[4:0],G1[5:3]} {G1[2:0],B1[4:0]}... (is this correct?)
                                            // PIC_BUF_TYPE_RGB32:     Data: B0 G0 R0 A0 B1 G1 R1 A1...
    PIC_BUF_LAYOUT_BAYER_RGRGGBGB  = 0x11,  // 1st line: RGRG... , 2nd line GBGB... , etc.
    PIC_BUF_LAYOUT_BAYER_GRGRBGBG  = 0x12,  // 1st line: GRGR... , 2nd line BGBG... , etc.
    PIC_BUF_LAYOUT_BAYER_GBGBRGRG  = 0x13,  // 1st line: GBGB... , 2nd line RGRG... , etc.
    PIC_BUF_LAYOUT_BAYER_BGBGGRGR  = 0x14,  // 1st line: BGBG... , 2nd line GRGR... , etc.

    PIC_BUF_LAYOUT_SEMIPLANAR      = 0x20,  // PIC_BUF_TYPE_YCbCr422:  Luma:  Y0 Y1 Y2 Y3... ; Chroma: Cb0 Cr0 Cb1 Cr1...
                                            // PIC_BUF_TYPE_YCbCr420:  Luma:  Y0 Y1 Y2 Y3... ; Chroma: Cb0 Cr0 Cb1 Cr1...
                                            // PIC_BUF_TYPE_YCbCr400:  Luma:  Y0 Y1 Y2 Y3... ; Chroma: not used

    PIC_BUF_LAYOUT_PLANAR          = 0x30,  // PIC_BUF_TYPE_YCbCr444:  Y: Y0 Y1 Y2 Y3...;  Cb: Cb0 Cb1 Cb2 Cb3...; Cr: Cr0 Cr1 Cr2 Cr3...
                                            // PIC_BUF_TYPE_YCbCr422:  Y: Y0 Y1 Y2 Y3...;  Cb: Cb0 Cb1 Cb2 Cb3...; Cr: Cr0 Cr1 Cr2 Cr3...
                                            // PIC_BUF_TYPE_YCbCr420:  Y: Y0 Y1 Y2 Y3...;  Cb: Cb0 Cb1 Cb2 Cb3...; Cr: Cr0 Cr1 Cr2 Cr3...
                                            // PIC_BUF_TYPE_YCbCr400:  Y: Y0 Y1 Y2 Y3...;  Cb: not used;           Cr: not used...
                                            // PIC_BUF_TYPE_RGB888:    R: R0 R1 R2 R3...;  G:  G0 G1 G2 G3...;     B:  B0 B1 B2 B3...
                                            // PIC_BUF_TYPE_RGB666:    R: {00,R0[5:0]}...; G:  {00,G0[5:0]}...;    B:  {00,B0[5:0]}...
    PIC_BUF_LAYOUT_META_DATA      = 0x40,
    _PIC_BUF_LAYOUT_DUMMY_
} PicBufLayout_t;

/*****************************************************************************/
/**
 *          PixelDataAlignMode_t
 *
 * @brief   The align mode of the image data a picture buffer holds.
 *
 *
 *****************************************************************************/

typedef enum PicBufAlign_e{
    PIC_BUF_DATA_ALIGN_MODE_INVALID          = -1,
    PIC_BUF_DATA_UNALIGN_MODE                = 0,  // pixel data not aligned.
    PIC_BUF_DATA_ALIGN_128BIT_MODE           = 1,  // pixel data  aligned with 128 bit.
	PIC_BUF_DATA_ALIGN_DOUBLE_WORD           = 1, // pixel data  aligned with double word.
	PIC_BUF_DATA_ALIGN_WORD                  = 2,  // pixel data  aligned with word.
    PIC_BUF_DATA_ALIGN_16BIT_MODE            = 2,  // pixel data  aligned with 16 bit.
    PIC_BUF_DATA_ALIGN_MODE_MAX
}PicBufAlign_t;

typedef enum PicBufYUVBIT_e{
	PIC_BUF_DATA_YUV_BIT_MAX_INVALID = -1,
	PIC_BUF_DATA_YUV_8_BIT = 0,  //  yuv pixel data 8 bit
	PIC_BUF_DATA_YUV_10_BIT = 1, // yuv pixel data 10 bit
	PIC_BUF_DATA_YUV_12_BIT = 2, // yuv pixel data 12 bit
	PIC_BUF_DATA_YUV_BIT_MAX
}PicBufYUVBIT_t;

/*****************************************************************************/
/**
 * @brief   Enumeration type to specify the order of YUV or RGB channel,
 *          Right now only surpport RGB888 format
 *
 *****************************************************************************/
typedef enum PicBufYuvOrder_e
{
    PIC_BUF_CHANNEL_ORDER_INVALID = -1,
    PIC_BUF_CHANNEL_ORDER_YUV = 0,  /** 0: YUV or RGB */
    PIC_BUF_CHANNEL_ORDER_YVU = 1,  /** 1: YVU or RBG */
    PIC_BUF_CHANNEL_ORDER_UYV = 2,  /** 2: UYV or GRB */
    PIC_BUF_CHANNEL_ORDER_VYU = 3,  /** 3: VYU or BRG */
    PIC_BUF_CHANNEL_ORDER_UVY = 4,  /** 4: UVY or GBR */
    PIC_BUF_CHANNEL_ORDER_VUY = 5,  /** 5: VUY or BGR */
    PIC_BUF_CHANNEL_ORDER_MAX = 6,
}PicBufYuvOrder_t;


/*******************************************************************************
 * @brief Common SCMI buffer type
 *
 * This structure defines a SCMI buffer. In addition to an address pointer to
 * the actual buffer, this structure also contains the size of the buffer and
 * an optional (may be null) pointer to some buffer meta data. This meta data
 * is defined seperately for each specific buffer type. The buffer_flags
 * variable contains more information about the buffer in the form of
 * (up to 32) bit flags.
 * The bit flags and their meaning are defined by each SCMI system separately.
 * For a list of buffer bit flags, see the respective system's documentation.
 */

/*Be careful of machine int width bits(32/64)*/
typedef struct _ScmiBuffer {
    void        *p_next;        /**< Pointer used by module to chain SCMI-buffers */
    void        *p_address;     /**< Address of the actual buffer */
    uint32_t    base_address;   /**< HW address of the actual buffer */
    BufIdentity buf_id;
    uint32_t    size;           /**< Size of the buffer in bytes */
    uint32_t    buffer_flags;   /**< Generic buffer flags */
    int64_t     time_stamp;     /**< Time stamp of the buffer in ticks with 27 MHz resolution */
    void        *p_meta_data;   /**< Optional pointer to buffer meta data */
} ScmiBuffer;


/**
 *
 * @brief The MediaBufferPool holds elements from type MediaBuffer_t.
 */
typedef struct MediaBuffer_s {
    uint32_t      baseAddress;  /**< HW address of system memory buffer. */
    uint32_t      baseSize;     /**< Base size of buffer. */
    uint32_t      lockCount;    /**< Counting how many times buffer is used. 0 means buffer is free. */
    void         *pOwner;       /**< The buffer management context to which media buffer belongs */
    bool_t        isFull;       /**< Flag set to TRUE when buffer is put in queue as a full buffer */
    void         *pMetaData;    /**< Pointer to optional meta data structure. */
    uint8_t       index;        /**< The index in buffer management context */
    BUFF_MODE     bufMode;      /**< The memory type of this media buffer */
    void         *pIplAddress;  /**< The virtual address of this buffer in ISP platform.*/
#ifdef WITH_FLEXA
    int           fd;
    void          *mobj;
#endif
    ScmiBuffer    buf;          /**< Common SCMI buffer type. not use.TODO delete */

} MediaBuffer_t;


/*****************************************************************************/
/**
 *          PicBufPlane_t
 *
 * @brief   Common information about a color component plane within an image buffer.
 *
 *****************************************************************************/
typedef struct PicBufPlane_s {
    uint8_t    *pData;
    uint32_t    BaseAddress;
    uint32_t    PicWidthPixel;
    uint32_t    PicWidthBytes;
    uint32_t    PicHeightPixel;
    uint8_t     PixelDataAlignMode;
    uint8_t     bitWidth;
} PicBufPlane_t;

/*****************************************************************************/
/**
 *          PicBufExpoInfo_t
 *
 * @brief   image expo info.
 *
 *****************************************************************************/
typedef struct PicBufExpoInfo_s {
    uint16_t vsTime;
    uint16_t sTime;
    uint16_t mTime;
    uint16_t lTime;
} PicBufExpoInfo_t;

/*****************************************************************************/
/**
 *          PicBufMetadataInfo_t
 *
 * @brief  image all metadata info.
 *
 *****************************************************************************/
typedef struct PicBufMetadataInfo_s {
    uint64_t frameCount;
    PicBufExpoInfo_t expoInfo;

} PicBufMetadataInfo_t;

#define METADATA_MAX_NUM 3

/*****************************************************************************/
/**
 *          MetadataBufInfo_t
 *
 * @brief  metadata windows buffer info.
 *
 *****************************************************************************/
typedef struct MetadataBufInfo_s {
    uint8_t winNum;
    uint8_t *pBuffer[METADATA_MAX_NUM];
    uint32_t address[METADATA_MAX_NUM];
    uint32_t bufferSize[METADATA_MAX_NUM];
} MetadataBufInfo_t;

/*****************************************************************************/
/**
 *          PicBufMetaData_t
 *
 * @brief   All the meta data one needs to know about an image buffer.
 *
 *****************************************************************************/
typedef struct PicBufMetaData_s {
    PicBufType_t            Type;       // type of picture data
    PicBufLayout_t          Layout;     // kind of data layout
    uint32_t                Align;      // min. alignment required for color component planes or sub buffer base adresses for this picture buffer
    int64_t                 TimeStampUs;  // timestamp in us
    struct PicBufMetaData_s *pNext3D;   // Reference to PicBufMetaData of the subsequent buffer in a 3D descriptor chain, valid only in 3D mode; set to NULL if last in chain or for 2D mode.
                                        // Note: depending on the 3D format in use, the primary buffer holds left image data while the secondary buffer holds right or depth information.
                                        //       Certain 3D formats require further buffers, in which case the 3D chain consists of more than two descriptors.
  //  BufIdentity             buf_id;

    PicBufMetadataInfo_t    metaInfo;
    PicBufYuvOrder_t        yuvOrder;
    union Data_u {                      // the type and layout dependent meta data
        struct data_s {      // DATA
            uint8_t    *pData;
            uint32_t    BaseAddress;
            uint32_t    DataSize;
        } data;

        struct data_meta {   // meta
            uint8_t    *pData;
            uint32_t    BaseAddress;
            uint32_t    DataSize;

            PicBufPlane_t  plane[METADATA_MAX_NUM];  //meta data windows number 3
            MetadataBufInfo_t metaBufInfo;
        } meta;

        PicBufPlane_t raw;   // RAW8, RAW16

        struct jpeg_s {      // JPEG
            uint8_t    *pHeader;
            uint32_t    HeaderSize;
            uint8_t    *pData;
            uint32_t    BaseAddress;
            uint32_t    DataSize;
        } jpeg;

        union YCbCr_u {     // YCbCr444, YCbCr422, YCbCr420, YCbCr32
            PicBufPlane_t combined;
            struct semiplanar_s {
                PicBufPlane_t Y;
                PicBufPlane_t CbCr;
        		uint8_t bitWidth;
	    } semiplanar;
            struct planar_YUV_s {
                PicBufPlane_t Y;
                PicBufPlane_t Cb;
                PicBufPlane_t Cr;
            } planar;
        } YCbCr;

        union RGB_u {        // RGB888, RGB32
            PicBufPlane_t combined;
            struct planar_RGB_s {
                PicBufPlane_t R;
                PicBufPlane_t G;
                PicBufPlane_t B;
            } planar;
        } RGB;
#ifdef ISP_MI_BP
        union BAYER_u {         // rggb bggr
            struct planar_BAYER_s {
                PicBufPlane_t R;
                PicBufPlane_t Gr;
                PicBufPlane_t Gb;
                PicBufPlane_t B;
            } planar;
        } BAYER;
#endif
    } Data;
} PicBufMetaData_t;

#endif  // __CAMERA_DEVICE_BUF_DEFS_H__
