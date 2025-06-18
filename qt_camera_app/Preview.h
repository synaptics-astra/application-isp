#ifndef PREVIEW_H
#define PREVIEW_H

#include <QObject>
#include <QString>
#include <QDir>
#include <QFileInfoList>
#include <QDateTime>
#include <QUrl>


class Preview : public QObject {
    Q_OBJECT
public:
    explicit Preview(QObject *parent = nullptr) : QObject(parent) {}
    ~Preview() {}

    Q_INVOKABLE QString getLatestMediaFilePath(const QString &directoryPath);

protected:
    QString findLatestFile(QDir &directory, QStringList &filters);
    Q_INVOKABLE bool isVideoFile(const QString &filePath);
    Q_INVOKABLE bool isImageFile(const QString &filePath);
};

#endif // PREVIEW_H

