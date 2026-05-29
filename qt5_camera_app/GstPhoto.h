#ifndef __GST_PHOTO_H__
#define __GST_PHOTO_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/unistd.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QDateTime>
#include <QString>
#include "GstStreaming.h"


class GstPhoto : public GstStreaming
{
    Q_OBJECT
public:
    GstPhoto(QObject *parent = 0);
    virtual ~GstPhoto();

    QString getFileName() {return m_file_name;}
    void  setFileName(const QString& name);

    //new add .jpec, .png, .bmp
    QString getPhotoFormat() {return m_photo_format;}
    void setPhotoFormat(const QString& format);

    //change manual width and height
    int getImageWidth() {return m_image_width;}
    void setImageWidth(const int& width) {m_image_width = width;}

    int getImageHeight() {return m_image_height;}
    void setImageHeight(const int& height) {m_image_height = height;}

private:
    QString m_file_name;
    QString m_photo_format;
    GstElement *filesink;
    int m_image_width;
    int m_image_height;

    virtual void createPipeline(int width, int height, int framerate);
    virtual void terminatePipeline();
    static QString defaultFileName();
};

#endif // __GST_PHOTO_H__


