import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    id: photSettingContent
    spacing: 10
    width: parent.width - 5
    Label {
        text: "Photo Settings"
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
            id: photoNameField
            height: 40
            implicitHeight: 40
            placeholderText: "Enter photo name"
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
            id: photoFileType
        model: ["png", "jpeg"]
        currentIndex: 0
        Layout.fillWidth: true

        height: 40
        implicitHeight: 40

        contentItem: Text {
              text: photoFileType.displayText
          verticalAlignment: Text.AlignVCenter
          leftPadding: 10
          color: "black"
          elide: Text.ElideRight
        }

           background: Rectangle {
               color: "white"
           border.color: "black"
           border.width: 1
           opacity: 0.5
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
            photoCapture.filename = photoNameField.text;
            photoCapture.photoFormat = photoFileType.currentText;
        }
    }
}
