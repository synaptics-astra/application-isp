#ifndef NV12_FRAME_H
#define NV12_FRAME_H

struct NV12Frame {
    const uint8_t* y;
    size_t         y_len;
    const uint8_t* uv;
    size_t         uv_len;
    int            index;
    int            y_stride;
    int            uv_stride;
};
#endif
