#ifndef V4L2_CTL_H
#define V4L2_CTL_H

#include <stdint.h>

class V4L2Ctl {
public:
    V4L2Ctl();
    ~V4L2Ctl();

    int setAwbRoi(int fd, uint16_t roi[25][4]);
    int setAwbRoiWeight(int fd, uint32_t weights[25]);

    int setAeRoi(int fd, uint16_t roi[25][4]);
    int setAeRoiWeight(int fd, uint32_t weights[25]);
};

#endif // V4L2_CTL_H
