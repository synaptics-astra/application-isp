#include "camera_texture.h"

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

void CameraTexture::updateFrame(void *data)
{
    unsigned char* uv = (unsigned char*) data + (mWidth * mHeight);
    //Set UV data
    glBindTexture(GL_TEXTURE_2D, mTexture[1]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mWidth/2, mHeight/2, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, uv);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    //Set Y data
    glBindTexture(GL_TEXTURE_2D, mTexture[0]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mWidth, mHeight, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
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
