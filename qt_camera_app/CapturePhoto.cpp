#include "CapturePhoto.h"
#include "GstPhoto.h"
#include "camera_player.h"

CapturePhoto::CapturePhoto(QObject *parent)
    : QObject(parent)
{
    mp_gst_photo = new GstPhoto();
    mp_gst_photo->start();
}

CapturePhoto::~CapturePhoto()
{
    delete mp_gst_photo;
}

void CapturePhoto::SetCameraPlayer(QObject* obj, int width, int height, int framerate)
{
    updateWidth(width);
    updateHeight(height);
    updateFramerate(framerate);
    mp_camera_player = dynamic_cast<CameraPlayer*>(obj);
    if (mp_camera_player) {
        connect(mp_camera_player, &CameraPlayer::updateStream, this, &CapturePhoto::updatePhotoCapture);
        connect(this, &CapturePhoto::filenameChanged, mp_gst_photo, &GstStreaming::update);
        connect(mp_gst_photo, &GstStreaming::streamingDone, this, &CapturePhoto::onCapturingDone);
    }
}

void CapturePhoto::updateWidth(int val)
{
    mp_gst_photo->setWidth(val);
}

void CapturePhoto::updateHeight(int val)
{
    mp_gst_photo->setHeight(val);
}

void CapturePhoto::updateFramerate(int val)
{
    mp_gst_photo->setFramerate(val);
}

void CapturePhoto::updatePhotoCapture(uchar* buffer, int size)
{
     mp_gst_photo->sendDataRemote(buffer, size);
}

bool CapturePhoto::capture() const
{
    return mp_gst_photo->getRunning();
}

void CapturePhoto::setCapture(bool val)
{
    mp_gst_photo->setRunning(val);
}

QString CapturePhoto::photoFormat() const
{
    return mp_gst_photo->getPhotoFormat();
}

void CapturePhoto::setPhotoFormat(const QString &val)
{
    if (val != photoFormat()) {
        mp_gst_photo->setPhotoFormat(val);
        emit photoFormatChanged(val);
    }
}

QString CapturePhoto::filename() const
{
    return mp_gst_photo->getFileName();
}

void CapturePhoto::setFilename(const QString &val)
{
    if (val != filename()) {
        mp_gst_photo->setFileName(val);
        emit filenameChanged(val);
    }
}

int CapturePhoto::width() const
{
    return 1920;
}

void CapturePhoto::setWidth(int val)
{
    emit widthChanged(val);
}

int CapturePhoto::height() const
{
    return 1080;
}

void CapturePhoto::setHeight(int val)
{
    emit heightChanged(val);
}

void CapturePhoto::onCapturingDone()
{
	emit captureChanged(true);
}
