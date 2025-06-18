#include "VideoPlayer.h"

VideoPlayer::VideoPlayer(QObject *parent) : QObject(parent), gstHandler(new GstPreviewVideo(this)) {
    connect(gstHandler, &GstPreviewVideo::frameReady, this, &VideoPlayer::frameReady);
    connect(gstHandler, &GstPreviewVideo::errorOccurred, this, &VideoPlayer::errorOccurred);
    gstHandler->start();
}

VideoPlayer::~VideoPlayer() {
    gstHandler->requestInterruption();
    gstHandler->wait();
    delete gstHandler;
}

void VideoPlayer::setSource(const QString &filePath, int m_framerate) {
    gstHandler->setSource(filePath, m_framerate);
}

void VideoPlayer::play() {
    gstHandler->play();
}

void VideoPlayer::pause() {
    gstHandler->pause();
}

void VideoPlayer::stop() {
    gstHandler->terminatePipeline();
}
