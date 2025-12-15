#ifndef IMAGESTREAM_HPP
#define IMAGESTREAM_HPP

#include <QMutex>
#include "nv12_frame.h"
#ifdef USE_QUEUE_HOLDER
#include <QQueue>
#endif

class ImageStream
{
    public:
        ImageStream(int width, int height);
        ~ImageStream();
#ifdef USE_QUEUE_HOLDER
        NV12Frame* getFrontImage();
        void pushBackImage(NV12Frame* f);
#else
        uchar *getFrontImage() { return data[front_index]; }
        uchar *getBackImage() { return data[(front_index + 1) % 2]; }
        void swapImage();
        bool isUpdated() { return updated; }
        void decUpdated() { updated--; }
        void yuyv2rgb(const uchar *yuv, int yw, int yh);
        void uyvy2rgb(const uchar *yuv, int yw, int yh);
#endif
        void lockFrontImage() { mutex.lock(); }
        void unlockFrontImage() { mutex.unlock(); }
        int getWidth() { return m_width; }
        int getHeight() { return m_height; }

        void yuv2rgb(const uchar *yuv, int yw, int yh);

    private:
#ifdef USE_QUEUE_HOLDER
        QQueue<NV12Frame*> m_queue;
        uchar *empty_buffer;
#else
        int updated;
        uchar *data[2];
        int front_index;
#endif
        int m_width;
        int m_height;
        QMutex mutex;
};

#endif // IMAGESTREAM_HPP
