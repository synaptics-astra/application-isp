#include "PreviewLoader.h"


PreviewLoader::PreviewLoader(QObject *parent) : QObject(parent) {}

QString PreviewLoader::getLatestImageFilePath(const QString &directoryPath)
{
    QDir dir(directoryPath);
    QString filePath = findLatestFile(dir);
    if (!filePath.isEmpty()) {
        return QUrl::fromLocalFile(filePath).toString();
    }

    return "";
}


QString PreviewLoader::findLatestFile(QDir &directory)
{
    QString latestFilePath;
    QDateTime latestModifiedTime;

    directory.setFilter(QDir::Files | QDir::NoSymLinks);
    directory.setSorting(QDir::Time);
    QFileInfoList list = directory.entryInfoList();

    for (const QFileInfo &fileInfo : list) {
        if (fileInfo.isFile() && (fileInfo.suffix() == "png" || fileInfo.suffix() == "jpeg")) {
            QDateTime currentModifiedTime = fileInfo.lastModified();
            if (currentModifiedTime > latestModifiedTime) {
                latestModifiedTime = currentModifiedTime;
                latestFilePath = fileInfo.absoluteFilePath();
            }
        }
    }
    return latestFilePath;
}
