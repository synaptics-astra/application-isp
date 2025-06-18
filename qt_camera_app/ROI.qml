import QtQuick 2.15
import QtQuick.Controls 2.15
import RoiControl 1.0

Item {
    id: roiItem
    property double imageWidth: 1920
    property double imageHeight: 1080
    property int roi_number: 0


    ListModel {
        id: roiModel
        ListElement { x: 0; y: 0; width: 0; height: 0 }
        ListElement { x: 0; y: 0; width: 0; height: 0 }
        ListElement { x: 0; y: 0; width: 0; height: 0 }
    }

    Repeater {
        model: roiModel
        delegate: Rectangle {
            id: rect
            visible: false
            color: "transparent"
            width: model.width
            height: model.height
            x: model.x
            y: model.y
            border.color: "#62CDFF"
            border.width: 2

            Text {
                color: "red"
                text: index.toString()

            }
        }
    }



    signal roiChanged(int indx, int xVal, int yVal, int widthVal, int heightVal, int weightVal)

    RoiControl {
       id: roiControl
    }

    MouseArea {
        id: roiMouseArea
        anchors.fill: parent

        property point startPoint
        property double scaleX: topRectangle.width / imageWidth
        property double scaleY: topRectangle.height / imageHeight

        onPressed: {
            if (roiMouseArea.enabled) {
                roiItem.children[roi_number].visible = true
                roiItem.children[roi_number].x = mouse.x
                roiItem.children[roi_number].y = mouse.y
                roiItem.children[roi_number].width = 0
                roiItem.children[roi_number].height = 0
                startPoint = Qt.point(roiItem.children[roi_number].x, roiItem.children[roi_number].y)
            }
        }

        onPositionChanged: {
            if (mouse.buttons & Qt.LeftButton && roiMouseArea.enabled) {
                var rectWidth = mouse.x - startPoint.x
                var rectHeight = mouse.y - startPoint.y
                roiItem.children[roi_number].width = Math.max(0, rectWidth)
                roiItem.children[roi_number].height = Math.max(0, rectHeight)
            }
        }

        onReleased: {
            if (roiMouseArea.enabled) {
                if (roiItem.children[roi_number].width === 0 && roiItem.children[roi_number].height === 0) {
                    roiItem.children[roi_number].visible = false;
                    if (roi_number > 0) roi_number--;
                } else {
                    var xVal = (roiItem.children[roi_number].x/scaleX);
                    var yVal = (roiItem.children[roi_number].y/scaleY);
                    var widthVal = (roiItem.children[roi_number].width/scaleX);
                    var heightVal = (roiItem.children[roi_number].height/scaleY);

                    roiControl.setRoi(roi_number, xVal, yVal, widthVal, heightVal);
                    roiControl.setRoiWeight(roi_number, 100);

                    roiChanged(roi_number, xVal, yVal, widthVal, heightVal, 100);

                    if (roi_number < 2)
                        roi_number++;
                }
            }
        }
    }
}
