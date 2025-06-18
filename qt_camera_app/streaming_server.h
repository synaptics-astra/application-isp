#ifndef STREAMING_SERVER_H
#define STREAMING_SERVER_H

#include <QObject>

class CameraPlayer;
class GstStreamingRTP;

class StreamingServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool start READ start WRITE setStart NOTIFY startChanged)
    Q_PROPERTY(unsigned short port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString addr READ addr WRITE setAddr NOTIFY addrChanged)
    public:
        explicit StreamingServer(QObject *parent = 0);
        ~StreamingServer();
        Q_INVOKABLE void SetCameraPlayer(QObject* obj, int width, int height, int framerate);
        bool start() const;
        void setStart(bool val);
        unsigned short port()const;
        void setPort(unsigned short val);
        QString addr()const;
        void setAddr(QString val);
    public slots:
        void updateStreamingServer(uchar* buffer, int size);
        void updateWidth(int val);
        void updateHeight(int val);
        void updateFramerate(int val);
    signals:
        void startChanged(bool);
        void portChanged(unsigned short);
        void addrChanged(QString);
    private:
        CameraPlayer *mp_camera_player;
        GstStreamingRTP *mp_gst_streaming;
};

#endif // STREAMING_SERVER_
