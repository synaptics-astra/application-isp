#ifndef __GST_STREAMINGFILE_H__
#define __GST_STREAMINGFILE_H__

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
#include <QDateTime>
#include <QQueue>
#include "GstStreaming.h"

#define MAX_NUMBER_BUFFER 108000
#define DEFAULT_FILE_FORMAT "mp4"

class GstStreamingFileStreaming : public GstStreaming
{
    Q_OBJECT
public:
    GstStreamingFileStreaming(QObject *parent = 0);
    virtual ~GstStreamingFileStreaming();

    QString getFileName() {return m_file_name;}
    void  setFileName(QString val) { m_file_name = val;}

    int getNumberBuffer() {return m_number_buffer;}
    void setNumberBuffer(int val);

    QString getFileFormat() {return m_file_format;}
    void setFileFormat(QString val) { m_file_format = val;}
    void terminatePipelineInternal();
private:
    QString m_file_name;
    QString m_file_format;
    int m_number_buffer;
    GstElement *filesink;
    bool m_is_terminate_pipeline;
    bool m_is_thread_runing;
    bool clean_done = false;
    QMutex m_cond_mutex;
    QWaitCondition m_cond;
    pthread_t m_tid;

    static void* terminationHandler(void *arg);
    void cleanupTermination();
    void runTerminateHandler();
    virtual void createPipeline(int width, int height, int framerate);
    virtual void terminatePipeline();

    QString defaultFileName();
};
#endif
