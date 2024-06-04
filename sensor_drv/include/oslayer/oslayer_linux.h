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

/* VeriSilicon 2020 */

/**
 * Module    : Operating System Abstraction Layer
 *
 * Hierarchy :
 *
 * Purpose   : Encapsulates and abstracts services from different operating
 *             system, including user-mode as well as kernel-mode services.
 ******************************************************************************/
#ifdef LINUX

#ifndef __KERNEL__
#undef _GNU_SOURCE
#define _GNU_SOURCE        // required for CPU affinity stuff in <sched.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/time.h>
#include <errno.h>
#else
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/jiffies.h>
#include <linux/completion.h>
#include <asm/processor.h>
#include <asm/semaphore.h>
#endif

#include <ebase/types.h>

#ifdef __cplusplus
extern "C"
{
#endif



#ifdef __KERNEL__

#define OSLAYER_KERNEL

/*****************************************************************************/
/*  @brief Definitions for advanced memory allocation (kernel only) */
#define OSLAYER_ATOMIC  GFP_ATOMIC   /*< caller will never sleep, can be called from IRQ context */
#define OSLAYER_KERNEL  GFP_KERNEL   /*< Allocate kernel RAM. Function can sleep if memory is not available */
#define OSLAYER_USER    GFP_USER     /*< Allocate memory on behalf of user. Attached driver instance can */
                                     /*< sleep if memory is not available */
#endif /*  __KERNEL__ */


typedef int32_t (*osThreadFunc)(void *);
typedef int32_t (*osIsrFunc)(void *);
typedef int32_t (*osDpcFunc)(void *);


#ifdef OSLAYER_EVENT
/*****************************************************************************/
/*  @brief  Event object (Linux Version) of OS Abstraction Layer */
typedef struct _osEvent
{
#ifndef OSLAYER_KERNEL
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int32_t automatic;
    int32_t state;
#else
    struct completion x;
#endif
} osEvent;
#endif /* OSLAYER_EVENT */


#ifdef OSLAYER_MUTEX
/*****************************************************************************/
/*  @brief  Mutex object (Linux Version) of OS Abstraction Layer */
typedef struct _osMutex
{
#ifndef OSLAYER_KERNEL
    pthread_mutex_t handle;
#else
    struct semaphore *sem;
#endif
} osMutex;
#endif /* OSLAYER_MUTEX */


#ifdef OSLAYER_SEMAPHORE
/*****************************************************************************/
/*  @brief  Semaphore object (Linux Version) of OS Abstraction Layer */
typedef struct _osSemaphore
{
#ifndef OSLAYER_KERNEL
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int32_t count;
#else
    struct semaphore *sem;
#endif
} osSemaphore;
#endif /* OSLAYER_SEMAPHORE */


#ifdef OSLAYER_THREAD
/*****************************************************************************/
/*  @brief  Thread object (Linux Version) of OS Abstraction Layer */
typedef struct _osThread
{
#ifndef OSLAYER_KERNEL
    pthread_t handle;
    osMutex access_mut;
    int32_t wait_count;
#else
    int32_t handle;
#endif
    osEvent exit_event;
    int32_t (*pThreadFunc)(void *);
    void *p_arg;
} osThread;
#endif /* OSLAYER_THREAD */


#ifdef OSLAYER_MISC
#ifdef OSLAYER_KERNEL
/*****************************************************************************/
/*  @brief  Spin Lock object (Linux Kernel Version only) of OS Abstraction */
/*          Layer */
typedef struct _osSpinLock
{
    spinlock_t lock;
} osSpinLock;
#endif /* OSLAYER_KERNEL */
#endif /* OSLAYER_MISC */



#ifdef __cplusplus
}
#endif



#endif /* LINUX */
