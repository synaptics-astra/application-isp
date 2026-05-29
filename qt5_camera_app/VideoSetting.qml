import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15


ColumnLayout {
    id: videoSettingContent
    spacing: 10
    width: parent.width - 5

    Label {
        text: "Video Settings"
        font.bold: true
        Layout.alignment: Qt.AlignVCenter
        color: "white"
    }

    RowLayout {
        Label {
            text: "Name"
            font.bold: true
            Layout.alignment: Qt.AlignVCenter
            color: "white"
        }
        TextField {
            id: videoNameField
            placeholderText: "Enter video name"
            Layout.fillWidth: true
            background: Rectangle {
                color: "white"
                border.color: "black"
                opacity: 0.5
                border.width: 1
                radius: 5
            }
        }
    }

    RowLayout {
        Label {
            text: "File Type"
            font.bold: true
            Layout.alignment: Qt.AlignVCenter
            color: "white"
        }
        ComboBox {
            id: videoFileType
            model: ["mp4"]
            currentIndex: 0
            Layout.fillWidth: true

            background: Rectangle {
                color: "white"
                border.color: "black"
                opacity: 0.5
                border.width: 1
                radius: 5
            }
        }
    }

    Rectangle {
        color: "transparent"
        Layout.fillHeight: true
    }

    Button {
        text: "Apply"
        font.bold: true
        background: Rectangle {
            color: "white"
            opacity: 0.5
            radius: 10

        }
        Layout.fillWidth: true
        onClicked: {
            settingContent.visible = false
            captureVideo.filename = videoNameField.text;
        }
    }
}
