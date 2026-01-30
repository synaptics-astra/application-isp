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

#ifdef DOLPHIN
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/media.h>

#define MEDIA_DEV_MAX       8

extern int mcmWrFrameDebugEnable;
void help(void)
{
    printf("Usage:\n");
    printf("\t./isp_media_server -d\n");
    printf("\t-d  enable MCM frame dump\n");
    return;
}

int findMediaDevId()
{
    int MediaDevFd;
    int ret;
    struct media_device_info info;
    char MediaDevName[32];

    for (int i = 0; i < MEDIA_DEV_MAX; i++) {
        sprintf(MediaDevName, "/dev/media%d", i);

        MediaDevFd = open(MediaDevName, O_RDWR);
        MediaDevFd = open(MediaDevName, O_RDWR);
        if (MediaDevFd < 0) {
            printf("open %s error", MediaDevName);
            return -1;
        }

        memset(&info, 0, sizeof(info));
        ret = ioctl(MediaDevFd, MEDIA_IOC_DEVICE_INFO, &info);
        if (ret) {
            printf("Get media device info error %s!", strerror(-ret));
            close(MediaDevFd);
            return -1;
        }
        printf("media driver: %s\n",info.driver);
        printf("media model: %s\n",info.model);

        if (strcmp(info.driver, "vvcam-video") == 0) {
            close(MediaDevFd);
            printf("vvcam media Dev ID: %d\n", i);
            return i;
        }
        close(MediaDevFd);
    }

    return 0;
}
#endif

int main(int argc, char **argv)
{
    int RetVal = VSI_SUCCESS;
    DevId = 0;

    printf("**************************************************************");
    printf("ISP MediaControl Framework V%s (%s, %s)",
                    MEDIA_SERVER_VERSION, __DATE__, __TIME__);
    printf("**************************************************************");

#ifdef DOLPHIN
    setvbuf(stdout, NULL, _IOLBF, 0);   // line-buffered stdout
    setvbuf(stderr, NULL, _IONBF, 0);   // unbuffered stderr

    int Ch;
    while((Ch = getopt(argc, argv, "d")) != -1) {
        switch(Ch) {
            case 'd':
                printf("\nMCM WR frame dump enabled\n");
                mcmWrFrameDebugEnable = 1;
                break;
            default:
                help();
        }
    }
#endif

    struct sigaction SigAct;
    SigAct.sa_handler = SignalHandler;
    SigAct.sa_flags   = 0;
    sigemptyset(&SigAct.sa_mask);

    if (sigaction(SIGINT, &SigAct, NULL) == -1 ||
        sigaction(SIGTERM, &SigAct, NULL) == -1) {
        printf("Registering sigaction failed");
        return -1;
    }

    DevId = findMediaDevId();
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
