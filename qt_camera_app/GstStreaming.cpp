#include "GstStreaming.h"
#include <pthread.h>
#include <QDebug>
#include <time.h>
#include<sys/time.h>

#define FPS_COUNTER 30

GMainLoop* GstStreaming::loop = NULL;
static unsigned int frame_count = 0;

extern "C" {

    void handle_sigint(int sig)
    {
        if (GstStreaming::loop) {
            qInfo() << "The signal has come :\n" << sig;
            g_main_loop_quit(GstStreaming::loop);
        }
    }
    static gboolean tx_bus_message (GstBus * bus, GstMessage * message , gpointer data)
    {
        Q_UNUSED(bus);

        GstStreaming* gstStreaming = (GstStreaming*) data;

        switch (GST_MESSAGE_TYPE (message)) {
            case GST_MESSAGE_ERROR: {
                GError *err = NULL;
                gchar *dbg_info = NULL;
                gst_message_parse_error (message, &err, &dbg_info);
                g_printerr ("ERROR from element %s: %s\n",
                GST_OBJECT_NAME (message->src), err->message);
                g_printerr ("Debugging info: %s\n", (dbg_info) ? dbg_info : "none");
                g_error_free (err); g_free (dbg_info);
                break;
            }
            case GST_MESSAGE_EOS: {
                qWarning() << "got message: \n"<< gst_message_type_get_name (GST_MESSAGE_TYPE (message));
                gst_element_set_state (gstStreaming->getPipeline(), GST_STATE_PAUSED);
                gstStreaming->stopStream();
                gstStreaming->setPipelineFlushed(true);
                emit gstStreaming->streamingDone();
                break;
            }
            default:
                break;
        }
        return TRUE;
    }
    static void printTimeStamp()
    {
        timeval curTime;
        gettimeofday(&curTime, NULL);
        int milli = curTime.tv_usec / 1000;

        char buffer [80];
        strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&curTime.tv_sec));

        char currentTime[84] = "";
        sprintf(currentTime, "%s:%03d", buffer, milli);
        qDebug() << "current time: " << currentTime;
    }
}

GstStreaming::GstStreaming(QObject *parent)
    : QThread(parent),m_is_destroy(false),  m_running(false), m_processing(false), appsrc(0), caps(0)
{
}

GstStreaming::~GstStreaming()
{
}
void* GstStreaming::loopFunc(void* arg)
{
    Q_UNUSED(arg);
    if (loop == NULL) {
        loop = g_main_loop_new(NULL, FALSE);
        // Set up signal handler for Ctrl+C
        signal(SIGINT, handle_sigint);
        g_main_loop_run(loop);
        g_main_loop_unref(loop);
        loop = NULL;
    }
    return (void*) NULL;
}

void GstStreaming::run()
{
    QThread::currentThread()->setPriority(QThread::HighPriority);
    while (!m_is_destroy) {
        m_wait_mutex.lock();
        while (m_running == false) {
            m_wait.wait(&m_wait_mutex);
        }
        m_wait_mutex.unlock();
        processDataRemote();
    }
}

void GstStreaming::startStream()
{
    if (!m_running) {
        qInfo() << "Start Stream \n";
        createPipeline(m_width, m_height, m_framerate);
        pthread_attr_t attr;
        pthread_t thread;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&thread, &attr, GstStreaming::loopFunc, (void*)this);
        pthread_attr_destroy(&attr);
        m_wait_mutex.lock();
        m_running = true;
        m_wait.wakeAll();
        m_wait_mutex.unlock();
        gst_element_set_state (pipeline, GST_STATE_PLAYING);
    }
}

void GstStreaming::stopStream()
{
    qInfo() << "Stop Stream ";
    m_wait_mutex.lock();
    m_running = false;
    m_wait.wakeAll();
    m_wait_mutex.unlock();
    //gst_element_set_state (pipeline, GST_STATE_PAUSED);
    terminatePipeline();
    update();
}

void GstStreaming::setRunning(bool bRunning)
{
    if(bRunning != m_running){
        if(bRunning == true)
            startStream();
        else
            stopStream();
    }
}
void GstStreaming::update()
{
    if(m_running == true) {
        stopStream();
        startStream();
    }
}
void GstStreaming::handleGstBusMessage()
{
    gst_bus_add_watch (bus, (GstBusFunc)tx_bus_message, (void*) this);
}

void GstStreaming::setPipelineFlushed(bool val)
{
    if (m_pipeline_flushed != val)
        m_pipeline_flushed = val;
}
void GstStreaming::sendDataRemote(uchar *data, int size)
{
    m_wait_mutex.lock();
    if (m_running == true && appsrc != NULL) {
        const NV12Frame *in = reinterpret_cast<const NV12Frame*>(data);
        NV12Frame *f = new NV12Frame(*in);
        GstStreaming::StreamBuffer *streamBuffer =
            new GstStreaming::StreamBuffer(reinterpret_cast<uchar*>(f),
                static_cast<int>(sizeof(NV12Frame)));
        m_queue.enqueue(streamBuffer);
        m_wait.wakeAll();
    }
    m_wait_mutex.unlock();
}

bool GstStreaming::copy_nv12_data(const NV12Frame *f,
                                  int W, int H,
                                  int yStride, int uvStride,
                                  GstBuffer *out)
{
    GstMapInfo mi;
    if (!gst_buffer_map(out, &mi, GST_MAP_WRITE))
        return false;

    const gsize yBytes = (gsize)W * H;

    // Y plane
    for (int r = 0; r < H; ++r) {
        memcpy(mi.data + r * W, f->y + r * yStride, W);
    }

    // UV plane
    guint8 *uvDst = mi.data + yBytes;
    for (int r = 0; r < H / 2; ++r) {
        memcpy(uvDst + r * W, f->uv + r * uvStride, W);
    }

    gst_buffer_unmap(out, &mi);
    return true;
}

void GstStreaming::attach_nv12_meta(GstBuffer *out, int W, int H)
{
    gsize offsets[2] = { 0, (gsize)W * H };
    gint  strides[2] = { W, W };
    gst_buffer_add_video_meta_full(out,
                                   GST_VIDEO_FRAME_FLAG_NONE,
                                   GST_VIDEO_FORMAT_NV12,
                                   W, H,
                                   2,
                                   offsets,
                                   strides);
}

void GstStreaming::set_buffer_timestamps(GstBuffer *out, int fps)
{
    const GstClockTime dur = (fps > 0)
        ? gst_util_uint64_scale_int(1, GST_SECOND, fps)
        : GST_CLOCK_TIME_NONE;

    static guint64 s_seq = 0;
    const GstClockTime pts = (fps > 0) ? (s_seq * dur) : gst_util_get_timestamp();

    GST_BUFFER_PTS(out)      = pts;
    GST_BUFFER_DTS(out)      = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION(out) = dur;
    ++s_seq;
}

void GstStreaming::processDataRemote()
{
    m_close_mutex.lock();
    m_processing = true;
    m_close_mutex.unlock();

    m_wait_mutex.lock();
    while (m_queue.isEmpty() && m_running == true)
        m_wait.wait(&m_wait_mutex);
    m_wait_mutex.unlock();

    if (!m_queue.isEmpty()) {
        GstStreaming::StreamBuffer *streamBuffer = m_queue.dequeue();

        const NV12Frame* f = reinterpret_cast<const NV12Frame*>(streamBuffer->m_buffer);
        if (!f || !f->y || !f->uv || f->y_len == 0 || f->uv_len == 0) {
            qWarning() << "GstStreaming: invalid NV12M frame";
            delete streamBuffer;
        } else {
            const int W = m_width;
            const int H = m_height;
            const int yStride  = (f->y_stride  > 0) ? f->y_stride  : W;
            const int uvStride = (f->uv_stride > 0) ? f->uv_stride : W;

            const gsize total = (gsize)W * H + (gsize)W * (H / 2);

            GstBuffer *out = gst_buffer_new_allocate(nullptr, total, nullptr);
            if (!out) {
                qWarning() << "GstStreaming: alloc failed";
                delete f;
                delete streamBuffer;
            } else {
                if (!copy_nv12_data(f, W, H, yStride, uvStride, out)) {
                    qWarning() << "GstStreaming: copy failed";
                    gst_buffer_unref(out);
                    delete f;
                    delete streamBuffer;
                } else {
                    attach_nv12_meta(out, W, H);
                    set_buffer_timestamps(out, m_framerate);

                    GstFlowReturn fr = gst_app_src_push_buffer(GST_APP_SRC(appsrc), out);
                    if (G_UNLIKELY(fr != GST_FLOW_OK)) {
                        qWarning() << "GstStreaming: appsrc push failed, flow =" << fr;
                        gst_buffer_unref(out);
                    } else {
                        ++frame_count;
                    }
                    delete f;
                    delete streamBuffer;
                }
            }
        }
    }

    m_close_mutex.lock();
    m_processing = false;
    m_close.wakeAll();
    m_close_mutex.unlock();
}

void GstStreaming::kill()
{
    if (m_running) {
        // Stop the stream
        stopStream();

        // Ensure all buffers in the queue are processed
        m_wait_mutex.lock();
        while (!m_queue.isEmpty()) {
            m_wait.wait(&m_wait_mutex); // Wait for the queue to empty
        }
        m_wait_mutex.unlock();

        // After the queue is empty, ensure that the appsrc is flushed
        if (appsrc != NULL) {
            // Signal end of stream to GStreamer pipeline to flush any remaining buffers
            gst_app_src_end_of_stream(GST_APP_SRC(appsrc));
            // Optionally, wait for a flush timeout here if needed
            QElapsedTimer flushTimer;
            flushTimer.start();
            while (flushTimer.elapsed() < 5000) {
                // Wait for a few seconds (this could be adjusted as needed)
                if (m_pipeline_flushed) {
                    break;
                }
            }

            gst_element_set_state(pipeline, GST_STATE_NULL); // Stop the pipeline gracefully
        }

        // Clean up and mark the object for destruction
        m_is_destroy = true;
        m_wait_mutex.lock();
        m_wait.wakeAll();  // Wake all threads waiting on the object
        m_wait_mutex.unlock();
    }
}
