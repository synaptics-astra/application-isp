#include "GstStreamingFile.h"
#include <QDebug>

GstStreamingFileStreaming::GstStreamingFileStreaming(QObject *parent)
    : GstStreaming(parent),m_file_format(DEFAULT_FILE_FORMAT), m_number_buffer(MAX_NUMBER_BUFFER),
    m_is_terminate_pipeline(false), m_is_thread_runing(false)
{
    /* Initialize GStreamer */
    m_file_name = defaultFileName();
    gst_init (NULL, NULL);
    pthread_create(&m_tid, NULL, terminationHandler, (void*)this);
}

QString GstStreamingFileStreaming::defaultFileName() {
    return "video_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
}

GstStreamingFileStreaming::~GstStreamingFileStreaming()
{
    m_is_thread_runing = true;
    pthread_join(m_tid, NULL);
    gst_deinit();
}
void GstStreamingFileStreaming::createPipeline(int m_width, int m_height, int m_framerate)
{
    m_cond_mutex.lock();
    GError *error = NULL;

    try {
        /*Get pipeline handle*/
        QString pipeline_str = QString("appsrc name=myappsrc ! rawvideoparse use-sink-caps=false "
                "disable-passthrough=false width=%1 height=%2 format=nv12 framerate=%3/1 ! "
                "videoconvert ! v4l2h264enc extra-controls=c,video_bitrate=20000 ! "
                "video/x-h264, profile=\"high\", level=(string)4.2 ! "
                "h264parse config-interval=-1 ! ").arg(m_width).arg(m_height).arg(m_framerate);

        pipeline_str += m_file_format + "mux ! filesink name=myfilesink location=/tmp/"
            + m_file_name +"."+ m_file_format;
        pipeline = gst_parse_launch(pipeline_str.toUtf8().constData(), &error);

        if(pipeline == NULL)
            throw std::runtime_error("Failed to create pipeline");

        /*Get Bus handle*/
        bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
        if(bus == NULL)
            throw std::runtime_error("Failed to get pipeline BUS");

        /* add watch for messages */
        handleGstBusMessage();
        /*Get appsrc and filesink and set remote ip and port*/
        filesink = gst_bin_get_by_name (GST_BIN(pipeline), "myfilesink");
        if(filesink == NULL)
            throw std::runtime_error("Failed to get filesink");

        appsrc = gst_bin_get_by_name (GST_BIN(pipeline), "myappsrc");

        if(appsrc == NULL)
            throw std::runtime_error("Failed to get appsrc");

        g_object_set (G_OBJECT(appsrc), "num-buffers", m_number_buffer, NULL);

        caps = gst_caps_new_simple ("video/x-raw",
                "format", G_TYPE_STRING, "NV12",
                "framerate", GST_TYPE_FRACTION, m_framerate, 1,
                "width", G_TYPE_INT, m_width,
                "height", G_TYPE_INT, m_height,
                NULL);

        gst_app_src_set_caps(GST_APP_SRC(appsrc), caps);
        gst_caps_unref(caps);
    }
    catch (const std::exception& e) {
        qWarning() << "Exception caught in createPipeline: " << e.what();
        if (bus)
            gst_object_unref(bus);
        if (pipeline)
            gst_element_set_state(pipeline, GST_STATE_NULL), gst_object_unref(pipeline);
        m_cond_mutex.unlock();
        return;
    }
    m_cond_mutex.unlock();

    qInfo() << "Successfully cretaed pipeline ";
}

void GstStreamingFileStreaming::terminatePipelineInternal()
{
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
            (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

    if (msg != nullptr) {
        if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) {
            qDebug("EOS received and handled");
        } else {
            GError* err = nullptr;
            gchar* debug_info = nullptr;
            gst_message_parse_error(msg, &err, &debug_info);
            qWarning() << "Error received from element" << GST_OBJECT_NAME(msg->src)
                << ":" << err->message;
            qWarning() << "Debugging information: " << (debug_info ? debug_info : "none");
            g_error_free(err);
            g_free(debug_info);
        }
        gst_message_unref(msg);
    }
}

void* GstStreamingFileStreaming::terminationHandler(void *arg)
{
    GstStreamingFileStreaming *pStream = (GstStreamingFileStreaming *)arg;
    pStream->runTerminateHandler();
    return (void*)NULL;
}

void GstStreamingFileStreaming::cleanupTermination()
{
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (appsrc);
    gst_object_unref (filesink);
    gst_object_unref (bus);
    gst_object_unref (pipeline);
}

void GstStreamingFileStreaming::runTerminateHandler()
{
    while (m_is_thread_runing) {
        m_cond_mutex.lock();
        while (!m_is_terminate_pipeline) {
            m_cond.wait(&m_cond_mutex);
        }

        terminatePipelineInternal();
        m_is_terminate_pipeline = false;
        QDeadlineTimer deadline = QDeadlineTimer();
        deadline.setRemainingTime(100);

        m_close_mutex.lock();
        while(m_processing == true)
            m_close.wait(&m_close_mutex, deadline);

        m_close_mutex.unlock();

        cleanupTermination();
        m_cond_mutex.unlock();
    }
}

void GstStreamingFileStreaming::terminatePipeline()
{
    m_cond_mutex.lock();
    m_is_terminate_pipeline = true;
    m_cond.wakeOne();
    m_cond_mutex.unlock();

    gst_element_send_event(pipeline, gst_event_new_eos());
    m_file_name = defaultFileName();
}

void GstStreamingFileStreaming::setNumberBuffer(int val)
{
    if(val < 0 || val > MAX_NUMBER_BUFFER){
        val = MAX_NUMBER_BUFFER;
    }
    m_number_buffer = val;
}
