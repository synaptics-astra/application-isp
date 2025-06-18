#include "effects_controller.h"

EffectsController::EffectsController(QObject *parent):QObject(parent){}

EffectsController::~EffectsController(){}

void EffectsController::setContrastEffect(int value, int device_id)
{
    QProcess process;
        QString command = "v4l2-ctl";
        QStringList arguments;
        arguments << "-d" << QString("/dev/video%1").arg(device_id) << "-c" << QString("isp_cproc_manu_contrast=%1").arg(value);
        process.start(command, arguments);
    process.waitForFinished();
}

void EffectsController::setSaturationEffect(int value, int device_id)
{
    QProcess process;

        QString command = "v4l2-ctl";
        QStringList arguments;
        arguments << "-d" << QString("/dev/video%1").arg(device_id) << "-c" << QString("isp_cproc_manu_saturation=%1").arg(value);

        process.start(command, arguments);
    process.waitForFinished();
}

void EffectsController::setBrightnessEffect(int value, int device_id)
{
    QProcess process;

        QString command = "v4l2-ctl";
        QStringList arguments;
        arguments << "-d" << QString("/dev/video%1").arg(device_id) << "-c" << QString("isp_cproc_manu_bright=%1").arg(value);

        process.start(command, arguments);
    process.waitForFinished();
}

void EffectsController::setAwbEnabled(bool value, int device_id)
{
    QProcess process;

        QString command = "v4l2-ctl";
        QStringList arguments;
        arguments << "-d" << QString("/dev/video%1").arg(device_id) << "-c" << QString("isp_awb_enable=%1").arg(value);

        process.start(command, arguments);
    process.waitForFinished();
}

void EffectsController::setAeEnabled(bool value, int device_id)
{
    QProcess process;

        QString command = "v4l2-ctl";
        QStringList arguments;
        arguments << "-d" << QString("/dev/video%1").arg(device_id)<< "-c" << QString("isp_ae_enable=%1").arg(value);

        process.start(command, arguments);
    process.waitForFinished();
}

void EffectsController::setCprocEnabled(bool value, int device_id)
{
    QProcess process;

        QString command = "v4l2-ctl";
        QStringList arguments;
        arguments << "-d" << QString("/dev/video%1").arg(device_id) << "-c" << QString("isp_cproc_enable=%1").arg(value);

        process.start(command, arguments);
    process.waitForFinished();
}
