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
    if (m_running == true && appsrc != NULL){
        GstStreaming::StreamBuffer *streamBuffer = new GstStreaming::StreamBuffer(data, size);
        m_queue.enqueue(streamBuffer);
        m_wait.wakeAll();
    }
    m_wait_mutex.unlock();
}

void GstStreaming::processDataRemote()
{
    GstBuffer *buffer = NULL;
    GstMapInfo info;
    m_close_mutex.lock();
    m_processing = true;
    m_close_mutex.unlock();

    m_wait_mutex.lock();
    // Ensure the thread processes the queue or waits if it's empty
    while (m_queue.isEmpty() && m_running == true)
        m_wait.wait(&m_wait_mutex);
    m_wait_mutex.unlock();

    if (! m_queue.isEmpty()) {
        GstStreaming::StreamBuffer *streamBuffer = m_queue.dequeue();
        buffer = gst_buffer_new_allocate(NULL, streamBuffer->m_size, NULL);
        if(buffer != NULL){
            gst_buffer_map(buffer, &info, GST_MAP_WRITE);
            memcpy((info.data), streamBuffer->m_buffer, streamBuffer->m_size);
            gst_buffer_unmap(buffer, &info);
            gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
            ++frame_count;
        } else {
            qWarning() <<"Error: "<<__func__ << "Failed to allocate gst_buffer\n";
        }
        delete streamBuffer;
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
