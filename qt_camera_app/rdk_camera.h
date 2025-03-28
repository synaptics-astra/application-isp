#ifndef RDK_CAMERA_H
#define RDK_CAMERA_H

#include <QProcess>
#include "camera.h"

class RDKCamera : public Camera
{
    public:
        RDKCamera(QObject *parent = 0);
        virtual void setWidth(int width);
        virtual void setHeight(int height);

    private:
        int m_width;
        int m_height;
        void setV4l2Immu(int device_id, bool value);
        virtual int subInitCapture();
        virtual void textureProcess(const uchar *data, int width, int height);
};

#endif
