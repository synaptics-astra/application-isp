#include "ImagePreview.h"
#include <QDebug>

ImagePreview::ImagePreview(QObject *parent) : Preview(parent) {}

void ImagePreview::showImage(const QString &filePath) {
    if (isImageFile(filePath)) {
        qDebug() << "Displaying image:" << filePath;
    } else {
        qDebug() << "Not an image file:" << filePath;
    }
}
