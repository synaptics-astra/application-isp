#include "roi_control.h"
#include "v4l2_ctl.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

RoiControl::RoiControl(QObject *parent) : QObject(parent){}

RoiControl::~RoiControl() {
}

void RoiControl::setAwbRoi(int deviceFd, int index, int x, int y, int width, int height) {
    V4L2Ctl v4l2Ctl;
    awbRoi[index][0] = static_cast<uint16_t>(x);
    awbRoi[index][1] = static_cast<uint16_t>(y);
    awbRoi[index][2] = static_cast<uint16_t>(width);
    awbRoi[index][3] = static_cast<uint16_t>(height);
    v4l2Ctl.setAwbRoi(deviceFd, awbRoi);
}

void RoiControl::setAwbRoiWeight(int deviceFd, int index, int weight) {
    V4L2Ctl v4l2Ctl;
    awbWeights[index] = static_cast<uint32_t>(weight);
    v4l2Ctl.setAwbRoiWeight(deviceFd, awbWeights);
}

void RoiControl::setAeRoi(int deviceFd, int index, int x, int y, int width, int height) {
    V4L2Ctl v4l2Ctl;
    aeRoi[index][0] = static_cast<uint16_t>(x);
    aeRoi[index][1] = static_cast<uint16_t>(y);
    aeRoi[index][2] = static_cast<uint16_t>(width);
    aeRoi[index][3] = static_cast<uint16_t>(height);
    v4l2Ctl.setAeRoi(deviceFd, aeRoi);
}

void RoiControl::setAeRoiWeight(int deviceFd, int index, int weight) {
    V4L2Ctl v4l2Ctl;
    aeWeights[index] = static_cast<uint32_t>(weight);
    v4l2Ctl.setAeRoiWeight(deviceFd, aeWeights);
}

