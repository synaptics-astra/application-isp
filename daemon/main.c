/******************************************************************************\
|* Copyright 2010, Dream Chip Technologies GmbH. used with permission by      *|
|* VeriSilicon.                                                               *|
|* Copyright (c) <2020> by VeriSilicon Holdings Co., Ltd. ("VeriSilicon")     *|
|* All Rights Reserved.                                                       *|
|*                                                                            *|
|* The material in this file is confidential and contains trade secrets of    *|
|* of VeriSilicon.  This is proprietary information owned or licensed by      *|
|* VeriSilicon.  No part of this work may be disclosed, reproduced, copied,   *|
|* transmitted, or used in any way for any purpose, without the express       *|
|* written permission of VeriSilicon.                                         *|
|*                                                                            *|
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define MEDIA_SERVER_VERSION "4.5.x"
#define VSI_SUCCESS 0

static int DevId;
static void SignalHandler(int Signum);

extern int MediaDeviceCreate(int Dev);
extern int MediaDeviceRelease(int Dev);

int main(int argc, char **argv)
{
    int RetVal = VSI_SUCCESS;
    DevId = 0;

    printf("**************************************************************");
    printf("ISP MediaControl Framework V%s (%s, %s)",
                    MEDIA_SERVER_VERSION, __DATE__, __TIME__);
    printf("**************************************************************");

    struct sigaction SigAct;
    SigAct.sa_handler = SignalHandler;
    SigAct.sa_flags   = 0;
    sigemptyset(&SigAct.sa_mask);

    if (sigaction(SIGINT, &SigAct, NULL) == -1 ||
        sigaction(SIGTERM, &SigAct, NULL) == -1) {
        printf("Registering sigaction failed");
        return -1;
    }

    RetVal = MediaDeviceCreate(DevId);
    if (RetVal != VSI_SUCCESS) {
        printf("Create media device %d error %d", DevId, RetVal);
        return -1;
    }

    while(1) {
        usleep(100);
    }

    return 0;
}

static void SignalHandler(int Signum)
{
    int RetVal = VSI_SUCCESS;

    RetVal = MediaDeviceRelease(DevId);
    if (RetVal != VSI_SUCCESS) {
        printf("Release media device %d error %d", DevId, RetVal);
    }

    exit(0);
}
