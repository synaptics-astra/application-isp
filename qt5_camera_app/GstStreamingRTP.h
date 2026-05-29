#ifndef __GST_STREAMINGRTP_H__
#define __GST_STREAMINGRTP_H__

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
#include "GstStreaming.h"

#define DEFAULT_IP "10.46.216.197"
#define DEFAULT_PORT 8000
#define DEFAULT_BITRATE 1000

class GstStreamingRTP : public GstStreaming
{
    Q_OBJECT

    public:
        GstStreamingRTP(QObject *parent = 0);
        virtual ~GstStreamingRTP();

        unsigned short getRemotePort() {return m_remote_port;}
        void  setRemotePort(unsigned short val) { m_remote_port = val;}
        unsigned short getRemoteBitrate() {return m_remote_bitrate;}
        void  setRemoteBitrate(unsigned short val) { m_remote_bitrate = val;}
        QString getRemoteIP() {return m_remote_ip;}
        void  setRemoteIP(QString val) { m_remote_ip = val;}

    private:
        QString m_remote_ip;
        unsigned short m_remote_port;
        unsigned short m_remote_bitrate;
        GstElement *udpsink;
        virtual void createPipeline(int m_width, int m_height, int m_framerate);
        virtual void terminatePipeline();
};
#endif
