#include "CaptureVideo.h"
#include "GstStreamingFile.h"
#include "camera_player.h"

CaptureVideo::CaptureVideo(QObject *parent)
    :QObject(parent)
{
    mp_gst_streaming = new GstStreamingFileStreaming();
    mp_gst_streaming->start();
}
CaptureVideo::~CaptureVideo()
{
    delete mp_gst_streaming;
}
void CaptureVideo::SetCameraPlayer(QObject* obj, int width, int height, int framerate)
{
    updateWidth(width);
    updateHeight(height);
    updateFramerate(framerate);
    mp_camera_player = dynamic_cast<CameraPlayer*>(obj);
    if (mp_camera_player) {
        connect(mp_camera_player, &CameraPlayer::updateStream, this, &CaptureVideo::updateStreamingFile);
        connect(this, &CaptureVideo::filenameChanged, mp_gst_streaming, &GstStreaming::update);
        connect(this, &CaptureVideo::buffernumberChanged, mp_gst_streaming, &GstStreaming::update);
        connect(mp_gst_streaming, &GstStreaming::streamingDone, this, &CaptureVideo::onStreamingDone);
    }
}

void CaptureVideo::updateWidth(int val)
{
    mp_gst_streaming->setWidth(val);
}

void CaptureVideo::updateHeight(int val)
{
    mp_gst_streaming->setHeight(val);
}

void CaptureVideo::updateFramerate(int val)
{
    mp_gst_streaming->setFramerate(val);
}

void CaptureVideo::updateStreamingFile(uchar* buffer, int size)
{
    mp_gst_streaming->sendDataRemote(buffer, size);
}
bool CaptureVideo::start()const
{
    return mp_gst_streaming->getRunning();
}
void CaptureVideo::setStart(bool val)
{
    mp_gst_streaming->setRunning(val);
}
QString CaptureVideo::filename()const
{
    return mp_gst_streaming->getFileName();
}
void CaptureVideo::setFilename(QString val)
{
    QString filename  = mp_gst_streaming->getFileName();
    if (val != filename) {
        mp_gst_streaming->setFileName(val);
        emit filenameChanged(val);
    }
}
void CaptureVideo::onStreamingDone()
{
    emit startChanged(true);
}
int CaptureVideo::buffernumber()const
{
    return mp_gst_streaming->getNumberBuffer();
}
void CaptureVideo::setBuffernumber(int val)
{
    int number  = mp_gst_streaming->getNumberBuffer();
    if (val != number) {
        mp_gst_streaming->setNumberBuffer(val);
        emit buffernumberChanged(val);
    }
}
