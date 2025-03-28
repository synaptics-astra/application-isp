#ifndef ROICONTROL_H
#define ROICONTROL_H

#include <QObject>

class RoiControl : public QObject {
    Q_OBJECT
    public:
        explicit RoiControl(QObject *parent = nullptr);
        ~RoiControl();

        Q_INVOKABLE void setAwbRoi(int deviceFd, int index, int x, int y, int width, int height);
        Q_INVOKABLE void setAwbRoiWeight(int deviceFd, int index, int weight);

        Q_INVOKABLE void setAeRoi(int deviceFd, int index, int x, int y, int width, int height);
        Q_INVOKABLE void setAeRoiWeight(int deviceFd, int index, int weight);


    private:
        uint16_t awbRoi[25][4] = {0};
        uint32_t awbWeights[25] = {0};
        uint16_t aeRoi[25][4] = {0};
        uint32_t aeWeights[25] = {0};
};

#endif // ROICONTROL_H
