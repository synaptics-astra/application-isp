#include "Preview.h"
#include <QFileInfo>

QString Preview::getLatestMediaFilePath(const QString &directoryPath) {
    QDir dir(directoryPath);
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg" << "*.mp4" << "*.avi";
    QString filePath = findLatestFile(dir, filters);

    if (!filePath.isEmpty()) {
        return filePath;
    }

    return "";
}

QString Preview::findLatestFile(QDir &directory, QStringList &filters) {
    QString latestFilePath;
    QDateTime latestModifiedTime;

    QFileInfoList list = directory.entryInfoList(filters, QDir::Files, QDir::Time);

    if (!list.isEmpty()) {
        QFileInfo fileInfo = list.first();
        latestFilePath = fileInfo.absoluteFilePath();
    }

    return latestFilePath;
}

bool Preview::isVideoFile(const QString &filePath) {
    return filePath.endsWith(".mp4") || filePath.endsWith(".avi");
}

bool Preview::isImageFile(const QString &filePath) {
    return filePath.endsWith(".png") || filePath.endsWith(".jpeg") || filePath.endsWith(".jpg");
}
