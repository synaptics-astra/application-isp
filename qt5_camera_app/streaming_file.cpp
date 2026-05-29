#include "streaming_file.h"
#include "GstStreamingFile.h"
#include "camera_player.h"

StreamingFile::StreamingFile(QObject *parent)
    :QObject(parent)
{
    mp_gst_streaming = new GstStreamingFileStreaming();
    mp_gst_streaming->start();
}
StreamingFile::~StreamingFile()
{
    delete mp_gst_streaming;
}
void StreamingFile::SetCameraPlayer(QObject* obj)
{
    mp_camera_player = dynamic_cast<CameraPlayer*>(obj);
    connect(mp_camera_player, &CameraPlayer::updateStream, this, &StreamingFile::updateStreamingFile);
    connect(this, &StreamingFile::filenameChanged, mp_gst_streaming, &GstStreaming::update);
    connect(this, &StreamingFile::buffernumberChanged, mp_gst_streaming, &GstStreaming::update);
    connect(mp_gst_streaming, &GstStreaming::streamingDone, this, &StreamingFile::onStreamingDone);
}

void StreamingFile::updateStreamingFile(uchar* buffer, int size)
{
    mp_gst_streaming->sendDataRemote(buffer, size);
}
bool StreamingFile::start()const
{
    return mp_gst_streaming->getRunning();
}
void StreamingFile::setStart(bool val)
{
    mp_gst_streaming->setRunning(val);
}
QString StreamingFile::filename()const
{
    return mp_gst_streaming->getFileName();
}
void StreamingFile::setFilename(QString val)
{
    QString filename  = mp_gst_streaming->getFileName();
    if (val != filename) {
        mp_gst_streaming->setFileName(val);
        emit filenameChanged(val);
    }
}
void StreamingFile::onStreamingDone()
{
    emit startChanged(true);
}
int StreamingFile::buffernumber()const
{
    return mp_gst_streaming->getNumberBuffer();
}
void StreamingFile::setBuffernumber(int val)
{
    int number  = mp_gst_streaming->getNumberBuffer();
    if (val != number) {
        mp_gst_streaming->setNumberBuffer(val);
        emit buffernumberChanged(val);
    }
}
