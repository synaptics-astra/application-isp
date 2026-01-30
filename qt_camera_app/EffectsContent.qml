import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.15
import EffectsController 1.0
import RoiControl 1.0

ColumnLayout {
    id: columnTuneContent
    spacing: 5
    width: parent.width - 5

    property int device_id : cameraPlayer.get_device_id()
    property int video_fd: cameraPlayer.getVideoFd()
    property int defaultBlue: 200
    property int defaultRed: 200
    property int defaultGreen: 200

    property int defaultContrast: 154
    property int defaultBrightness: -5
    property int defaultSaturation: 154

    property double imageWidth: 1920
    property double imageHeight: 1080
    property int roi_number: 0


    property int roiWidth: columnTuneContent.width / 6

    EffectsController {
        id: effectsController
    }

    RoiControl {
       id: roiControl
    }

    //roi grid---start
    Item {
        id: roiGrid
        parent: cameraPlayer
        anchors.fill: parent
        visible: false

        property int n_rows: roiSizeBox.currentValue
        property int n_cols: roiSizeBox.currentValue
        property int roi_width: mainWindow.width / n_cols
        property int roi_height: mainWindow.height / n_rows

        Repeater {
            model: roiGrid.n_rows * roiGrid.n_cols
            Rectangle {
                width: roiGrid.roi_width
                height: roiGrid.roi_height
                border.color: "#66808080"
                border.width: 1
                color: "transparent"
                x: (index % roiGrid.n_cols) * roiGrid.roi_width
                y: Math.floor(index / roiGrid.n_cols) * roiGrid.roi_height
                Text {
                    id: roiNumber
                    text: "" + index
                    font.bold: true
                    font.pixelSize: 30
                    color: "#ff0000"
                    visible: false
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (parent.border.color == "#66808080") {
                            parent.border.color = "#000000";
                            parent.border.width = 2;
                            roiNumber.visible = true;

                            if (aeCheckbox.checked && awbCheckbox.checked) {
                                roiControl.setAwbRoi(video_fd, index, parent.x, parent.y, roiGrid.roi_width, roiGrid.roi_height);
                                roiControl.setAwbRoiWeight(video_fd, index, 91);
                                roiControl.setAeRoi(video_fd, index, parent.x, parent.y, roiGrid.roi_width, roiGrid.roi_height);
                                roiControl.setAeRoiWeight(video_fd, index, 91);
                            }
                            else if (aeCheckbox.checked) {
                                roiControl.setAeRoi(video_fd, index, parent.x, parent.y, roiGrid.roi_width, roiGrid.roi_height);
                                roiControl.setAeRoiWeight(video_fd, index, 91);
                            }
                            else if (awbCheckbox.checked) {
                                roiControl.setAwbRoi(video_fd, index, parent.x, parent.y, roiGrid.roi_width, roiGrid.roi_height);
                                roiControl.setAwbRoiWeight(video_fd, index, 91);
                            }
                        } else {
                            parent.border.color = "#66808080";
                            parent.border.width = 1;
                            roiNumber.visible = false;

                            if (aeCheckbox.checked) {
                                roiControl.setAeRoiWeight(video_fd, index, 0);
                            }
                            if (awbCheckbox.checked) {
                                roiControl.setAwbRoiWeight(video_fd, index, 0);
                            }
                        }
                    }
                }

            }
        }

    }
    //roi grid---end



    //Roi Item start
    Item {
        id: roiItem
        parent: cameraPlayer
        anchors.fill: parent
        visible: false

        ListModel {
            id: roiBoxModel
            ListElement { x: 0; y: 0; width: 0; height: 0 }
            ListElement { x: 0; y: 0; width: 0; height: 0 }
            ListElement { x: 0; y: 0; width: 0; height: 0 }
            ListElement { x: 0; y: 0; width: 0; height: 0 }
            ListElement { x: 0; y: 0; width: 0; height: 0 }
        }

        Repeater {
            model: roiBoxModel
            delegate: Rectangle {
                id: rect
                visible: false
                color: "transparent"
                width: model.width
                height: model.height
                x: model.x
                y: model.y
                border.color: "#62CDFF"
                border.width: 4

                Text {
                    color: "red"
                    text: (index+1).toString()
                    font.bold: true
                    font.pointSize: 20
                }
            }
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
                        roiModel.clearRow(roi_number);

                        if (roi_number > 0) roi_number--;
                    } else {
                        var xVal = (roiItem.children[roi_number].x/scaleX);
                        var yVal = (roiItem.children[roi_number].y/scaleY);
                        var widthVal = (roiItem.children[roi_number].width/scaleX);
                        var heightVal = (roiItem.children[roi_number].height/scaleY);

                        roiControl.setAwbRoi(video_fd, roi_number, xVal, yVal, widthVal, heightVal);
                        roiControl.setAwbRoiWeight(video_fd, roi_number, 100);

                        roiModel.setRow(roi_number, xVal, yVal, widthVal, heightVal, 100);
                        console.log("actual roi values: " + " " + roiItem.children[roi_number].x + " " + roiItem.children[roi_number].y + " " + roiItem.children[roi_number].width + " " + roiItem.children[roi_number].height);
                        console.log("scaled values: " + " " + xVal + " " + yVal + " " + widthVal + " "  + heightVal);

                        if (roi_number < 4)
                            roi_number++;
                    }
                }
            }
        }
    }
    //Roi Item end


    Label {
        text: "Effects"
        font.bold: true
        Layout.alignment: Qt.AlignVCenter
        color: "white"
    }

    RowLayout {
        spacing: 5
        CheckBox {
            id: awbCheckbox
            checked: true
            onCheckedChanged: effectsController.setAwbEnabled(awbCheckbox.checked ? 1 : 0, device_id)

        }

        Text {
            text: qsTr("AWB")
            color: "white"
            font.bold: true
            Layout.alignment: Qt.AlignVCenter
        }

    }

    RowLayout {
        spacing: 5
        CheckBox {
            id: aeCheckbox
            checked: true
            onCheckedChanged: effectsController.setAeEnabled(aeCheckbox.checked ? 1 : 0, device_id)
        }

        Text {
            text: qsTr("AE")
            color: "white"
            font.bold: true
            Layout.alignment: Qt.AlignVCenter
        }

    }

    RowLayout {
        spacing: 5
        CheckBox {
            id: cprocCheckbox
            checked: false
            onCheckedChanged: effectsController.setCprocEnabled(cprocCheckbox.checked ? 1 : 0, device_id)

        }

        Text {
            text: qsTr("CPROC")
            color: "white"
            font.bold: true
            Layout.alignment: Qt.AlignVCenter
        }

    }

    // CProc Sliders
    RowLayout {
        visible: cprocCheckbox.checked
        Label {
            text: "Saturation"
            Layout.preferredWidth: columnTuneContent.width * 0.3
            color: "white"
            font.bold: true
        }
        Slider {
            id: saturationSlider
            value: columnTuneContent.defaultSaturation
            from: 10
            to: 240
            stepSize: 10
            Layout.fillWidth: true
            onValueChanged: effectsController.setSaturationEffect(value, device_id)
        }
    }

    RowLayout {
        visible: cprocCheckbox.checked
        Label {
            text: "Contrast"
            Layout.preferredWidth: columnTuneContent.width * 0.3
            color: "white"
            font.bold: true
        }
        Slider {
            id: contrastSlider
            value: columnTuneContent.defaultContrast
            from: 10
            to: 240
            stepSize: 10
            Layout.fillWidth: true
            onValueChanged: effectsController.setContrastEffect(value, device_id)
        }
    }

    RowLayout {
        visible: cprocCheckbox.checked
        Label {
            text: "Brightness"
            Layout.preferredWidth: columnTuneContent.width * 0.3
            color: "white"
            font.bold: true
        }
        Slider {
            id: brightnessSlider
            value: columnTuneContent.defaultBrightness
            from: -115
            to: 115
            stepSize: 10
            Layout.fillWidth: true
            onValueChanged: effectsController.setBrightnessEffect(value, device_id)
        }
    }


    RowLayout {
        spacing: 5
        CheckBox {
            id: roiCheckBox
            checked: false
            onCheckedChanged: {
//                roiContainer.visible = checked
//                roiItem.visible = checked
                roiGrid.visible = checked
            }

        }

        Text {
            text: qsTr("ENABLE ROI")
            color: "white"
            font.bold: true
            Layout.alignment: Qt.AlignVCenter
        }

        CheckBox {
            id: roiSize
            checked: false
            onCheckedChanged: {
                roiSizeBox.visible = checked
            }

        }

        Text {
            text: qsTr("ROI SIZE")
            color: "white"
            font.bold: true
            Layout.alignment: Qt.AlignVCenter
        }

        ComboBox {
            id: roiSizeBox
            visible: false
            model: [5, 3, 4]
            currentIndex: 0
            Layout.fillWidth: true
            background: Rectangle {
                color: "white"
                border.color: "black"
                border.width: 1
                radius: 5
            }
        }
    }

    ListModel {
        id: roiModel

        ListElement { roiNo: "1"; x_val: 0; y_val: 0; roi_width: 0; roi_height: 0; roi_weight: 0}
        ListElement { roiNo: "2"; x_val: 0; y_val: 0; roi_width: 0; roi_height: 0; roi_weight: 0}
        ListElement { roiNo: "3"; x_val: 0; y_val: 0; roi_width: 0; roi_height: 0; roi_weight: 0}
        ListElement { roiNo: "4"; x_val: 0; y_val: 0; roi_width: 0; roi_height: 0; roi_weight: 0}
        ListElement { roiNo: "5"; x_val: 0; y_val: 0; roi_width: 0; roi_height: 0; roi_weight: 0}

        function clearRow(index) {
            get(index).x_val = 0;
            get(index).y_val = 0;
            get(index).roi_width = 0;
            get(index).roi_height = 0;
            get(index).roi_weight = 0;

            roiControl.setAwbRoi(video_fd, index, 0, 0, 0, 0);
            roiControl.setAwbRoiWeight(video_fd, index, 0);

        }

        function setRow(ind, xV, yV, wV, hV, wgt) {
            get(ind).x_val = Math.floor(xV);
            get(ind).y_val = Math.floor(yV);
            get(ind).roi_width = Math.floor(wV);
            get(ind).roi_height = Math.floor(hV);
            get(ind).roi_weight = Math.floor(wgt);
        }
    }

    Column {
        id: roiContainer
        visible: false
        Layout.fillWidth: true

        Row {
            spacing: 1
            Layout.fillWidth: true
            Label { text: "ROI No"; width: roiWidth; Layout.alignment: Qt.AlignCenter; color: "white"; font.bold: true}
            Label { text: "X"; width: roiWidth; Layout.alignment: Qt.AlignCenter; color: "white"; font.bold: true}
            Label { text: "Y"; width: roiWidth; Layout.alignment: Qt.AlignCenter; color: "white"; font.bold: true}
            Label { text: "Width"; width: roiWidth; Layout.alignment: Qt.AlignCenter; color: "white"; font.bold: true}
            Label { text: "Height"; width: roiWidth; Layout.alignment: Qt.AlignCenter; color: "white"; font.bold: true}
            Label { text: "Weight"; width: roiWidth; Layout.alignment: Qt.AlignCenter; color: "white"; font.bold: true}
//            Label { text: "Clear"; width: roiWidth; Layout.alignment: Qt.AlignCenter; color: "white"; font.bold: true}
        }

        Repeater {
            model: roiModel
            delegate: Row {
                spacing: 1
                Layout.fillWidth: true

                TextField { text: roiNo; width: roiWidth }
                TextField { text: x_val; width: roiWidth }
                TextField { text: y_val; width: roiWidth }
                TextField { text: roi_width; width: roiWidth }
                TextField { text: roi_height; width: roiWidth }
                TextField { text: roi_weight; width: roiWidth }

//                Button {
//                    text: "Clear"
//                    width: roiWidth
//                    onClicked: roiModel.clearRow(index)
//                }
            }
        }
    }

    Button {
        text: "Cproc Reset"
        visible: cprocCheckbox.checked
        background: Rectangle {
            color: "white"
            opacity: 0.7
            radius: 10
        }
        onClicked: {
            contrastSlider.value = columnTuneContent.defaultContrast
            saturationSlider.value = columnTuneContent.defaultSaturation
            brightnessSlider.value = columnTuneContent.defaultBrightness

            effectsController.setContrastEffect(defaultContrast)
            effectsController.setSaturationEffect(defaultSaturation)
            effectsController.setBrightnessEffect(defaultBrightness)
        }
        font.bold: true
        Layout.fillWidth: true
    }
}
