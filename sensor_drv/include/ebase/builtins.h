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
 *   @file builtins.h
 *
 *	This file defines some macros for standard library functions. Usually we
 *	dont link against glibc, so we use the builtins. Standard library function
 *	calls are only permitted in debug mode.
 *
 *****************************************************************************/
#ifndef BUILTINS_H_
#define BUILTINS_H_

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif


#if defined(__GNUC__)
	void uints_menset( void* s, int32_t c, size_t n );
	#define MEMSET(	TARGET, C, LEN)	uints_menset(TARGET, C, LEN)

	void uints_memcpy( void* s1, const void* s2, size_t n);
	#define MEMCPY( DST, SRC, LEN)	uints_memcpy(DST,SRC,LEN)
#else
	#include <string.h>
	#define MEMSET(	TARGET, C, LEN)	memset(TARGET,C,LEN)
	#define MEMCPY( DST, SRC, LEN)	memcpy(DST,SRC,LEN)
#endif

#define WIPEOBJ( TARGET ) MEMSET( &TARGET, 0, sizeof( TARGET ) )

#ifdef __cplusplus
}
#endif

#endif /*BUILTINS_H_*/


