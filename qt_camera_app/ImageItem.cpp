#include "ImageItem.h"

ImageItem::ImageItem(QQuickItem *parent) : QQuickPaintedItem(parent) {
    setOpaquePainting(true);
}

void ImageItem::setImage(const QImage &image) {
    m_image = image;
    update();
}

void ImageItem::paint(QPainter *painter) {
    if (!m_image.isNull()) {
        painter->drawImage(boundingRect(), m_image);
    }
}
