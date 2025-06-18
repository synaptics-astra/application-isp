#include <QDebug>
#include "streaming_server.h"
#include "GstStreamingRTP.h"
#include "camera_player.h"

StreamingServer::StreamingServer(QObject *parent)
    :QObject(parent)
{
    mp_gst_streaming = new GstStreamingRTP();
    mp_gst_streaming->start();
}
StreamingServer::~StreamingServer()
{
    delete mp_gst_streaming;
}
void StreamingServer::SetCameraPlayer(QObject* obj, int width, int height, int framerate)
{
    updateWidth(width);
    updateHeight(height);
    updateFramerate(framerate);
    mp_camera_player = dynamic_cast<CameraPlayer*>(obj);
    connect(mp_camera_player, &CameraPlayer::updateStream, this, &StreamingServer::updateStreamingServer);
    connect(this, &StreamingServer::portChanged, mp_gst_streaming, &GstStreaming::update);
    connect(this, &StreamingServer::addrChanged, mp_gst_streaming, &GstStreaming::update);
    connect(mp_camera_player, &CameraPlayer::frameWidthChanged, this, &StreamingServer::updateWidth);
    connect(mp_camera_player, &CameraPlayer::frameHeightChanged, this, &StreamingServer::updateHeight);
    connect(mp_camera_player, &CameraPlayer::frameRateChanged, this, &StreamingServer::updateFramerate);
}
void StreamingServer::updateWidth(int val)
{
    mp_gst_streaming->setWidth(val);
}

void StreamingServer::updateHeight(int val)
{
    mp_gst_streaming->setHeight(val);
}

void StreamingServer::updateFramerate(int val)
{
    mp_gst_streaming->setFramerate(val);
}

void StreamingServer::updateStreamingServer(uchar* buffer, int size)
{
    mp_gst_streaming->sendDataRemote(buffer, size);
}
bool StreamingServer::start()const
{
    return mp_gst_streaming->getRunning();
}
void StreamingServer::setStart(bool val)
{
    mp_gst_streaming->setRunning(val);
}
unsigned short StreamingServer::port()const
{
    return mp_gst_streaming->getRemotePort();
}
void StreamingServer::setPort(unsigned short val)
{
    unsigned short port = mp_gst_streaming->getRemotePort();
    if (val != port) {
        mp_gst_streaming->setRemotePort(val);
        emit portChanged(val);
    }
}
QString StreamingServer::addr()const
{
    return mp_gst_streaming->getRemoteIP();
}
void StreamingServer::setAddr(QString val)
{
    QString addr  = mp_gst_streaming->getRemoteIP();
    if (val != addr) {
        mp_gst_streaming->setRemoteIP(val);
        emit addrChanged(val);
    }
}
