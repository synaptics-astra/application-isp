#ifndef VIDEOSTREAMER_H
#define VIDEOSTREAMER_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

class VideoStreamer : public QThread
{
    Q_OBJECT

public:
    explicit VideoStreamer(QObject *parent = nullptr)
        : QThread(parent), m_running(false){}

    virtual ~VideoStreamer() {}

    virtual void startStream() = 0;
    virtual void stopStream() = 0;

protected:
    bool m_running;
    QMutex m_mutex;
    QWaitCondition m_condition;

    virtual void run() = 0;
};

#endif // VIDEOSTREAMER_H
