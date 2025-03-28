#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QObject>
#include "GstPreviewVideo.h"

class VideoPlayer : public QObject {
    Q_OBJECT

public:
    explicit VideoPlayer(QObject *parent = nullptr);
    ~VideoPlayer();

    Q_INVOKABLE void setSource(const QString &filePath, int m_framerate);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();

signals:
    void frameReady(const QImage &image);
    void errorOccurred(QString error);

private:
    GstPreviewVideo *gstHandler;
};

#endif // VIDEOPLAYER_H
