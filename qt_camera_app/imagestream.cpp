#include "imagestream.h"
#include <cstring>

#ifdef USE_QUEUE_HOLDER
ImageStream::ImageStream(int width, int height)
    : m_width(width), m_height(height)
{
    empty_buffer = new uchar[width * height * 3];
    memset(empty_buffer, 0, (width * height * 3));
}
#else
ImageStream::ImageStream(int width, int height)
    : front_index(0), updated(false), m_width(width), m_height(height)
{
    for (int i = 0; i < 2; i++)
        data[i] = new uchar[width * height * 3];
}
#endif

ImageStream::~ImageStream()
{
#ifdef USE_QUEUE_HOLDER
    delete empty_buffer;
#else
    for (int i = 0; i < 2; i++)
        delete data[i];
#endif
}
#ifdef USE_QUEUE_HOLDER
NV12Frame* ImageStream::getFrontImage()
{
    NV12Frame* buffer = nullptr;

    mutex.lock();
    if(!m_queue.isEmpty())
        buffer = m_queue.dequeue();
    mutex.unlock();

    return buffer;
}
void ImageStream::pushBackImage(NV12Frame* buffer)
{
    mutex.lock();
    m_queue.enqueue(buffer);
    mutex.unlock();
}

#else
void ImageStream::yuv2rgb(const uchar *yuv, int yw, int yh)
{
    uchar *rgb = getBackImage();
#ifdef YUV_SEMIPLANNER_SHADER
    memcpy(rgb, yuv, (yw * yh * 3)/2);
#else
    uchar* uv = const_cast<uchar*> (yuv) + yw * yh;
    int pos, k =0;
    int uv_pos;

    for (int i = 0; i < yh; ++i) {
        for (int j = 0; j < yw; ++j) {
            pos = yw *i + j;
            uv_pos = (yw * (i >> 1)) + (j & (~ 0x01));
            rgb[k++] = yuv[pos];
            rgb[k++] = uv[uv_pos];
            rgb[k++] = uv[uv_pos +1];
        }
    }
#endif

}

void ImageStream::yuyv2rgb(const uchar *yuv, int yw, int yh)
{
    uchar *rgb = getBackImage();

    for (int i = 0; i < yw * yh; i++) {
        rgb[i * 3 + 0] = yuv[i * 2];
        rgb[i * 3 + 1] = yuv[i * 2 + (i & 1 ? -1 : 1)];
        rgb[i * 3 + 2] = yuv[i * 2 + (i & 1 ? 1 : 3)];
    }
}

void ImageStream::uyvy2rgb(const uchar *yuv, int yw, int yh)
{
    uchar *rgb = getBackImage();

    for (int i = 0; i < yw * yh; i++) {
        rgb[i * 3 + 0] = yuv[i * 2 + 1];
        rgb[i * 3 + 1] = yuv[i * 2 + (i & 1 ? -2 : 0)];
        rgb[i * 3 + 2] = yuv[i * 2 + (i & 1 ? 0 : 2)];
    }
}
void ImageStream::swapImage()
{
    mutex.lock();
    front_index = (front_index + 1) % 2;
    updated = 4;
    mutex.unlock();
}
#endif
