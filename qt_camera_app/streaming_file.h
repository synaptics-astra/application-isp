#ifndef STREAMING_FILE_H
#define STREAMING_FILE_H

#include <QObject>

class CameraPlayer;
class GstStreamingFileStreaming;

class StreamingFile : public QObject
{
    Q_OBJECT
        Q_PROPERTY(bool start READ start WRITE setStart NOTIFY startChanged)
        Q_PROPERTY(QString filename READ filename WRITE setFilename NOTIFY filenameChanged)
        Q_PROPERTY(int buffernumber READ buffernumber WRITE setBuffernumber NOTIFY buffernumberChanged)
    public:
        explicit StreamingFile(QObject *parent = 0);
        ~StreamingFile();
        Q_INVOKABLE void SetCameraPlayer(QObject* obj);
        bool start() const;
        void setStart(bool val);
        QString filename()const;
        void setFilename(QString val);
        int buffernumber()const;
        void setBuffernumber(int val);
        public slots:
            void updateStreamingFile(uchar* buffer, int size);
        void onStreamingDone();
signals:
        void startChanged(bool);
        void filenameChanged(QString);
        void buffernumberChanged(int);
        private slots:
    private:
            CameraPlayer *mp_camera_player;
            GstStreamingFileStreaming *mp_gst_streaming;
};

#endif // STREAMING_FILE_
