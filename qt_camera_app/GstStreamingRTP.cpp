#include "GstStreamingRTP.h"
#include <pthread.h>
#include <QDebug>

GstStreamingRTP::GstStreamingRTP(QObject *parent)
    : GstStreaming(parent), m_remote_ip(DEFAULT_IP), m_remote_port(DEFAULT_PORT)
{
    /* Initialize GStreamer */
    gst_init (NULL, NULL);
}

GstStreamingRTP::~GstStreamingRTP()
{
    gst_deinit();
}

void GstStreamingRTP::createPipeline(int m_width, int m_height, int m_framerate)
{
    m_pipeline.lock();
    GError *error = NULL;
    /*Get pipeline handle*/
#ifdef SUPPORT_SOFT_H264ENC
    pipeline = gst_parse_launch("appsrc name=myappsrc ! rawvideoparse use-sink-caps=false "
            "width=1920 height=1080 format=nv12 framerate=30/1 ! x264enc key-int-max=30  "
            "speed-preset=veryfast tune=zerolatency sliced-threads=true threads=6 ! "
            "h264parse config-interval=-1 ! rtph264pay ! udpsink  name=myudpsink", &error);
#else
    QString pipeline_str = QString("appsrc name=myappsrc ! rawvideoparse use-sink-caps=false "
            "disable-passthrough=false width=%1 height=%2 format=nv12 framerate=%3/1 ! "
            "videorate ! video/x-raw,framerate=%3/1 ! v4l2h264enc "
            "extra-controls=\"c,video_bitrate=30000, video_gop_size=20\" ! "
            "video/x-h264, profile=high, level=(string)4.2, framerate=%3/1 ! "
            "h264parse config-interval=-1 ! rtph264pay ! udpsink  name=myudpsink sync=false")
        .arg(m_width).arg(m_height).arg(m_framerate);

    QMutexLocker locker(&m_close_mutex);
    pipeline = gst_parse_launch(pipeline_str.toUtf8().constData(), &error);
#endif

    if(pipeline == NULL){
        qWarning() << "Error: "<<__func__ << "failed to create pipeline :" <<error->message << "\n";
        m_pipeline.unlock();
        return;
    }

    /*Get Bus handle*/
    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    if(bus == NULL){
        qWarning() <<"Error: "<<__func__ << "Failed to get Rx pipeline BUS \n";
        m_pipeline.unlock();
        return;
    }

    /* add watch for messages */
    handleGstBusMessage();
    /*Get appsrc and udpsink and set remote ip and port*/
    udpsink = gst_bin_get_by_name (GST_BIN(pipeline), "myudpsink");
    if(udpsink == NULL){
        qWarning() <<"Error: "<<__func__ << "Failed : to get udpsink\n";
        m_pipeline.unlock();
        return;
    }
    g_object_set (G_OBJECT(udpsink), "host", m_remote_ip.toStdString().c_str(), NULL);
    g_object_set (G_OBJECT(udpsink), "port", m_remote_port, NULL);

    appsrc = gst_bin_get_by_name (GST_BIN(pipeline), "myappsrc");

    if(appsrc == NULL){
        qWarning() <<"Error: "<<__func__ << "Failed : to get appsrc\n";
        m_pipeline.unlock();
        return;
    }

    /* set the caps on the source */
    caps = gst_caps_new_simple ("video/x-raw",
            "format", G_TYPE_STRING, "NV12",
            "framerate", GST_TYPE_FRACTION, m_framerate, 1,
//            "colorimetry", G_TYPE_STRING, "bt601",
//            "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
//            "chroma-site", G_TYPE_STRING, "mpeg2",
            "interlace-mode", G_TYPE_STRING, "progressive",
            "width", G_TYPE_INT, m_width,

            "height", G_TYPE_INT, m_height,
            NULL);

    gst_app_src_set_caps(GST_APP_SRC(appsrc), caps);
    m_pipeline.unlock();
    gst_caps_unref(caps);

    qInfo() << "Successfully cretaed pipeline ";

}

void GstStreamingRTP::terminatePipeline()
{
    QDeadlineTimer deadline = QDeadlineTimer();
    deadline.setRemainingTime(100);
    m_close_mutex.lock();
    while(m_processing == true)
        m_close.wait(&m_close_mutex, deadline);
    m_close_mutex.unlock();
    gst_object_unref (appsrc);
    gst_object_unref (udpsink);
    gst_object_unref (bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    qInfo() << "Successfully terminated pipeline";
}
