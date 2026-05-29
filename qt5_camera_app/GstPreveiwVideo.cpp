#include "GstPreviewVideo.h"
#include <QDebug>

GstPreviewVideo::GstPreviewVideo(QObject *parent) : QThread(parent) {
    gst_init(nullptr, nullptr);
}

GstPreviewVideo::~GstPreviewVideo() {
    stop = true;
    terminatePipeline();
}

void GstPreviewVideo::setSource(const QString &filePath, int m_framerate) {
    if (!isRunning()) {
        createPipeline(filePath, m_framerate);
        start();
    } else {
        qWarning() << "Pipeline is already running";
    }
}

void GstPreviewVideo::createPipeline(const QString &filePath, int m_framerate) {
#if defined(KLAMATH)
    QString pipelineStr = QString(
            "filesrc location=\"%1\" ! "
            "qtdemux name=demux demux.video_0 ! "
            "h264parse ! avdec_h264 ! "
            "videorate ! video/x-raw,framerate=%2/1 ! "
            "videoconvert ! video/x-raw,format=RGB ! "
            "appsink name=mysink max-buffers=2 drop=true sync=true")
        .arg(filePath).arg(m_framerate);
#elif defined(DOLPHIN)
    QString pipelineStr = QString("filesrc location=\"%1\" ! qtdemux name=demux demux.video_0 ! "
            "h264parse ! v4l2h264dec ! glupload ! glcolorconvert ! gldownload ! "
            "video/x-raw,format=RGB,framerate=%2/1 ! appsink name=mysink max-buffers=5 sync=true")
        .arg(filePath).arg(m_framerate);
#endif
    QMutexLocker locker(&m_pipeline);
    GError *err = nullptr;
    pipeline = gst_parse_launch(pipelineStr.toUtf8().constData(), &err);

    if (err) {
        emit errorOccurred(QString(err->message));
        g_error_free(err);
        return;
    }

    sink = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
    if (!sink) {
        emit errorOccurred("Failed to get appsink from pipeline.");
        return;
    }

    bus = gst_element_get_bus(pipeline);
    if (!bus) {
        emit errorOccurred("Failed to get bus from pipeline.");
        return;
    }

    setupBus();
}

void GstPreviewVideo::play() {
    QMutexLocker locker(&m_pipeline);
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
    }
}

void GstPreviewVideo::pause() {
    QMutexLocker locker(&m_pipeline);
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_PAUSED);
    }
}

void GstPreviewVideo::terminatePipeline() {
    QMutexLocker locker(&m_pipeline);
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }

    if (sink) {
        gst_object_unref(sink);
        sink = nullptr;
    }
}

void GstPreviewVideo::setupBus() {
    gst_bus_add_watch(bus, busCall, this);
    gst_object_unref(bus);
}

gboolean GstPreviewVideo::busCall(GstBus *bus, GstMessage *msg, gpointer user_data) {
    Q_UNUSED(bus);
    GstPreviewVideo *self = static_cast<GstPreviewVideo*>(user_data);
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug_info;
            gst_message_parse_error(msg, &err, &debug_info);
            emit self->errorOccurred(QString::fromUtf8(err->message));
            g_error_free(err);
            g_free(debug_info);
            break;
        }
        case GST_MESSAGE_EOS:
            qDebug() << "End of Stream reached";
            emit self->endOfStream();
            self->terminatePipeline();
            break;
        default:
            break;
    }

    return TRUE;
}

void GstPreviewVideo::run() {
    QMutexLocker locker(&m_pipeline);
    if (pipeline)
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
    locker.unlock();

    if (sink) {
        GstSample *sample;

        while (!stop) {
            sample = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), 100 * GST_MSECOND);
            if (sample) {
                GstBuffer *buffer = gst_sample_get_buffer(sample);
                GstMapInfo map;

                if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
                    GstCaps *caps = gst_sample_get_caps(sample);
                    GstStructure *structure = gst_caps_get_structure(caps, 0);
                    int width, height;
                    gst_structure_get_int(structure, "width", &width);
                    gst_structure_get_int(structure, "height", &height);

                    QImage image = QImage(map.data, width, height, QImage::Format_RGB888).copy();
                    emit frameReady(image);
                    gst_buffer_unmap(buffer, &map);
                }
                gst_sample_unref(sample);
            } else {
                if (gst_app_sink_is_eos(GST_APP_SINK(sink))) {
                    emit endOfStream();
                    break;
                }
            }
        }
    }
}
