#ifndef CAPTURE_PHOTO_H
#define CAPTURE_PHOTO_H

#include <QObject>

class CameraPlayer;
class GstPhoto;

class CapturePhoto : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool capture READ capture WRITE setCapture NOTIFY captureChanged)
    Q_PROPERTY(QString photoFormat READ photoFormat WRITE setPhotoFormat NOTIFY photoFormatChanged)
    Q_PROPERTY(QString filename READ filename WRITE setFilename NOTIFY filenameChanged)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)

public:
    explicit CapturePhoto(QObject *parent = 0);
    ~CapturePhoto();
    Q_INVOKABLE void SetCameraPlayer(QObject* obj, int width, int height, int framerate);
    bool capture() const;
    void setCapture(bool val);
    QString photoFormat() const;
    void setPhotoFormat(const QString &val);
    QString filename() const;
    void setFilename(const QString &val);
    int width() const;
    void setWidth(int val);
    int height() const;
    void setHeight(int val);

signals:
    void captureChanged(bool);
    void photoFormatChanged(QString);
    void filenameChanged(QString);
    void widthChanged(int);
    void heightChanged(int);

public slots:
    void updatePhotoCapture(uchar* buffer, int size);
    void onCapturingDone();
    void updateWidth(int val);
    void updateHeight(int val);
    void updateFramerate(int val);

private:
    CameraPlayer *mp_camera_player;
    GstPhoto *mp_gst_photo;
};

#endif // CAPTURE_PHOTO_H
