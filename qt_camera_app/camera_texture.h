#ifndef CAMERA_TEXTURE_H
#define CAMERA_TEXTURE_H

#include <QSGTexture>
#include <QOpenGLTexture>
#include "nv12_frame.h"

#define MAX_TEXTURE 2

class CameraTexture : public QSGTexture
{
    public:
        CameraTexture(int width, int height);
        virtual ~CameraTexture();
#ifdef YUV_SEMIPLANNER_SHADER
        virtual void bind() {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, mTexture[1]);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mTexture[0]);
        }
        virtual int textureId() const { return mTexture[0]; }
#else
        virtual void bind() { glBindTexture(GL_TEXTURE_2D, mTexture); }
        virtual int textureId() const { return mTexture; }
#endif
        virtual bool hasAlphaChannel() const { return false; }
        virtual bool hasMipmaps() const { return false; }
        virtual QSize textureSize() const { return QSize(mWidth, mHeight); }

        void updateFrame(const NV12Frame *frame);

    private:
        int mWidth;
        int mHeight;
#ifdef YUV_SEMIPLANNER_SHADER
        GLuint mTexture[MAX_TEXTURE];
#else
        GLuint mTexture;
#endif
};

#endif // CAMERA_TEXTURE_H
