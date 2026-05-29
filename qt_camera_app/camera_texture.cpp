#include "camera_texture.h"
#include <QByteArray>
#include <QDebug>
#include <cstring>

#ifdef YUV_SEMIPLANNER_SHADER
CameraTexture::CameraTexture(int width, int height)
    : mWidth(width), mHeight(height)
{
    for (int i = 0 ; i < MAX_TEXTURE; ++i) {
        glGenTextures(1, &mTexture[i]);
        glBindTexture(GL_TEXTURE_2D, mTexture[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        if (i == 0) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mWidth, mHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, mWidth/2, mHeight/2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, 0);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

    }
}

CameraTexture::~CameraTexture()
{
    for (int i = 0 ; i < MAX_TEXTURE; ++i) {
        glDeleteTextures(1, &mTexture[i]);
    }
}

void CameraTexture::updateFrame(const NV12Frame *frame)
{
    if (!frame || !frame->y || !frame->uv) return;

    const int W = mWidth;
    const int H = mHeight;

    const int yStrideBytes  = frame->y_stride  > 0 ? frame->y_stride  : W;
    const int uvStrideBytes = frame->uv_stride > 0 ? frame->uv_stride : W;

    if ((uvStrideBytes & 1) != 0) return;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glBindTexture(GL_TEXTURE_2D, mTexture[1]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, uvStrideBytes / 2);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    W/2, H/2, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, frame->uv);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindTexture(GL_TEXTURE_2D, mTexture[0]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, yStrideBytes);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    W, H, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->y);
    glBindTexture(GL_TEXTURE_2D, 0);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}
#else
CameraTexture::CameraTexture(int width, int height)
    : mWidth(width), mHeight(height)
{
    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

CameraTexture::~CameraTexture()
{
    glDeleteTextures(1, &mTexture);
}

void CameraTexture::updateFrame(void *data)
{
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mWidth, mHeight, GL_RGB, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}
#endif
