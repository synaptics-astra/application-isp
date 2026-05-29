#ifndef CAMERA_PLAYER_H
#define CAMERA_PLAYER_H

#include <QQuickItem>

class Camera;

class CameraPlayer : public QQuickItem
{
    Q_OBJECT
        Q_PROPERTY(bool play READ play WRITE setPlay NOTIFY playChanged)
        Q_PROPERTY(QString device_id READ get_device_id WRITE set_device_id NOTIFY deviceIdChanged)
        Q_PROPERTY(int frameWidth READ getWidth WRITE setWidth NOTIFY frameWidthChanged)
        Q_PROPERTY(int frameHeight READ getHeight WRITE setHeight NOTIFY frameHeightChanged)
        Q_PROPERTY(int frameRate READ getFramerate WRITE setFramerate NOTIFY frameRateChanged)
    public:
        explicit CameraPlayer(QQuickItem *parent = 0);
        ~CameraPlayer();

        QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);

        bool play() const { return mPlay; }
        void setPlay(bool);
        Q_INVOKABLE QString get_device_id();
        Q_INVOKABLE int getVideoFd();
        void set_device_id(QString);
        int getWidth() const { return frameWidth; }
        int getHeight() const { return frameHeight; }
        int getFramerate() const { return frameRate; }
        void setWidth(int value);
        void setHeight(int value);
        void setFramerate(int value);

    signals:
        void playChanged(bool);
        void updateStream(uchar* buffer, int size);
        void deviceIdChanged(QString);
        void frameWidthChanged(int);
        void frameHeightChanged(int);
        void frameRateChanged(int);

        private slots:
            void textureChanged(uchar* buffer, int size);
        void boundChanged();

    private:
        Camera *mCamera;
        bool mPlay;
        bool mBoundChanged;
        bool mTextureChanged;
        int frameWidth;
        int frameHeight;
        int frameRate;
};

#endif // CAMERA_PLAYER_H
