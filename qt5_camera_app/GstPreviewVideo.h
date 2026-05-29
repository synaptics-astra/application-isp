#ifndef GSTPREVIEWVIDEO_H
#define GSTPREVIEWVIDEO_H

#include <QObject>
#include <QImage>
#include <QThread>
#include <QMutex>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

class GstPreviewVideo : public QThread {
    Q_OBJECT

public:
    explicit GstPreviewVideo(QObject *parent = nullptr);
    ~GstPreviewVideo();

    void setSource(const QString &filePath, int m_framerate);
    void play();
    void pause();
    void terminatePipeline();

signals:
    void frameReady(const QImage &image);
    void errorOccurred(QString error);
    void endOfStream();

protected:
    void run() override;
private:
    static gboolean busCall(GstBus *bus, GstMessage *msg, gpointer user_data);
    void createPipeline(const QString &filePath, int m_framerate);
    void setupBus();

    GstElement *pipeline = nullptr;
    GstElement *sink = nullptr;
    GstBus *bus = nullptr;
    bool stop = false;
    QMutex m_pipeline;
};

#endif // GSTPREVIEWVIDEO_H
