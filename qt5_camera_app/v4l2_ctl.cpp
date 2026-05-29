#include "v4l2_ctl.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define V4L2_CTRL_CLASS_USER 0x00980000
#define V4L2_CID_AWB_ROI 0x00982a07
#define V4L2_CID_AWB_ROI_WEIGHT 0x00982a06
#define V4L2_CID_AE_ROI 0x0098291b
#define V4L2_CID_ROI_AE_WEIGHT 0x0098291a

V4L2Ctl::V4L2Ctl() {}

V4L2Ctl::~V4L2Ctl() {}

int V4L2Ctl::setAwbRoi(int fd, uint16_t roi[25][4]) {
    struct v4l2_ext_controls ctrls;
    struct v4l2_ext_control ctrl;
    ctrl.id = V4L2_CID_AWB_ROI;
    ctrl.p_u16 = &roi[0][0];
    ctrl.size = 25 * 4 * sizeof(uint16_t);

    ctrls.ctrl_class = V4L2_CTRL_CLASS_USER;
    ctrls.controls = &ctrl;
    ctrls.count = 1;

    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &ctrls) == -1) {
        perror("Failed to set AWB ROI");
        return -1;
    }
    return 0;
}

int V4L2Ctl::setAwbRoiWeight(int fd, uint32_t weights[25]) {
    struct v4l2_ext_controls ctrls;
    struct v4l2_ext_control ctrl;
    ctrl.id = V4L2_CID_AWB_ROI_WEIGHT;
    ctrl.p_u32 = &weights[0];
    ctrl.size = 25 * sizeof(uint32_t);

    ctrls.ctrl_class = V4L2_CTRL_CLASS_USER;
    ctrls.controls = &ctrl;
    ctrls.count = 1;

    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &ctrls) == -1) {
        perror("Failed to set AWB ROI Weight");
        return -1;
    }
    return 0;
}

int V4L2Ctl::setAeRoi(int fd, uint16_t roi[25][4]) {
    struct v4l2_ext_controls ctrls;
    struct v4l2_ext_control ctrl;
    ctrl.id = V4L2_CID_AE_ROI;
    ctrl.p_u16 = &roi[0][0];
    ctrl.size = 25 * 4 * sizeof(uint16_t);

    ctrls.ctrl_class = V4L2_CTRL_CLASS_USER;
    ctrls.controls = &ctrl;
    ctrls.count = 1;

    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &ctrls) == -1) {
        perror("Failed to set AWB ROI");
        return -1;
    }
    return 0;
}

int V4L2Ctl::setAeRoiWeight(int fd, uint32_t weights[25]) {
    struct v4l2_ext_controls ctrls;
    struct v4l2_ext_control ctrl;
    ctrl.id = V4L2_CID_ROI_AE_WEIGHT;
    ctrl.p_u32 = &weights[0];
    ctrl.size = 25 * sizeof(uint32_t);

    ctrls.ctrl_class = V4L2_CTRL_CLASS_USER;
    ctrls.controls = &ctrl;
    ctrls.count = 1;

    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &ctrls) == -1) {
        perror("Failed to set AWB ROI Weight");
        return -1;
    }
    return 0;
}
