#include "GstPhoto.h"
#include <QDateTime>
#include <QDebug>

GstPhoto::GstPhoto(QObject *parent)
    : GstStreaming(parent), m_photo_format(DEFAULT_IMAGE_FORMAT), m_image_width(DEFAULT_WIDTH),
    m_image_height(DEFAULT_HEIGHT)
{
    gst_init (NULL, NULL);
    m_file_name = defaultFileName();
}

GstPhoto::~GstPhoto() {
    terminatePipeline();
    gst_deinit();
}

void GstPhoto::setPhotoFormat(const QString& format) {
    if (format != "jpeg" && format != "png" && format != "bmp")
        m_photo_format = "png";
    else
        m_photo_format = format;
}

void GstPhoto::setFileName(const QString& name) {
    m_file_name = name;
}

QString GstPhoto::defaultFileName() {
    return "image_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
}

void GstPhoto::createPipeline(int m_width, int m_height, int m_framerate) {
    GError *error = NULL;

    QString pipeline_str = QString("appsrc name=myappsrc ! rawvideoparse use-sink-caps=false "
            "width=%1 height=%2 format=nv12 framerate=%3/1 ! videoconvert ! ")
        .arg(m_width).arg(m_height).arg(m_framerate);
    pipeline_str += m_photo_format + "enc ! filesink name=myfilesink location=/tmp/"
        + m_file_name +"."+ m_photo_format;

    QMutexLocker locker(&m_close_mutex);
    pipeline = gst_parse_launch(pipeline_str.toUtf8().constData(), &error);

    if (pipeline == NULL) {
        qWarning() << "Failed to create pipeline:" << error->message;
        g_error_free (error);
        return;
    }

    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    if(bus == NULL){
        qWarning() <<"Error: "<<__func__ << "Failed to get Rx pipeline BUS \n";
        return;
    }

    handleGstBusMessage();

    filesink = gst_bin_get_by_name(GST_BIN(pipeline), "myfilesink");
    if(filesink == NULL){
        qWarning() <<"Error: "<<__func__ << "Failed : to get filesink\n";
        return;
    }

    appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "myappsrc");
    if(appsrc == NULL){
        qWarning() <<"Error: "<<__func__ << "Failed : to get appsrc\n";
        return;
    }

    g_object_set (G_OBJECT(appsrc), "num-buffers", 1, NULL);

    qInfo() << "Successfully created pipeline to store image frames";
}

void GstPhoto::terminatePipeline() {
    m_file_name = defaultFileName();
    QDeadlineTimer deadline = QDeadlineTimer();
    deadline.setRemainingTime(100);
    m_close_mutex.lock();
    while(m_processing == true)
        m_close.wait(&m_close_mutex, deadline);
    m_close_mutex.unlock();
    gst_object_unref (appsrc);
    gst_object_unref (filesink);
    gst_object_unref (bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    qInfo() << "Successfully terminated pipeline";
}
