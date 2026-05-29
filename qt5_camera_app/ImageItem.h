#ifndef IMAGEITEM_H
#define IMAGEITEM_H

#include <QQuickPaintedItem>
#include <QPainter>
#include <QImage>

class ImageItem : public QQuickPaintedItem {
    Q_OBJECT

public:
    explicit ImageItem(QQuickItem *parent = nullptr);
    void paint(QPainter *painter) override;

public slots:
    void setImage(const QImage &image);

private:
    QImage m_image;
};

#endif // IMAGEITEM_H
