#ifndef EFFECTS_CONTROLLER_H
#define EFFECTS_CONTROLLER_H

#include <QObject>
#include <QProcess>

class EffectsController: public QObject
{
    Q_OBJECT

public:
    explicit EffectsController(QObject *parent = NULL);
    ~EffectsController();

    Q_INVOKABLE void setCprocEnabled(bool value, int device_id);
    Q_INVOKABLE void setContrastEffect(int value, int device_id);
    Q_INVOKABLE void setBrightnessEffect(int value, int device_id);
    Q_INVOKABLE void setSaturationEffect(int value, int device_id);

    Q_INVOKABLE void setAwbEnabled(bool value, int device_id);
    Q_INVOKABLE void setAeEnabled(bool value, int device_id);
};

#endif // EFFECTS_CONTROLLER_H
