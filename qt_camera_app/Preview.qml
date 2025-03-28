import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ImagePreview 1.0

Rectangle {
    id: previewRectangle

    property bool clicked: false

    ImagePreview {
        id: imagePath
    }

    Button {
        id: previewButton
        width: bottomRectangle.width * 0.04
        height: bottomRectangle.width * 0.04
        anchors.right: topRectangle.right
        anchors.bottom: topRectangle.bottom
        anchors.margins: 20

        background: Rectangle {
            color: "transparent"
        }

        contentItem: Image {
            id: gallery
            source: "qrc:/Image/gallery.png"
            fillMode: Image.PreserveAspectFit
            anchors.fill: parent
            anchors.margins: 8
        }



        onClicked: {
            previewRectangle.clicked = !previewRectangle.clicked;
            var latestFile = imagePath.getLatestMediaFilePath("/tmp/");

            if (previewRectangle.clicked) {
                gallery.source = "qrc:/Image/back.png"
            } else {
                gallery.source = "qrc:/Image/gallery.png"
            }

            if (latestFile !== "") {
                if (imagePath.isImageFile(latestFile)) {
                    latestFile = "file://"+latestFile;
                    previewImage.source = latestFile;
                    previewImage.visible = !previewImage.visible;
                }else if (imagePath.isVideoFile(latestFile)){
                    if (previewRectangle.clicked) {
                        videoPlayer.setSource(latestFile, cameraFps);
                    } else {
                        videoPlayer.stop();
                    }

                    previewVideo.visible = !previewVideo.visible;                    
                }
            }

            cameraPlayer.visible = !cameraPlayer.visible;
            settingButton.visible = !settingButton.visible;
            effectsButton.visible = !effectsButton.visible;
            buttonContainer.visible = !buttonContainer.visible;
            exitButton.visible = !exitButton.visible;
        }
    }
}
