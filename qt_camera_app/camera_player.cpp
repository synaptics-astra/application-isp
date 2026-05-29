#include "camera_player.h"
#include <QSGNode>
#include <QDebug>

#ifdef CONFIG_RDK_PLATFORM
#include "rdk_camera.h"
#endif

CameraPlayer::CameraPlayer(QQuickItem *parent)
    : QQuickItem(parent), mPlay(false), mBoundChanged(true), mTextureChanged(true)
{
    setFlag(ItemHasContents, true);
    connect(this, &CameraPlayer::xChanged, this, &CameraPlayer::boundChanged);
    connect(this, &CameraPlayer::yChanged, this, &CameraPlayer::boundChanged);
    connect(this, &CameraPlayer::widthChanged, this, &CameraPlayer::boundChanged);
    connect(this, &CameraPlayer::heightChanged, this, &CameraPlayer::boundChanged);
#ifdef CONFIG_RDK_PLATFORM
    mCamera = new RDKCamera(this);
#endif
    connect(mCamera, &Camera::imageChanged, this, &CameraPlayer::textureChanged);
    mCamera->start();
}

void CameraPlayer::setWidth(int value) {
    if (frameWidth != value) {
        frameWidth = value;
        mCamera->setWidth(value);
        emit frameWidthChanged(value);
    }
}

void CameraPlayer::setHeight(int value) {
    if (frameHeight != value) {
        frameHeight = value;
        mCamera->setHeight(value);
        emit frameHeightChanged(value);
    }
}

void CameraPlayer::setFramerate(int value) {
    if (frameRate != value) {
        frameRate = value;
        emit frameRateChanged(value);
    }
}

CameraPlayer::~CameraPlayer()
{

}

QString CameraPlayer::get_device_id()
{
    return QVariant(mCamera->getDeviceId()).toString();
}

int CameraPlayer::getVideoFd()
{
    return mCamera->getVideoFd();
}

void CameraPlayer::set_device_id(QString str)
{
    mCamera->setDeviceId(str.toUInt());
    emit deviceIdChanged(str);
}

void CameraPlayer::setPlay(bool value)
{
    if (mPlay == value)
        return;

    if (value)
        mCamera->startStream();
    else
        mCamera->stopStream();

    mPlay = value;
    emit playChanged(value);
}

void CameraPlayer::boundChanged()
{
    mBoundChanged = true;
    update();
}

void CameraPlayer::textureChanged(uchar* buffer, int size)
{
    mTextureChanged = true;
    emit updateStream( buffer,size);
    update();
}

QSGNode *CameraPlayer::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QThread::currentThread()->setPriority(QThread::HighPriority);
    //QSGGeometryNode *node;
    QSGNode *node;

    if (!oldNode) {
        node = mCamera->createNode();
    }
    else
        node = static_cast<QSGGeometryNode *>(oldNode);

    if (mBoundChanged) {
        mCamera->updateGeometry(x(), y(), width(), height());
        mBoundChanged = false;
        node->markDirty(QSGNode::DirtyGeometry);
    }

    if (mTextureChanged) {
        mCamera->updateMaterial();
        mTextureChanged = false;
        node->markDirty(QSGNode::DirtyMaterial);
    }

    return node;
}

