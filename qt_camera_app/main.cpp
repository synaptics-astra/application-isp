#include <QDebug>
#include <QQuickView>
#include <QQmlEngine>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include "effects_controller.h"
#include "camera_player.h"
#include "streaming_server.h"
#include "CaptureVideo.h"
#include "CapturePhoto.h"
#include "ImagePreview.h"
#include "VideoPlayer.h"
#include "ImageItem.h"
#include "roi_control.h"
#include <QFile>
#include <QTextStream>

#define DEFAULT_WIDTH 1920
#define DEFAULT_HEIGHT 1080
#define DEFAULT_FRAMERATE 30

#define OV_WIDTH 640
#define OV_HEIGHT 480
#define OV_FRAMERATE 60

void readSensorConfig(int &width, int &height, int &framerate) {
    QFile file("/proc/vsi/isp_subdev0");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open sensor configuration file";
        return;
    }

    QTextStream in(&file);
    QString line = in.readLine();
    int lineCount = 0;

    while (!line.isNull() && lineCount < 3) {
        line = in.readLine();
        lineCount++;
        if (lineCount == 2) {
            qWarning() << line;
            if (line.contains("ov")) {
                width = OV_WIDTH;
                height = OV_HEIGHT;
                framerate = OV_FRAMERATE;
            } else {
                width = DEFAULT_WIDTH;
                height = DEFAULT_HEIGHT;
                framerate = DEFAULT_FRAMERATE;
            }
            break;
        }
    }

    file.close();
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<CameraPlayer>("CameraPlayer", 1, 0, "CameraPlayer");
    qmlRegisterType<StreamingServer>("StreamingServer", 1, 0, "StreamingServer");
    qmlRegisterType<CaptureVideo>("CaptureVideo", 1, 0, "CaptureVideo");
    qmlRegisterType<EffectsController>("EffectsController", 1, 0, "EffectsController");
    qmlRegisterType<CapturePhoto>("CapturePhoto", 1, 0, "CapturePhoto");
    qmlRegisterType<ImagePreview>("ImagePreview", 1, 0, "ImagePreview");
    qmlRegisterType<VideoPlayer>("VideoPlayer", 1, 0, "VideoPlayer");
    qmlRegisterType<ImageItem>("ImageItem", 1, 0, "ImageItem");
    qmlRegisterType<RoiControl>("RoiControl", 1, 0, "RoiControl");

    QQmlApplicationEngine engine;

    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;
    int framerate = DEFAULT_FRAMERATE;

    readSensorConfig(width, height, framerate);

    engine.rootContext()->setContextProperty("windowHeight", height);
    engine.rootContext()->setContextProperty("windowWidth", width);
    engine.rootContext()->setContextProperty("cameraFps", framerate);
    engine.rootContext()->setContextProperty("defaultWidth", DEFAULT_WIDTH);

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
            &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
            qCritical() << "Error: Failed to load QML file:" << url.toString();
            QCoreApplication::exit(-1);
            }
            }, Qt::QueuedConnection);
    engine.load(url);



    return app.exec();
}
