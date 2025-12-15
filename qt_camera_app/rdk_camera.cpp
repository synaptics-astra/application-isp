#include "rdk_camera.h"
#include "imagestream.h"
#include <QDebug>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

#define SUPPORT_NV12_STREAMING 1
#define CAMERA_WIDTH		1920
#define CAMERA_HEIGHT		1080

RDKCamera::RDKCamera(QObject *parent)
    : Camera(parent), m_width(CAMERA_WIDTH), m_height (CAMERA_HEIGHT)
{
    m_image = new ImageStream(m_width, m_height);
}
void RDKCamera::setHeight(int height)
{
    if (m_image != NULL  && m_height != height ) {
        m_height  = height;
        delete m_image;
        m_image = new ImageStream(m_width, m_height);
    }
}
void RDKCamera::setWidth(int width)
{
    if (m_image != NULL  && m_width != width ) {
        m_width  = width;
        delete m_image;
        m_image = new ImageStream(m_width, m_height);
    }
}

void RDKCamera::setV4l2Immu(int device_id, bool value)
{
    QProcess process;
    QString command = "v4l2-ctl";
    QStringList arguments;
    arguments << "-d" << QString("/dev/video%1").arg(device_id) << "-c"
        << QString("mmu_enable=%1").arg(value);
    process.start(command, arguments);
    process.waitForFinished();
}

int RDKCamera::subInitCapture()
{
    int device_id = getDeviceId();
    setV4l2Immu(device_id, 1);

    int err, fd = videodev.fd;

    bool support_fmt;
    struct v4l2_fmtdesc ffmt;
    memset(&ffmt, 0, sizeof(ffmt));
    ffmt.index = 0;
    __u32 type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    __u32 pixelformat = V4L2_PIX_FMT_NV12M;

    ffmt.type = type;
    support_fmt = false;
    qWarning()<< "Required " << pixelformat;
    while ((err = ioctl(fd, VIDIOC_ENUM_FMT, &ffmt)) == 0) {
        qWarning()<< "fmt" << ffmt.pixelformat << (char *)ffmt.description;
#ifdef SUPPORT_NV12_STREAMING
        if (ffmt.pixelformat == pixelformat) {
#else
            if (ffmt.pixelformat == V4L2_PIX_FMT_YUYV) {
#endif
                support_fmt = true;
                break;
            }
            ffmt.index++;
        }
        if (!support_fmt) {
#ifdef SUPPORT_NV12_STREAMING
            qWarning() << "V4L2_PIX_FMT_NV12M is not supported by this camera";
#else
            qWarning() << "V4L2_PIX_FMT_YUYV is not supported by this camera";
#endif
            return -1;
        }

        int index;
        if ((err = ioctl(fd, VIDIOC_G_INPUT, &index)) < 0)
            qWarning() << "VIDIOC_G_INPUT fail" <<  errno;
        else
            qDebug() << "current input index =" << index;

        struct v4l2_format fmt;
        memset(&fmt, 0, sizeof(fmt));
        fmt.type = type;
        if ((err = ioctl(fd, VIDIOC_G_FMT, &fmt)) < 0) {
            qWarning() << "VIDIOC_G_FMT fail" << errno;
        } else {
            qWarning() << "fmt width =" << fmt.fmt.pix_mp.width
                << " height =" << fmt.fmt.pix_mp.height
                << " pfmt =" << fmt.fmt.pix_mp.pixelformat;
        }

        fmt.type = type;
#if SUPPORT_V4L2_MPLANE
        fmt.fmt.pix_mp.width = m_width;
        fmt.fmt.pix_mp.height = m_height;
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12M;
        fmt.fmt.pix_mp.num_planes = MAX_PLANES;

        fmt.fmt.pix_mp.plane_fmt[0].bytesperline = m_width;
        fmt.fmt.pix_mp.plane_fmt[0].sizeimage = (m_width * m_height);
        fmt.fmt.pix_mp.plane_fmt[1].bytesperline = m_width;
        fmt.fmt.pix_mp.plane_fmt[1].sizeimage = (m_width * m_height) / 2;
#else
        fmt.fmt.pix.width = m_width;
        fmt.fmt.pix.height = m_height;
#endif

#ifdef SUPPORT_NV12_STREAMING
        fmt.fmt.pix_mp.pixelformat = pixelformat;
#else
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
#endif
        if ((err = ioctl(fd, VIDIOC_S_FMT, &fmt)) < 0)
            qWarning() << "VIDIOC_S_FMT fail" << errno;
        else
            qDebug() << "VIDIOC_S_FMT success";

        memset(&fmt, 0, sizeof(fmt));
        fmt.type = type;
        if ((err = ioctl(fd, VIDIOC_G_FMT, &fmt)) < 0)
            qWarning() << "VIDIOC_G_FMT fail" << strerror(errno);
        else
            qWarning() << "fmt width =" << fmt.fmt.pix_mp.width
                << " height =" << fmt.fmt.pix_mp.height
                << " pfmt =" << fmt.fmt.pix_mp.pixelformat;
#if SUPPORT_V4L2_MPLANE
        Q_ASSERT(fmt.fmt.pix_mp.width == m_width);
        Q_ASSERT(fmt.fmt.pix_mp.height == m_height);
#else
        Q_ASSERT(fmt.fmt.pix.width == m_width);
        Q_ASSERT(fmt.fmt.pix.height == m_height);
#endif

#ifdef SUPPORT_NV12_STREAMING
#if SUPPORT_V4L2_MPLANE
        Q_ASSERT(fmt.fmt.pix_mp.pixelformat == V4L2_PIX_FMT_NV12M);
#else
        Q_ASSERT(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV12);
#endif
#else
#if SUPPORT_V4L2_MPLANE
        Q_ASSERT(fmt.fmt.pix_mp.pixelformat == V4L2_PIX_FMT_YUYV);
#else
        Q_ASSERT(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV);
#endif
#endif

#if SUPPORT_V4L2_MPLANE
        videodev.cap_width = fmt.fmt.pix_mp.width;
        videodev.cap_height = fmt.fmt.pix_mp.height;
        videodev.y_stride = fmt.fmt.pix_mp.plane_fmt[0].bytesperline;
        videodev.uv_stride = fmt.fmt.pix_mp.plane_fmt[1].bytesperline;

#else
        videodev.cap_width = fmt.fmt.pix.width;
        videodev.cap_height = fmt.fmt.pix.height;
        videodev.y_stride = fmt.fmt.pix.bytesperline;
        videodev.uv_stride = fmt.fmt.pix.bytesperline;
#endif

        return 0;
    }

    void RDKCamera::textureProcess(const NV12Frame *frame, int width, int height)
    {
        Q_UNUSED(width);
        Q_UNUSED(height);
        m_image->pushBackImage(const_cast<NV12Frame*>(frame));
    }
