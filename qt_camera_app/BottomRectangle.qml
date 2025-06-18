import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

RowLayout {
    id: buttonRow
    visible: true
    anchors.centerIn: parent
    spacing: 20

    property int photoButtonClickCount: 0
    property int videoButtonClickCount: 0
    property int streamButtonClickCount: 0


    property int closedWidthHeight: buttonContainer.height * 0.6
    property int activeWidthHeight: buttonContainer.height * 0.8
    signal sendActiveButtonNumber(int buttonNumber)

    function setVisibility(isVisible)
    {
        exitButton.visible = isVisible;
        settingButton.visible = isVisible;
        previeButton.visible = isVisible;
    }

    //closed video button----------------------------------------------------------------
    Button {
        id: closedVideoButton
        visible: true
        Layout.preferredWidth:closedWidthHeight
        Layout.preferredHeight:closedWidthHeight

        background: Rectangle {
            color: "transparent"
        }


        contentItem: Image {
            id: videoImage
            source: "qrc:/Image/video-white.png"
            fillMode: Image.PreserveAspectFit
            anchors.fill: parent
            anchors.margins: 8
        }

        onClicked : {
            closedPhotoButton.visible = true
            activePhotoButton.visible = false

            closedVideoButton.visible = false
            activeVideoButton.visible = true

            closedStreamButton.visible = true
            activeStreamButton.visible = false

            //videoTimingAnimation.running = false

            buttonContainer.width = bottomRectangle.width * 0.4;

            sendActiveButtonNumber(0)
        }
    }

    Button {
        id: activeVideoButton
        visible: false
        Layout.preferredWidth:activeWidthHeight
        Layout.preferredHeight:activeWidthHeight

        background: Rectangle {
            id: videoActive
            color: "transparent"
            border.color: "white"
            border.width: 3
            radius: activeWidthHeight/2
        }

        contentItem: Image {
            id: activeVideoButtonSource
            source: videoButtonClickCount%2 === 0 ? "qrc:/Image/blue-circle.png" : "qrc:/Image/blue-square.png"

            fillMode: Image.PreserveAspectFit
            anchors.fill: parent
            anchors.margins: 8
        }

        onClicked: {
            streamButtonClickCount = 0;
            videoButtonClickCount += 1;
            photoButtonClickCount = 0;

            if (videoButtonClickCount % 2 == 1) {
                closedPhotoButton.visible = false
                closedStreamButton.visible = false
                activeVideoButtonSource.anchors.margins = 14
                timer.start()
                timer.running = true
                videoTiming.visible = true
                //videoTimingAnimation.running = true
                captureVideo.start = 1

                buttonContainer.width = bottomRectangle.height * 0.7;
                setVisibility(false);

            } else {
                closedPhotoButton.visible = true
                closedStreamButton.visible = true
                activeVideoButtonSource.anchors.margins = 5
                timer.stop()
                timer.running = false
                videoTiming.visible = false
                videoContent.secondsElapsed = 0
                //videoTimingAnimation.running = false

                buttonContainer.width = bottomRectangle.width * 0.4;

                captureVideo.start = 0
                setVisibility(true);
            }

            sendActiveButtonNumber(0);
        }
    }

    //closed photo button-----------------------------------------------------------------
    Button {
        id: closedPhotoButton
        visible: false
        Layout.preferredWidth: closedWidthHeight
        Layout.preferredHeight: closedWidthHeight

        background: Rectangle {
            id: closedPhotoRectangle
            color: "transparent"
            radius: closedWidthHeight/2
        }

        contentItem: Image {
            id: closedPhotoImage
            source: "qrc:/Image/photo-white.png"
            fillMode: Image.PreserveAspectFit
            anchors.fill: parent
            anchors.margins: 8
        }

        onClicked : {
            closedPhotoButton.visible = false
            activePhotoButton.visible = true

            closedVideoButton.visible = true
            activeVideoButton.visible = false

            closedStreamButton.visible = true
            activeStreamButton.visible = false

            streamButtonClickCount = 0

            sendActiveButtonNumber(1)
        }
    }

    //active photo button---------------------------------------------------------------
    Button {
        id: activePhotoButton
        visible: true
        Layout.preferredWidth: activeWidthHeight
        Layout.preferredHeight: activeWidthHeight

        background: Rectangle {
            id: activePhotoRectangle
            color: "#62CDFF"
            border.color: "white"
            border.width: 3
            radius: activeWidthHeight/2
        }

        contentItem: Image {
            id: activePhotoImage
            source: "qrc:/Image/photo-white.png"
            fillMode: Image.PreserveAspectFit
            anchors.fill: parent
            anchors.margins: 15
        }

        onClicked: {
            sendActiveButtonNumber(1)

            streamButtonClickCount = 0;
            videoButtonClickCount = 0;
            photoButtonClickCount += 0;
            photoCapture.capture = 1;

            captureAnimation.start();
            cameraOverlay.visible = true;
        }
    }

    //closed stream button----------------------------------------------------------------------
    Button {
        id: closedStreamButton
        visible: true
        Layout.preferredWidth: closedWidthHeight
        Layout.preferredHeight: closedWidthHeight

        background: Rectangle {
            color: "transparent"
        }

        contentItem: Image {
            id: streamImage
            source: "qrc:/Image/live-white.png"
            fillMode: Image.PreserveAspectFit
            anchors.fill: parent
            anchors.margins: 8
        }

        onClicked: {
            closedPhotoButton.visible = true
            activePhotoButton.visible = false

            closedVideoButton.visible = true
            activeVideoButton.visible = false

            closedStreamButton.visible = false
            activeStreamButton.visible = true

            //streamAnimation.running = false
            buttonContainer.width = bottomRectangle.width * 0.4;

            sendActiveButtonNumber(2)
        }
    }

    //active stream button
    Button {
        id: activeStreamButton
        visible: false
        Layout.preferredWidth: activeWidthHeight
        Layout.preferredHeight: activeWidthHeight

        background: Rectangle {
            id: streamActive
            color: "transparent"
            border.color: "white"
            border.width: 3
            radius: activeWidthHeight/2
        }

        contentItem: Image {
            id: streamActiveImage
            source: streamButtonClickCount%2 === 0 ? "qrc:/Image/blue-circle.png" : "qrc:/Image/blue-stop.png"
            Text {
                id: activeStreamText
                text: qsTr("Live")
                font.bold: true
                color: "white"
                anchors.centerIn: parent
            }

            fillMode: Image.PreserveAspectFit
            anchors.fill: parent
            anchors.margins: 8
        }

        onClicked: {
            streamButtonClickCount += 1;
            videoButtonClickCount = 0;
            photoButtonClickCount = 0;

            if (streamButtonClickCount % 2 == 1) {
                closedPhotoButton.visible = false
                closedVideoButton.visible = false
                activeStreamText.text = ""
                server1.start = 1
                streamContent.visible = true
                streamActiveImage.anchors.margins = 10
                //streamAnimation.running = true
                buttonContainer.width = bottomRectangle.height * 0.7;
                setVisibility(false);
            } else {
                closedPhotoButton.visible = true
                closedVideoButton.visible = true
                activeStreamText.text = "Live"
                server1.start = 0
                streamContent.visible = false
                streamActiveImage.anchors.margins = 5
                //streamAnimation.running = false

                buttonContainer.width = bottomRectangle.width * 0.4;
                setVisibility(true);
            }

            sendActiveButtonNumber(2);
        }
    }
}
