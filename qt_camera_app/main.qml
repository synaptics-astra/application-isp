import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import CameraPlayer 1.0
import StreamingServer 1.0
import CaptureVideo 1.0
import QtQuick.Window 2.15
import EffectsController 1.0
import CapturePhoto 1.0
import VideoPlayer 1.0
import ImageItem 1.0

ApplicationWindow {
    id: mainWindow
    title: "Astra-Cam"
    visible: true
    width: windowWidth
    height: windowHeight
    visibility: windowWidth < defaultWidth ? ApplicationWindow.Windowed :  ApplicationWindow.FullScreen
    color: "black"
    property int defaultWidth: 1920
    property int defaultHeight: 1080
    property real scaleFix: Math.max(0.6, Math.min(width / defaultWidth, height / defaultHeight))


    //close app
    Action {
        id: closeAppAction
        shortcut: "Ctrl+C"
        onTriggered: {
            Qt.quit();
        }
    }

    Keys.onPressed: {
        if (closeAppAction.shortcut.match(event)) {
            closeAppAction.trigger();
        }
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        spacing: 5

        //Left column-------------------------------------------
        Rectangle {
            id: leftColumn
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width
            anchors.centerIn: parent
            Layout.alignment: Qt.AlignCenter
            color: "black"
            radius: 15


            ColumnLayout {
                id: leftColumnLayout
                anchors.fill: parent

                Rectangle {
                    id: topRectangle
                    Layout.fillWidth: true
                    Layout.preferredHeight: parent.height
                    color: "black"

                    signal sendRoi(int ind, int xChanged, int yChanged, int weightChanged, int heightChanged, int weightChange)

                    CameraPlayer {
                       id: cameraPlayer
                       width: parent.width
                       height: parent.height
                       play: true
                       visible: true
                       frameWidth: windowWidth
                       frameHeight: windowHeight
                       frameRate: cameraFps
                       ROI {
                            id: roiBox
                            anchors.fill: parent
                            visible: false;

                            onRoiChanged: {
                                mainLayout.roi_ind = indx;
                                mainLayout.x_val = xVal;
                                mainLayout.y_val = yVal;
                                mainLayout.roi_width = widthVal;
                                mainLayout.roi_height = heightVal;
                                mainLayout.roi_weight = weightVal;
                            }
                        }
                    }

                    //preview image
                    Image {
                        id: previewImage
                        width: parent.width
                        height: parent.height
                        anchors.centerIn: parent
                        Layout.alignment: Qt.AlignCenter
                        visible: false
                    }

                    //preview video
                    Rectangle {
                        id: previewVideo
                        width: parent.width
                        height: parent.height
                        anchors.centerIn: parent
                        Layout.alignment: Qt.AlignCenter
                        visible: false


                        VideoPlayer {
                            id: videoPlayer

                            onFrameReady: {
                                videoOutput.setImage(image)
                            }
                        }

                        ImageItem {
                            id: videoOutput
                            anchors.fill: parent
                        }
                    }

                    //capture animmation
                    Rectangle {
                        id: cameraOverlay
                        visible: false
                        anchors.fill: cameraPlayer
                        color: "#80000000"

                        SequentialAnimation {
                            id: captureAnimation
                            running: false

                            PropertyAnimation {
                                target: cameraOverlay
                                property: "opacity"
                                from: 0.0
                                to: 1.0
                                duration: 30
                            }

                            PauseAnimation {
                                duration: 15
                            }

                            PropertyAnimation {
                                target: cameraOverlay
                                property: "opacity"
                                from: 1.0
                                to: 0.0
                                duration: 30
                            }

                            onStopped: cameraOverlay.visible = false;
                        }
                    }

                    Rectangle {
                        id: bottomRectangle
                        width: topRectangle.width
                        height: Math.max(80, topRectangle.height * 0.15)
                        color: "transparent"
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
//                        anchors.margins: 20

                        //exit button
                        Button {
                            id: exitButton
                            width: Math.max(48, parent.width * 0.04 * mainWindow.scaleFix)
                            height: width
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            anchors.margins: 20 * mainWindow.scaleFix

                            background: Rectangle {
                                color: "transparent"
                            }

                            contentItem: Image {
                                id: eixtIcon
                                source: "qrc:/Image/exit1.png"
                                fillMode: Image.PreserveAspectFit
                                anchors.fill: parent
                                anchors.margins: 8
                            }

                            onClicked: {
                                Qt.quit()
                            }
                        }

                        //Multistreaming on/off----------------------------------------
                        Rectangle {
                            id: mode
                            width: bottomRectangle.width * 0.06
                            height: bottomRectangle.width * 0.03
                            anchors.left: bottomRectangle.left
                            anchors.bottom: bottomRectangle.bottom
                            anchors.margins: 20
                            visible: false
                            radius: mode.height / 2
                            color: slider.x >= mode.width / 2 ? "#0D92F4" : "#62CDFF"
//                            anchors.verticalCenter: bottomRectangle.verticalCenter

                            Rectangle {
                                id: slider
                                width: mode.height
                                height: mode.height
                                radius: height / 2
                                color: "white"

                                Text {
                                    id: modeText
                                    text: slider.checked ? "M" : "S"
                                    color: slider.x >= mode.width / 2 ? "#0D92F4" : "#62CDFF"
                                    font.bold: true
                                    anchors.centerIn: parent
                                    Layout.alignment: Qt.AlignCenter
                                }

                                property bool checked: false
                                x: checked ? mode.width - width : 0

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        slider.checked = !slider.checked
                                        if (slider.checked) {
//                                            roiBox.visible = true;
                                        } else {
//                                            roiBox.visible = false;
                                        }
                                    }
                                }

                                Behavior on x {
                                    NumberAnimation {
                                        duration: 50
                                        onStopped: {
                                            modeText.text = slider.checked ? "M" : "S"
                                        }
                                    }
                                }

                                onCheckedChanged: {
                                    slider.x = checked ? mode.width - width : 0;
                                }
                            }
                        }

                        Rectangle {
                            id: buttonContainer
                            visible: buttonContainer
                            anchors.centerIn: parent
                            Layout.alignment: Qt.AlignCenter
                            width: bottomRectangle.width * 0.4
                            height: bottomRectangle.height * 0.7
                            radius: 10
                            color: "#80000000"//66: 40 percent 33: 20 PathPercent

                            BottomRectangle {
                                id: buttonController
                                anchors.horizontalCenter: bottomRectangle.horizontalCenter
                                property int activeButtonNumber: 1
                                onSendActiveButtonNumber: {
                                    buttonController.activeButtonNumber = buttonNumber
                                }

                                StreamingServer {
                                    id: server1
                                    Component.onCompleted: {
                                        server1.SetCameraPlayer(cameraPlayer, windowWidth, windowHeight, cameraFps);
                                    }
                                }

                                //capture video
                                CaptureVideo {
                                    id: captureVideo
                                    Component.onCompleted: {
                                        captureVideo.SetCameraPlayer(cameraPlayer, windowWidth, windowHeight, cameraFps);
                                    }
                                }

                                CapturePhoto {
                                    id: photoCapture
                                    Component.objectName: {
                                        photoCapture.SetCameraPlayer(cameraPlayer, windowWidth, windowHeight, cameraFps);
                                    }
                                }
                            }
                        }

                        //preview ---------------------------------
                        Preview {
                            id: previeButton
                            anchors.right: bottomRectangle.right
                            anchors.bottom: bottomRectangle.bottom
                            //width: Math.max(60, 56 * mainWindow.scaleFix)
                            width: Math.max(48, parent.width * 0.04 * mainWindow.scaleFix)
                            height: width
                            color: "transparent"
                            visible: true
                        }
                    }

                    Button {
                        id: settingButton
                        width: Math.max(40, parent.width * 0.02 * mainWindow.scaleFix)
                        height: width
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.margins: 20 * mainWindow.scaleFix
                        visible: true
                        contentItem: Image {
                            source: "qrc:/Image/settings.png"
                            fillMode: Image.PreserveAspectFit
                            anchors.fill: parent
                        }

                        background: Rectangle {
                            color: "white"
                            border.color: "white"
                            radius: leftColumnLayout.width * 0.005
                        }

                        onClicked: {
                            settingContent.visible = !settingContent.visible
                        }
                    }

                    Rectangle {
                        id: settingContent
                        visible: false
                        width: topRectangle.width * 0.30
                        height: topRectangle.height * 0.40
                        radius: 10
                        color:"#80000000"
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.margins: 20

                        //setting content here
                        PhotoSetting {
                            id: photoSetting
                            visible: buttonController.activeButtonNumber == 1
                            anchors.fill: parent
                            anchors.margins: 10

                        }

                        VideoSetting {
                            id: videoSetting
                            visible: buttonController.activeButtonNumber == 0
                            anchors.fill: parent
                            anchors.margins: 10

                        }

                        StreamSetting {
                            id: streamSetting
                            visible: buttonController.activeButtonNumber == 2
                            anchors.fill: parent
                            anchors.margins: 10

                        }
                    }

                    Button {
                        id: effectsButton
                        width: Math.max(40, parent.width * 0.02 * mainWindow.scaleFix)
                        height: width
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.margins: 20
                        visible: platform
                        contentItem: Image {
                            source: "qrc:/Image/effect.png"
                            fillMode: Image.PreserveAspectFit
                            anchors.fill: parent
                        }
                        //#62CDFF
                        background: Rectangle {
                            color: "white"
                            border.color: "white"
                            radius: leftColumnLayout.width * 0.005
                        }

                        onClicked: {
                            effectContent.visible = !effectContent.visible
                        }
                    }

                    Rectangle {
                        id: effectContent
                        visible: false
                        width: windowWidth > 640 ? topRectangle.width * 0.30 : topRectangle.width * 0.45
                        height: windowHeight > 480 ? topRectangle.height * 0.45 : topRectangle.height * 0.60
                        radius: 10
                        color:"#66000000"
                        //opacity: 0.1
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.margins: 20

                        EffectsContent {
                            anchors.fill: parent
                            anchors.margins: 10
                        }

                    }

                    Rectangle {
                        id: videoContent
                        width:parent.width
                        height: parent.height
                        color: "transparent"

                        property int secondsElapsed: 0

                        Timer {
                            id: timer
                            interval: 1000
                            running: false
                            repeat: true
                            onTriggered: {
                                videoContent.secondsElapsed +=1;
                            }
                        }

                        Rectangle {
                            id: videoTiming
                            visible: false
                            width: parent.width * 0.10
                            height: parent.height * 0.10
                            radius: 10
                            color: "transparent"
                            y: topRectangle.height - bottomRectangle.height - 50
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.margins: 10

                            RowLayout {
                                anchors.centerIn: parent
                                Layout.alignment: Qt.AlignCenter
                                Text {
                                    id: liveDot
                                    text: "."
                                    font.pixelSize: 50
                                    color: "red"
                                    font.bold: true
                                    visible: true
                                    Layout.bottomMargin: 30

//                                    SequentialAnimation on color {
//                                        id: videoTimingAnimation
//                                        loops: Animation.Infinite
//                                        running: false

//                                        ColorAnimation {
//                                            from: "red"
//                                            to: "transparent"
//                                            duration: 500
//                                        }
//                                        ColorAnimation {
//                                            from: "transparent"
//                                            to: "red"
//                                            duration: 200
//                                        }
//                                    }
                                }

                                Text {
                                    id: timerDisplay
                                    text: {
                                        let hours = Math.floor(videoContent.secondsElapsed / 3600).toString().padStart(2, '0');
                                        let minutes = Math.floor((videoContent.secondsElapsed % 3600) / 60).toString().padStart(2, '0');
                                        let seconds = Math.floor(videoContent.secondsElapsed % 60).toString().padStart(2, '0');
                                        hours + ":" + minutes + ":" + seconds
                                    }

                                    color: "white"
                                }

                            }
                        }
                    }

                    Rectangle {
                        id: streamContent
                        visible: false
                        width: topRectangle.width * 0.10
                        height: topRectangle.height * 0.10
                        y: topRectangle.height - bottomRectangle.height - 50
                        radius: 10
                        color: "transparent"
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.margins: 10

                        RowLayout {
                            anchors.centerIn: parent
                            Layout.alignment: Qt.AlignCenter
                            Text {
                                id: liveText
                                text: "STREAMING "
                                color: "red"
                                font.bold: true
                                visible: true

//                                SequentialAnimation on color {
//                                    id: streamAnimation
//                                    loops: Animation.Infinite
//                                    running: false

//                                    ColorAnimation {
//                                        from: "red"
//                                        to: "transparent"
//                                        duration: 500
//                                    }
//                                    ColorAnimation {
//                                        from: "transparent"
//                                        to: "red"
//                                        duration: 200
//                                    }
//                                }
                            }

                            Text {
                                text: qsTr("ON IP ") + server1.addr + qsTr(" PORT ") + server1.port
                                color: "white"
                                font.bold: true
                                visible: true
                            }
                        }
                    }
                }
            }
        }
    }
}

