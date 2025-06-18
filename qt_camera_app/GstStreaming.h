#ifndef __GST_STREAMING_H__
#define __GST_STREAMING_H__

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

#define DEFAULT_IMAGE_FORMAT "png"
#define DEFAULT_HEIGHT 1080
#define DEFAULT_WIDTH 1920


class GstStreaming : public QThread
{
    Q_OBJECT
    public:
        GstStreaming(QObject *parent = 0);
        virtual ~GstStreaming();
        void startStream();
        void stopStream();
        void kill();
        static GMainLoop *loop;
        bool m_is_destroy;
        bool getRunning() {return m_running;}
        void setRunning(bool bRunning);
        void handleGstBusMessage();
        void sendDataRemote(uchar *data, int size);
        virtual void processDataRemote();
        GstElement* getPipeline() {return pipeline;}
        void setWidth(int val) { m_width = val; }
        void setHeight(int val) { m_height = val;}
        void setFramerate(int val) { m_framerate = val;}
        void setPipelineFlushed(bool val);
    signals:
        void streamingDone();
        public slots:
            void update();
    protected:
        bool m_running;
        bool m_processing;
        int m_width;
        int m_height;
        int m_framerate;
        bool m_pipeline_flushed;
        QMutex m_wait_mutex;
        QWaitCondition m_wait;
        QMutex m_close_mutex;
        QWaitCondition m_close;
        QMutex m_pipeline;

        GstElement *pipeline;
        GstElement *appsrc;
        GstCaps* caps;
        GstBus *bus;
        GstMessage *msg;
        static void* loopFunc(void*);
        virtual void createPipeline(int width, int height, int framerate)=0;
        virtual void terminatePipeline()=0;
        void run();
        class StreamBuffer
        {
            public :
                StreamBuffer(uchar* buffer, int size):m_buffer(buffer), m_size(size) {}
                uchar* m_buffer;
                int m_size;
        };
        QQueue<StreamBuffer*> m_queue;
};
#endif
