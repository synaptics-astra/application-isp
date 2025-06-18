#ifndef CAMERA_H
#define CAMERA_H

#include <VideoStreamer.h>
#include <linux/videodev2.h>
#define SUPPORT_V4L2_MPLANE 1
#define MAX_PLANES 2

class ImageStream;
class CameraTexture;
class QSGGeometryNode;

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

        QSGGeometryNode *createNode();
        void updateGeometry(qreal x, qreal y, qreal width, qreal height);
        void updateMaterial();

    signals:
        void imageChanged(uchar* buffer, int size);
        void deviceIdChanged();

    protected:
        Camera(QObject *parent = nullptr);
        virtual ~Camera();
        ImageStream *m_image;

        static const int CAPTURE_MAX_BUFFER = 8;
        static const int MAX_NUM_OF_V4L2_DEVICES = 32;
        unsigned int device_id;
        unsigned int used_device_id;
        int m_height;
        int m_width;

        struct buf_info{
            int index;
            unsigned int length;
            void *start;
        };

        struct video_dev
        {
            int fd;
            int cap_width, cap_height;
            struct buf_info buff_info[CAPTURE_MAX_BUFFER];
            int numbuffer;
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
        QSGGeometryNode *m_node;

        int initCapture();
        int startCapture();
        int captureFrame();
        int stopCapture();
        void closeCapture();
        void fillBuffer(void* buff);

        void updateTexture(const uchar *data, int width, int height);
        virtual void textureProcess(const uchar *data, int width, int height);
        virtual int subInitCapture() {return 0;}
};

#endif // CAMERA_H
