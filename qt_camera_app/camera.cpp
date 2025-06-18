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

#include <QDebug>
#include <QSGGeometryNode>

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
void Camera::textureProcess(const uchar *data, int width, int height)
{
    Q_UNUSED(data);
    Q_UNUSED(width);
    Q_UNUSED(height);
}

void Camera::updateTexture(const uchar *data, int width, int height)
{
    if (!m_running || !data)
        return;

    textureProcess(data, width, height);
    //m_rtp_streaming->sendDataRemote(const_cast<uchar*>(data), (width * height * 3)/2);
#ifndef USE_QUEUE_HOLDER
    m_image->swapImage();
#endif
    emit imageChanged(const_cast<uchar*>(data), (width * height * 3)/2);
}

void Camera::updateMaterial()
{
    uchar* buffer = m_image->getFrontImage();
    m_texture->updateFrame(buffer);
    fillBuffer(buffer);
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
        int x  = str.find("vvcam");

        if(x < 0) {
            qDebug() << "Not the valid driver \n" ;
            continue;
        }


        setDeviceId(id);
        videodev.fd = fd;
        if ((err = subInitCapture()) < 0) {
            qDebug() << "Format not supported.............";
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

    if ((err = ioctl(fd, VIDIOC_REQBUFS, &reqbuf)) < 0) {
        qWarning() << "Cannot allocate memory";
        goto err1;
    }
    videodev.numbuffer = reqbuf.count;
    qWarning() << "buffer actually allocated" << reqbuf.count;

    uint i;
    for (i = 0; i < reqbuf.count; i++) {
        struct v4l2_buffer buf;
        struct v4l2_plane planes[2];
        memset(&buf, 0, sizeof(buf));
        memset(&planes, 0, sizeof planes);
        buf.m.planes = planes;
        buf.type = reqbuf.type;
        buf.index = i;
        buf.memory = reqbuf.memory;
        buf.length = 1;
        err = ioctl(fd, VIDIOC_QUERYBUF, &buf);
        Q_ASSERT(err == 0);

        videodev.buff_info[i].index = i;
#if SUPPORT_V4L2_MPLANE
        videodev.buff_info[i].length = planes[0].length;
        videodev.buff_info[i].start =
            (uchar *)mmap(NULL, planes[0].length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, planes[0].m.mem_offset);

        Q_ASSERT(videodev.buff_info[i].start != MAP_FAILED);
        memset((void *) videodev.buff_info[i].start, 0x80,
                videodev.buff_info[i].length);

#else
        videodev.buff_info[i].length = buf.length;
        videodev.buff_info[i].start =
            (uchar *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        Q_ASSERT(videodev.buff_info[i].start != MAP_FAILED);


        memset((void *) videodev.buff_info[i].start, 0x80,
                videodev.buff_info[i].length);
#endif
        err = ioctl(fd, VIDIOC_QBUF, &buf);
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

    memset(&buf, 0, sizeof(buf));
    memset(&planes, 0, sizeof planes);
#if SUPPORT_V4L2_MPLANE
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.m.planes = planes;
    buf.length = 1;
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

    if (frame_count++ % frame_devisor == 0) {
        updateTexture((uchar *)videodev.buff_info[buf.index].start, videodev.cap_width, videodev.cap_height);
    }

#if 0
    ret = ioctl(videodev.fd, VIDIOC_QBUF, &buf);
    if (ret < 0) {
        qWarning() << "Cap VIDIOC_QBUF";
        return ret;
    }
#endif


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
    struct buf_info *buff_info;

    /* Un-map the buffers */
    for (i = 0; i < CAPTURE_MAX_BUFFER; i++){
        buff_info = &videodev.buff_info[i];
        if (buff_info->start) {
            munmap(buff_info->start, buff_info->length);
            buff_info->start = NULL;
        }
    }

    if (videodev.fd >= 0) {
        close(videodev.fd);
        videodev.fd = -1;
    }
}
void Camera::fillBuffer(void* buff)
{
    int i;
    bool bFound = false;
    struct buf_info *buff_info;


    /* Un-map the buffers */
    for (i = 0; i < CAPTURE_MAX_BUFFER; i++){
        buff_info = &videodev.buff_info[i];
        if (buff_info->start == buff ) {
            bFound = true;
            break;
        }
    }
    if(bFound ==  true) {
        int ret;
        struct v4l2_buffer buf;
        struct v4l2_plane planes[2];

        memset(&buf, 0, sizeof(buf));
        memset(&planes, 0, sizeof planes);
#if SUPPORT_V4L2_MPLANE
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
#else
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#endif
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;
        buf.length = 1;
        buf.m.planes = planes;

        ret = ioctl(videodev.fd, VIDIOC_QBUF, &buf);
        if (ret < 0) {
            qWarning() << "Cap VIDIOC_QBUF %d and str error %s" <<errno << strerror(errno);
        }
    }
}
