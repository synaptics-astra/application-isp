#ifndef PREVIEWLOADER_H
#define PREVIEWLOADER_H

#include <QObject>
#include <QString>
#include <QDir>
#include <QFileInfoList>
#include <QDateTime>
#include <QUrl>

class PreviewLoader : public QObject {
    Q_OBJECT
public:
    explicit PreviewLoader(QObject *parent = nullptr);
    Q_INVOKABLE QString getLatestImageFilePath(const QString &directoryPath);

private:
    QString findLatestFile(QDir &directory);
};

#endif // PREVIEWLOADER_H
