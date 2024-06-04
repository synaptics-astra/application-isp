/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2020 Synaptics Incorporated. All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */
#ifndef __IMX258_LINEAR_H__
#define __IMX258_LINEAR_H__

/* 1080P Bin HV1/2 2096x1176 */
static uint16_t IMX258_REG_1920[][2] = {
  {0x0100, 0x00},
  {0x0104, 0x01},
  {0x0136, 0x19},
  {0x0137, 0x00},
  {0x3051, 0x00},
  {0x6B11, 0xCF},
  {0x7FF0, 0x08},
  {0x7FF1, 0x0F},
  {0x7FF2, 0x08},
  {0x7FF3, 0x1B},
  {0x7FF4, 0x23},
  {0x7FF5, 0x60},
  {0x7FF6, 0x00},
  {0x7FF7, 0x01},
  {0x7FF8, 0x00},
  {0x7FF9, 0x78},
  {0x7FFA, 0x01},
  {0x7FFB, 0x00},
  {0x7FFC, 0x00},
  {0x7FFD, 0x00},
  {0x7FFE, 0x00},
  {0x7FFF, 0x03},
  {0x7F76, 0x03},
  {0x7F77, 0xFE},
  {0x7FA8, 0x03},
  {0x7FA9, 0xFE},
  {0x7B24, 0x81},
  {0x7B25, 0x01},
  {0x6564, 0x07},
  {0x6B0D, 0x41},
  {0x653D, 0x04},
  {0x6B05, 0x8C},
  {0x6B06, 0xF9},
  {0x6B08, 0x65},
  {0x6B09, 0xFC},
  {0x6B0A, 0xCF},
  {0x6B0B, 0xD2},
  {0x6700, 0x0E},
  {0x6707, 0x0E},
  {0x5F04, 0x00},
  {0x5F05, 0xED},
  {0x0301, 0x05},
  {0x0303, 0x02},
  {0x0305, 0x04},
  {0x0306, 0x00},
  {0x0307, 0x4E},
  {0x0309, 0x0A},
  {0x030B, 0x01},
  {0x030D, 0x04},
  {0x030E, 0x00},
  {0x030F, 0x37},
  {0x0310, 0x01},
  {0x0820, 0x05},
  {0x0821, 0x5F},
  {0x0822, 0x00},
  {0x0823, 0x00},
  {0x4648, 0x7F},
  {0x7420, 0x00},
  {0x7421, 0x5C},
  {0x7422, 0x00},
  {0x7423, 0xD7},
  {0x9104, 0x04},
  {0x0112, 0x0A},
  {0x0113, 0x0A},
  {0x0114, 0x03},
  {0x0342, 0x14},
  {0x0343, 0xE8},
  {0x0340, 0x04},
  {0x0341, 0xBC},
  {0x0344, 0x00},
  {0x0345, 0x00},
  {0x0346, 0x01},
  {0x0347, 0x80},
  {0x0348, 0x10},
  {0x0349, 0x6F},
  {0x034A, 0x0A},
  {0x034B, 0xAF},
  {0x0381, 0x01},
  {0x0383, 0x01},
  {0x0385, 0x01},
  {0x0387, 0x01},
  {0x0900, 0x01},
  {0x0901, 0x12},
  {0x0401, 0x01},
  {0x0404, 0x00},
  {0x0405, 0x20},
  {0x0408, 0x00},
  {0x0409, 0x06},
  {0x040A, 0x00},
  {0x040B, 0x00},
  {0x040C, 0x0F},
  {0x040D, 0x02},
  {0x040E, 0x04},
  {0x040F, 0x38},
  {0x3038, 0x00},
  {0x303A, 0x00},
  {0x303B, 0x10},
  {0x300D, 0x00},
  {0x034C, 0x07},
  {0x034D, 0x80},
  {0x034E, 0x04},
  {0x034F, 0x38},
  {0x0600, 0x00},
  {0x0601, 0x00},
  {0x0202, 0x04},
  {0x0203, 0xB2},
  {0x0204, 0x00},
  {0x0205, 0x00},
  {0x020E, 0x01},
  {0x020F, 0x00},
  {0x0210, 0x01},
  {0x0211, 0x00},
  {0x0212, 0x01},
  {0x0213, 0x00},
  {0x0214, 0x01},
  {0x0215, 0x00},
  {0x7BCD, 0x01},
  {0x94DC, 0x20},
  {0x94DD, 0x20},
  {0x94DE, 0x20},
  {0x95DC, 0x20},
  {0x95DD, 0x20},
  {0x95DE, 0x20},
  {0x7FB0, 0x00},
  {0x9010, 0x3E},
  {0x9419, 0x50},
  {0x941B, 0x50},
  {0x9519, 0x50},
  {0x951B, 0x50},
  {0x3030, 0x00},
  {0x3031, 0x00},
  {0x3032, 0x00},
  {0x0220, 0x00},
  {0x0104, 0x00},
  {0x0100, 0x01}, // stream on
};

static uint16_t IMX258_REG_1920_LANE2[][2] = {
  {0x0100, 0x00},
  {0x0104, 0x01},
  {0x0136, 0x19},
  {0x0137, 0x00},
  {0x3051, 0x00},
  {0x6B11, 0xCF},
  {0x7FF0, 0x08},
  {0x7FF1, 0x0F},
  {0x7FF2, 0x08},
  {0x7FF3, 0x1B},
  {0x7FF4, 0x23},
  {0x7FF5, 0x60},
  {0x7FF6, 0x00},
  {0x7FF7, 0x01},
  {0x7FF8, 0x00},
  {0x7FF9, 0x78},
  {0x7FFA, 0x01},
  {0x7FFB, 0x00},
  {0x7FFC, 0x00},
  {0x7FFD, 0x00},
  {0x7FFE, 0x00},
  {0x7FFF, 0x03},
  {0x7F76, 0x03},
  {0x7F77, 0xFE},
  {0x7FA8, 0x03},
  {0x7FA9, 0xFE},
  {0x7B24, 0x81},
  {0x7B25, 0x01},
  {0x6564, 0x07},
  {0x6B0D, 0x41},
  {0x653D, 0x04},
  {0x6B05, 0x8C},
  {0x6B06, 0xF9},
  {0x6B08, 0x65},
  {0x6B09, 0xFC},
  {0x6B0A, 0xCF},
  {0x6B0B, 0xD2},
  {0x6700, 0x0E},
  {0x6707, 0x0E},
  {0x5F04, 0x00},
  {0x5F05, 0xED},
  {0x0301, 0x05},
  {0x0303, 0x02},
  {0x0305, 0x04},
  {0x0306, 0x00},
  {0x0307, 0xC0},
  {0x0309, 0x0A},
  {0x030B, 0x01},
  {0x030D, 0x04},
  {0x030E, 0x00},
  {0x030F, 0x50},
  {0x0310, 0x01},
  {0x0820, 0x03},
  {0x0821, 0xE8},
  {0x0822, 0x00},
  {0x0823, 0x00},
  {0x4648, 0x7F},
  {0x7420, 0x01},
  {0x7421, 0x00},
  {0x7422, 0x03},
  {0x7423, 0x00},
  {0x9104, 0x04},
  {0x0112, 0x0A},
  {0x0113, 0x0A},
  {0x0114, 0x01},
  {0x0342, 0x29},
  {0x0343, 0xE0},
  {0x0340, 0x05},
  {0x0341, 0xD4},
  {0x0344, 0x00},
  {0x0345, 0x00},
  {0x0346, 0x01},
  {0x0347, 0x80},
  {0x0348, 0x10},
  {0x0349, 0x6F},
  {0x034A, 0x0A},
  {0x034B, 0xAF},
  {0x0381, 0x01},
  {0x0383, 0x01},
  {0x0385, 0x01},
  {0x0387, 0x01},
  {0x0900, 0x01},
  {0x0901, 0x12},
  {0x0401, 0x01},
  {0x0404, 0x00},
  {0x0405, 0x20},
  {0x0408, 0x00},
  {0x0409, 0xB6},
  {0x040A, 0x00},
  {0x040B, 0x30},
  {0x040C, 0x0F},
  {0x040D, 0x02},
  {0x040E, 0x04},
  {0x040F, 0x38},
  {0x3038, 0x00},
  {0x303A, 0x00},
  {0x303B, 0x10},
  {0x300D, 0x00},
  {0x034C, 0x07},
  {0x034D, 0x80},
  {0x034E, 0x04},
  {0x034F, 0x38},
  {0x0600, 0x00},
  {0x0601, 0x00},
  {0x0202, 0x05},
  {0x0203, 0xCA},
  {0x0204, 0x00},
  {0x0205, 0x00},
  {0x020E, 0x01},
  {0x020F, 0x00},
  {0x0210, 0x01},
  {0x0211, 0x00},
  {0x0212, 0x01},
  {0x0213, 0x00},
  {0x0214, 0x01},
  {0x0215, 0x00},
  {0x7BCD, 0x01},
  {0x94DC, 0x20},
  {0x94DD, 0x20},
  {0x94DE, 0x20},
  {0x95DC, 0x20},
  {0x95DD, 0x20},
  {0x95DE, 0x20},
  {0x7FB0, 0x00},
  {0x9010, 0x3E},
  {0x9419, 0x50},
  {0x941B, 0x50},
  {0x9519, 0x50},
  {0x951B, 0x50},
  {0x3030, 0x00},
  {0x3031, 0x00},
  {0x3032, 0x00},
  {0x0220, 0x00},
  {0x0104, 0x00},
  {0x0100, 0x01}, // stream on
};

/* 3840x2160 [0AQH5] Golden settings */
static uint16_t IMX258_REG_3840[][2] = {
  {0x0100, 0x00},
  {0x0104, 0x01},
  {0x0136, 0x19},
  {0x0137, 0x00},
  {0x3051, 0x00},
  {0x6B11, 0xCF},
  {0x7FF0, 0x08},
  {0x7FF1, 0x0F},
  {0x7FF2, 0x08},
  {0x7FF3, 0x1B},
  {0x7FF4, 0x23},
  {0x7FF5, 0x60},
  {0x7FF6, 0x00},
  {0x7FF7, 0x01},
  {0x7FF8, 0x00},
  {0x7FF9, 0x78},
  {0x7FFA, 0x01},
  {0x7FFB, 0x00},
  {0x7FFC, 0x00},
  {0x7FFD, 0x00},
  {0x7FFE, 0x00},
  {0x7FFF, 0x03},
  {0x7F76, 0x03},
  {0x7F77, 0xFE},
  {0x7FA8, 0x03},
  {0x7FA9, 0xFE},
  {0x7B24, 0x81},
  {0x7B25, 0x01},
  {0x6564, 0x07},
  {0x6B0D, 0x41},
  {0x653D, 0x04},
  {0x6B05, 0x8C},
  {0x6B06, 0xF9},
  {0x6B08, 0x65},
  {0x6B09, 0xFC},
  {0x6B0A, 0xCF},
  {0x6B0B, 0xD2},
  {0x6700, 0x0E},
  {0x6707, 0x0E},
  {0x5F04, 0x00},
  {0x5F05, 0xED},
  {0x0301, 0x05},
  {0x0303, 0x02},
  {0x0305, 0x04},
  {0x0306, 0x00},
  {0x0307, 0xCB},
  {0x0309, 0x0A},
  {0x030B, 0x01},
  {0x030D, 0x04},
  {0x030E, 0x00},
  {0x030F, 0xAC},
  {0x0310, 0x01},
  {0x0820, 0x10},
  {0x0821, 0xCC},
  {0x0822, 0x00},
  {0x0823, 0x00},
  {0x4648, 0x7F},
  {0x7420, 0x00},
  {0x7421, 0x5C},
  {0x7422, 0x00},
  {0x7423, 0xD7},
  {0x9104, 0x04},
  {0x0112, 0x0A},
  {0x0113, 0x0A},
  {0x0114, 0x03},
  {0x0342, 0x14},
  {0x0343, 0xE8},
  {0x0340, 0x0C},
  {0x0341, 0x58},
  {0x0344, 0x00},
  {0x0345, 0x00},
  {0x0346, 0x00},
  {0x0347, 0x00},
  {0x0348, 0x10},
  {0x0349, 0x6F},
  {0x034A, 0x0C},
  {0x034B, 0x2F},
  {0x0381, 0x01},
  {0x0383, 0x01},
  {0x0385, 0x01},
  {0x0387, 0x01},
  {0x0900, 0x00},
  {0x0901, 0x11},
  {0x0401, 0x00},
  {0x0404, 0x00},
  {0x0405, 0x10},
  {0x0408, 0x00},
  {0x0409, 0xB8},
  {0x040A, 0x01},
  {0x040B, 0xE0},
  {0x040C, 0x0F},
  {0x040D, 0x00},
  {0x040E, 0x08},
  {0x040F, 0x70},
  {0x3038, 0x00},
  {0x303A, 0x00},
  {0x303B, 0x10},
  {0x300D, 0x00},
  {0x034C, 0x0F},
  {0x034D, 0x00},
  {0x034E, 0x08},
  {0x034F, 0x70},
  {0x0600, 0x00},
  {0x0601, 0x00},
  {0x0202, 0x0C},
  {0x0203, 0x4E},
  {0x0204, 0x00},
  {0x0205, 0x00},
  {0x020E, 0x01},
  {0x020F, 0x00},
  {0x0210, 0x01},
  {0x0211, 0x00},
  {0x0212, 0x01},
  {0x0213, 0x00},
  {0x0214, 0x01},
  {0x0215, 0x00},
  {0x7BCD, 0x00},
  {0x94DC, 0x20},
  {0x94DD, 0x20},
  {0x94DE, 0x20},
  {0x95DC, 0x20},
  {0x95DD, 0x20},
  {0x95DE, 0x20},
  {0x7FB0, 0x00},
  {0x9010, 0x3E},
  {0x9419, 0x50},
  {0x941B, 0x50},
  {0x9519, 0x50},
  {0x951B, 0x50},
  {0x3030, 0x00},
  {0x3031, 0x00},
  {0x3032, 0x00},
  {0x0220, 0x00},
  {0x0104, 0x00},
  {0x0100, 0x01}, // stream on
};



/* 3840x2160 Lane Count 2 [0AQH5] Golden settings */
static uint16_t IMX258_REG_3840_LANE2[][2] = {
  {0x0100, 0x00},
  {0x0104, 0x01},
  {0x0136, 0x19},
  {0x0137, 0x00},
  {0x3051, 0x00},
  {0x6B11, 0xCF},
  {0x7FF0, 0x08},
  {0x7FF1, 0x0F},
  {0x7FF2, 0x08},
  {0x7FF3, 0x1B},
  {0x7FF4, 0x23},
  {0x7FF5, 0x60},
  {0x7FF6, 0x00},
  {0x7FF7, 0x01},
  {0x7FF8, 0x00},
  {0x7FF9, 0x78},
  {0x7FFA, 0x01},
  {0x7FFB, 0x00},
  {0x7FFC, 0x00},
  {0x7FFD, 0x00},
  {0x7FFE, 0x00},
  {0x7FFF, 0x03},
  {0x7F76, 0x03},
  {0x7F77, 0xFE},
  {0x7FA8, 0x03},
  {0x7FA9, 0xFE},
  {0x7B24, 0x81},
  {0x7B25, 0x01},
  {0x6564, 0x07},
  {0x6B0D, 0x41},
  {0x653D, 0x04},
  {0x6B05, 0x8C},
  {0x6B06, 0xF9},
  {0x6B08, 0x65},
  {0x6B09, 0xFC},
  {0x6B0A, 0xCF},
  {0x6B0B, 0xD2},
  {0x6700, 0x0E},
  {0x6707, 0x0E},
  {0x5F04, 0x00},
  {0x5F05, 0xED},
  {0x0301, 0x05},
  {0x0303, 0x02},
  {0x0305, 0x04},
  {0x0306, 0x00},
  {0x0307, 0xD0},
  {0x0309, 0x0A},
  {0x030B, 0x01},
  {0x030D, 0x04},
  {0x030E, 0x00},
  {0x030F, 0xA1},
  {0x0310, 0x01},
  {0x0820, 0x07},
  {0x0821, 0xDC},
  {0x0822, 0x80},
  {0x0823, 0x00},
  {0x4648, 0x7F},
  {0x7420, 0x01},
  {0x7421, 0x00},
  {0x7422, 0x03},
  {0x7423, 0x00},
  {0x9104, 0x04},
  {0x0112, 0x0A},
  {0x0113, 0x0A},
  {0x0114, 0x01},
  {0x0342, 0x29},
  {0x0343, 0xE0},
  {0x0340, 0x08},
  {0x0341, 0x90},
  {0x0344, 0x00},
  {0x0345, 0x00},
  {0x0346, 0x01},
  {0x0347, 0xE0},
  {0x0348, 0x10},
  {0x0349, 0x6F},
  {0x034A, 0x0A},
  {0x034B, 0x4F},
  {0x0381, 0x01},
  {0x0383, 0x01},
  {0x0385, 0x01},
  {0x0387, 0x01},
  {0x0900, 0x00},
  {0x0901, 0x11},
  {0x0401, 0x00},
  {0x0404, 0x00},
  {0x0405, 0x10},
  {0x0408, 0x00},
  {0x0409, 0xB8},
  {0x040A, 0x00},
  {0x040B, 0x00},
  {0x040C, 0x0F},
  {0x040D, 0x00},
  {0x040E, 0x08},
  {0x040F, 0x70},
  {0x3038, 0x00},
  {0x303A, 0x00},
  {0x303B, 0x10},
  {0x300D, 0x00},
  {0x034C, 0x0F},
  {0x034D, 0x00},
  {0x034E, 0x08},
  {0x034F, 0x70},
  {0x0600, 0x00},
  {0x0601, 0x00},
#if 1
  {0x0202, 0x08},
  {0x0203, 0x86},
  {0x0204, 0x00},
  {0x0205, 0x00},
#else
  {0x0202, 0x0c},
  {0x0203, 0x39},
  {0x0204, 0x01},
  {0x0205, 0xc0},
#endif
  {0x020E, 0x01},
  {0x020F, 0x00},
  {0x0210, 0x01},
  {0x0211, 0x00},
  {0x0212, 0x01},
  {0x0213, 0x00},
  {0x0214, 0x01},
  {0x0215, 0x00},
  {0x7BCD, 0x00},
  {0x94DC, 0x20},
  {0x94DD, 0x20},
  {0x94DE, 0x20},
  {0x95DC, 0x20},
  {0x95DD, 0x20},
  {0x95DE, 0x20},
  {0x7FB0, 0x00},
  {0x9010, 0x3E},
  {0x9419, 0x50},
  {0x941B, 0x50},
  {0x9519, 0x50},
  {0x951B, 0x50},
  {0x3030, 0x00},
  {0x3031, 0x00},
  {0x3032, 0x00},
  {0x0220, 0x00},
  {0x0104, 0x00},
  {0x0100, 0x01}, // stream on
};

#endif    /* __IMX258_LINEAR_H__ */
