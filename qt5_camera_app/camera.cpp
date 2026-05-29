#include "camera.h"
#include "imagestream.h"
#include "camera_texture.h"
#include "camera_texture.h"
#include "yuv2rgb_material.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

#include <stdlib.h>

#include <QDebug>
#include <QSGGeometryNode>

#define PLANE_Y  0
#define PLANE_UV 1

Camera* Camera::mCamera=nullptr;

Camera* Camera::getInstance(QObject *parent)
{
    if(mCamera == nullptr) {
        mCamera = new Camera(parent);
    }

    return mCamera;
}

Camera::Camera(QObject *parent)
    : VideoStreamer(parent), m_image(0), m_texture(0),
      frame_count(0), frame_devisor(1),
      m_node(0)
{
    videodev.fd = -1;
}

Camera::~Camera()
{
    closeCapture();

    if (m_texture)
        delete m_texture;
    if (m_image)
        delete m_image;

}

void Camera::setDeviceId(unsigned int id)
{
    if (device_id != id) {
        device_id = id;
    }
}

QSGGeometryNode *Camera::createNode()
{
    QSGGeometry *geometry;

    if (!m_texture)
        m_texture = new CameraTexture(m_image->getWidth(), m_image->getHeight());

    if (!m_node) {
        m_node = new QSGGeometryNode;
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);
        geometry->setDrawingMode(GL_TRIANGLE_STRIP);
        m_node->setGeometry(geometry);
        m_node->setFlag(QSGNode::OwnsGeometry);

        YUV2RGBMaterial *material = new YUV2RGBMaterial;
        material->setTexture(m_texture);
        m_node->setMaterial(material);
        m_node->setFlag(QSGNode::OwnsMaterial);
    }

    return m_node;
}

void Camera::updateGeometry(qreal x, qreal y, qreal width, qreal height)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    QSGGeometry::TexturedPoint2D *vertices
            = m_node->geometry()->vertexDataAsTexturedPoint2D();

    vertices[0].x = 0;
    vertices[0].y = 0 + height;
    vertices[0].tx = 0;
    vertices[0].ty = 1;

    vertices[1].x = 0;
    vertices[1].y = 0;
    vertices[1].tx = 0;
    vertices[1].ty = 0;

    vertices[2].x = 0 + width;
    vertices[2].y = 0 + height;
    vertices[2].tx = 1;
    vertices[2].ty = 1;

    vertices[3].x = 0 + width;
    vertices[3].y = 0;
    vertices[3].tx = 1;
    vertices[3].ty = 0;
}

void Camera::startStream()
{
    m_running = true;
    m_wait.wakeAll();
}

void Camera::stopStream()
{
    m_running = false;
    msleep(300);
}

void Camera::run()
{
    QThread::currentThread()->setPriority(QThread::HighPriority);
    while (1) {
        m_wait_mutex.lock();
        while (m_running == false)
            m_wait.wait(&m_wait_mutex);

        m_wait_mutex.unlock();

        if (initCapture() < 0)
            return;

        if (startCapture() < 0)
            return;

        while (m_running == true) {
            if (captureFrame() < 0)
                break;
        }
        stopCapture();
        closeCapture();
    }
}
void Camera::textureProcess(const NV12Frame *data, int width, int height)
{
    Q_UNUSED(data);
    Q_UNUSED(width);
    Q_UNUSED(height);
}

void Camera::updateTexture(const NV12Frame *frame, int width, int height)
{
    if (!m_running || !frame)
        return;

    textureProcess(frame, width, height);
    //m_rtp_streaming->sendDataRemote(const_cast<uchar*>(data), (width * height * 3)/2);
#ifndef USE_QUEUE_HOLDER
    m_image->swapImage();
#endif
    emit imageChanged(reinterpret_cast<uchar*>(const_cast<NV12Frame*>(frame)), 0);
}

void Camera::updateMaterial()
{
    NV12Frame *f = m_image->getFrontImage(); //todo: return type should be NV12Frame
    if (!f)
        return;

    m_texture->updateFrame(f);
    fillBuffer(f);
}

void Camera::vidioc_enuminput(int fd)
{
    int err;
    struct v4l2_input input;
    memset(&input, 0, sizeof(input));
    input.index = 0;
    while ((err = ioctl(fd, VIDIOC_ENUMINPUT, &input)) == 0) {
        qWarning() << "input name =" << (char *)input.name
            << " type =" << input.type
            << " status =" << input.status
            << " std =" << input.std;
        input.index++;
    }
}

int Camera::initCapture()
{
    if (videodev.fd > 0)
        return 0;

    int err;
    char device_dev[200];
    int fd = -1;
    struct v4l2_capability cap;
    unsigned int  capabilities = V4L2_CAP_VIDEO_CAPTURE;
    unsigned int id;

    for (id = 0; id < 32; id++) {
        sprintf(device_dev, "/dev/video%d", id);
        if((fd = open(device_dev, O_RDWR)) < 0)
            continue;


        if ((err = ioctl(fd, VIDIOC_QUERYCAP, &cap)) < 0) {
            qWarning() << "VIDIOC_QUERYCAP error " << err;
            continue;
        }

#if SUPPORT_V4L2_MPLANE
        capabilities = V4L2_CAP_VIDEO_CAPTURE_MPLANE;
#endif
        if ((cap.capabilities & capabilities) && (cap.capabilities & V4L2_CAP_STREAMING))
            qDebug() << "/dev/video" << id << ": Capable of capture and streaming";
        else {
            qDebug() << "/dev/video" << id << ": not Capable of capture or streaming";
            continue;
        }

        //Is this a vvcam driver,
        std::string str((char*)cap.driver);
        bool vvcam_driver  = (str.find("vvcam") != std::string::npos);
        bool camera_video_driver  = (str.find("camera-video") != std::string::npos);
        bool is_valid_driver = (vvcam_driver || camera_video_driver);

        if (!is_valid_driver) {
            qDebug() << "Not the valid driver \n" ;
            continue;
        }


        setDeviceId(id);
        videodev.fd = fd;
        if ((err = subInitCapture()) < 0) {
            qDebug() << "Format not supported.";
            continue;
        }
        break;
    }

    qWarning() << "card =" << (char *)cap.card
        << " driver =" << (char *)cap.driver
        << " bus =" << (char *)cap.bus_info;

    struct v4l2_requestbuffers reqbuf;
    reqbuf.count = CAPTURE_MAX_BUFFER;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    reqbuf.memory = V4L2_MEMORY_MMAP;

    if ((err = ioctl(videodev.fd, VIDIOC_REQBUFS, &reqbuf)) < 0) {
        qWarning() << "Cannot allocate memory, errno=" << errno << strerror(errno);
        goto err1;
    }
    videodev.numbuffer = reqbuf.count;
    qWarning() << "buffer actually allocated" << reqbuf.count;
    if (videodev.numbuffer > CAPTURE_MAX_BUFFER)
        videodev.numbuffer = CAPTURE_MAX_BUFFER;

    uint i;
    for (i = 0; i < (uint)videodev.numbuffer; i++) {
        struct v4l2_buffer buf;
        struct v4l2_plane planes[2];
        memset(&buf, 0, sizeof(buf));
        memset(&planes, 0, sizeof planes);
        buf.m.planes = planes;
        buf.type = reqbuf.type;
        buf.index = i;
        buf.memory = reqbuf.memory;
        buf.length = MAX_PLANES;

        err = ioctl(videodev.fd, VIDIOC_QUERYBUF, &buf);
        Q_ASSERT(err == 0);

        videodev.mplanes[i][PLANE_Y].len  = planes[PLANE_Y].length;
        videodev.mplanes[i][PLANE_UV].len  = planes[PLANE_UV].length;
        videodev.mplanes[i][PLANE_Y].addr =
            mmap(NULL, planes[PLANE_Y].length, PROT_READ | PROT_WRITE, MAP_SHARED, videodev.fd, planes[PLANE_Y].m.mem_offset);
        videodev.mplanes[i][PLANE_UV].addr =
            mmap(NULL, planes[PLANE_UV].length, PROT_READ | PROT_WRITE, MAP_SHARED, videodev.fd, planes[PLANE_UV].m.mem_offset);

        if (videodev.mplanes[i][PLANE_Y].addr == MAP_FAILED ||
            videodev.mplanes[i][PLANE_UV].addr == MAP_FAILED) {
                qWarning() << "mmap failed for planes index=" << i
                       << " y_off=" << planes[PLANE_Y].m.mem_offset
                       << " uv_off=" << planes[PLANE_UV].m.mem_offset
                       << " errno=" << errno << strerror(errno);

            if (videodev.mplanes[i][PLANE_Y].addr && videodev.mplanes[i][PLANE_Y].addr != MAP_FAILED)
                munmap(videodev.mplanes[i][PLANE_Y].addr, videodev.mplanes[i][PLANE_Y].len);
            if (videodev.mplanes[i][PLANE_UV].addr && videodev.mplanes[i][PLANE_UV].addr != MAP_FAILED)
                munmap(videodev.mplanes[i][PLANE_UV].addr, videodev.mplanes[i][PLANE_UV].len);
			err = -errno;
            goto err1;
        }

		buf.m.planes[PLANE_Y].bytesused = videodev.cap_width * videodev.cap_height;
        buf.m.planes[PLANE_UV].bytesused = (videodev.cap_width * videodev.cap_height) / 2;
        err = ioctl(videodev.fd, VIDIOC_QBUF, &buf);
        Q_ASSERT(err == 0);
    }

    return 0;

err1:
    close(fd);
    return err;
}

int Camera::startCapture()
{
    int a, ret;

    /* Start Streaming. on capture device */
#if SUPPORT_V4L2_MPLANE
    a = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
#else
    a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#endif
    ret = ioctl(videodev.fd, VIDIOC_STREAMON, &a);
    if (ret < 0) {
        qWarning() << "capture VIDIOC_STREAMON error fd=" << videodev.fd;
        return ret;
    }

    return 0;
}

int Camera::captureFrame()
{
    int ret;
    struct v4l2_buffer buf;
    struct v4l2_plane planes[2];
    NV12Frame *frame = nullptr;
    const uint8_t *y = nullptr, *uv = nullptr;
    size_t y_len = 0, uv_len = 0;

    memset(&buf, 0, sizeof(buf));
    memset(&planes, 0, sizeof planes);
#if SUPPORT_V4L2_MPLANE
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.m.planes = planes;
    buf.length = MAX_PLANES;
#else
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#endif
    buf.memory = V4L2_MEMORY_MMAP;

    /* Dequeue capture buffer */
    //Buf index  //gettimeofday ==> convert DD::MM:YY , mmsecond
    ret = ioctl(videodev.fd, VIDIOC_DQBUF, &buf);
    if (ret < 0) {
        qWarning() << "VIDIOC_DQBUF error";
        return ret;
    }

    y      = static_cast<const uint8_t*>(videodev.mplanes[buf.index][PLANE_Y].addr);
    uv     = static_cast<const uint8_t*>(videodev.mplanes[buf.index][PLANE_UV].addr);
    y_len  = videodev.mplanes[buf.index][PLANE_Y].len;
    uv_len = videodev.mplanes[buf.index][PLANE_UV].len;
    frame = new NV12Frame{ y, y_len, uv, uv_len, (int)buf.index,
                videodev.y_stride ? videodev.y_stride : videodev.cap_width,
                videodev.uv_stride ? videodev.uv_stride : videodev.cap_width
    };

    if (frame) {
        if (frame_count++ % frame_devisor == 0) {
            updateTexture(frame, videodev.cap_width, videodev.cap_height);
        }
    }

    return 0;
}

int Camera::stopCapture()
{
    int a, ret;

#if SUPPORT_V4L2_MPLANE
    a = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
#else
    a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#endif
    ret = ioctl(videodev.fd, VIDIOC_STREAMOFF, &a);
    if (ret < 0) {
        qWarning() << "VIDIOC_STREAMOFF";
        return ret;
    }

    return 0;
}

void Camera::closeCapture()
{
    int i;
    int p;

    for (i = 0; i < CAPTURE_MAX_BUFFER; i++){
        for (p = 0; p < MAX_PLANES; ++p) {
            void*  addr = videodev.mplanes[i][p].addr;
            size_t len  = videodev.mplanes[i][p].len;
            if (addr && addr != MAP_FAILED && len) {
                munmap(addr, len);
            }
            videodev.mplanes[i][p].addr = nullptr;
            videodev.mplanes[i][p].len  = 0;
        }
    }

    if (videodev.fd >= 0) {
        close(videodev.fd);
        videodev.fd = -1;
    }
}

void Camera::fillBuffer(void* buff)
{
    int ret;
    struct v4l2_buffer vbuf;
    struct v4l2_plane planes[2];
    NV12Frame* f = static_cast<NV12Frame*>(buff);

    if (!f) return;

    memset(&vbuf, 0, sizeof(vbuf));
    memset(&planes, 0, sizeof(planes));

    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    vbuf.m.planes = planes;
    vbuf.length   = MAX_PLANES;

    planes[0].bytesused = (unsigned)(videodev.cap_width * videodev.cap_height);
    planes[1].bytesused = (unsigned)((videodev.cap_width * videodev.cap_height)/2);
    vbuf.memory = V4L2_MEMORY_MMAP;
    vbuf.index  = f->index;

    ret = ioctl(videodev.fd, VIDIOC_QBUF, &vbuf);
    if (ret < 0) {
        qWarning() << "Cap VIDIOC_QBUF errno=" << errno << strerror(errno);
    }

    delete f;
}
