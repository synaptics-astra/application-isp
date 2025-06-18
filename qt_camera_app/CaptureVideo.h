#ifndef CAPTURE_VIDEO_H
#define CAPTURE_VIDEO_H

#include <QObject>

class CameraPlayer;
class GstStreamingFileStreaming;

class CaptureVideo : public QObject
{
	Q_OBJECT
    Q_PROPERTY(bool start READ start WRITE setStart NOTIFY startChanged)
    Q_PROPERTY(QString filename READ filename WRITE setFilename NOTIFY filenameChanged)
    Q_PROPERTY(int buffernumber READ buffernumber WRITE setBuffernumber NOTIFY buffernumberChanged)
public:
    explicit CaptureVideo(QObject *parent = 0);
    ~CaptureVideo();
    Q_INVOKABLE void SetCameraPlayer(QObject* obj, int width, int height, int framerate);
    bool start() const;
    void setStart(bool val);
    QString filename()const;
    void setFilename(QString val);
    int buffernumber()const;
    void setBuffernumber(int val);

public slots:
    void updateStreamingFile(uchar* buffer, int size);
    void onStreamingDone();
    void updateWidth(int val);
    void updateHeight(int val);
    void updateFramerate(int val);

signals:
    void startChanged(bool);
    void filenameChanged(QString);
    void buffernumberChanged(int);
private slots:
private:
    CameraPlayer *mp_camera_player;
    GstStreamingFileStreaming *mp_gst_streaming;
};

#endif // CAPTURE_VIDEO_H
