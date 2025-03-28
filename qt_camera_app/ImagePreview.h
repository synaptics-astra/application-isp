#ifndef IMAGEPREVIEW_H
#define IMAGEPREVIEW_H

#include "Preview.h"

class ImagePreview : public Preview {
    Q_OBJECT
public:
    explicit ImagePreview(QObject *parent = nullptr);
    Q_INVOKABLE void showImage(const QString &filePath);
};

#endif // IMAGEPREVIEW_H
