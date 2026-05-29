#ifndef CAMERA_H
#define CAMERA_H

#include <VideoStreamer.h>
#include <linux/videodev2.h>
#include <stddef.h>
#include <stdint.h>
#include <nv12_frame.h>
#include <QtQuick/QSGNode>

#define SUPPORT_V4L2_MPLANE 1
#define MAX_PLANES 2

class ImageStream;
class CameraTexture;

class Camera : public VideoStreamer
{
    Q_OBJECT
    public:
        static Camera* getInstance(QObject *parent = 0);

        void startStream();
        void stopStream();
        virtual unsigned int getDeviceId() {return device_id;}
        virtual void setDeviceId(unsigned int id);
        virtual int getVideoFd() {return videodev.fd;}
        virtual void setWidth(int width) { m_width = width; }
        virtual void setHeight(int height) { m_height = height; }

        QSGNode *createNode();
        void updateGeometry(qreal x, qreal y, qreal width, qreal height);
        void updateMaterial();

    signals:
        void imageChanged(uchar* buffer, int size);
        void deviceIdChanged();

    protected:
        Camera(QObject *parent = nullptr);
        virtual ~Camera();
        ImageStream *m_image;

#ifdef DOLPHIN
        static const int CAPTURE_MAX_BUFFER = 8;
#else
        static const int CAPTURE_MAX_BUFFER = 4;
#endif
        static const int MAX_NUM_OF_V4L2_DEVICES = 32;
        unsigned int device_id;
        unsigned int used_device_id;
        int m_height;
        int m_width;

        struct plane_map {
            void *addr = nullptr;
            size_t len = 0;
        };

        struct buf_info{
            int index;
            unsigned int length;
            void *start;
        };

        struct video_dev
        {
            int fd;
            int y_stride;
            int uv_stride;
            int cap_width, cap_height;
            struct buf_info buff_info[CAPTURE_MAX_BUFFER];
            int numbuffer;
            plane_map mplanes[CAPTURE_MAX_BUFFER][MAX_PLANES];
        } videodev;

        struct plane
        {
            char* buffer;
            int length;
        };

        struct frame_buffer
        {
            int num_planes;
            plane planes[MAX_PLANES];
        };

        void vidioc_enuminput(int fd);
        void run();

    private:
        Camera(const Camera&) = delete;
        Camera& operator=(const Camera&) = delete;

        static Camera* mCamera;
        CameraTexture *m_texture;
        unsigned int frame_count;
        int frame_devisor;
        QMutex m_wait_mutex;
        QWaitCondition m_wait;
        QSGNode *m_node;

        int initCapture();
        int startCapture();
        int captureFrame();
        int stopCapture();
        void closeCapture();
        void fillBuffer(void* buff);

        void updateTexture(const NV12Frame *frame, int width, int height);
        virtual void textureProcess(const NV12Frame *frame, int width, int height);
        virtual int subInitCapture() {return 0;}
};

#endif // CAMERA_H
