import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    id: streamSettingContent
    spacing: 10
    width: parent.width - 5
    Label {
        text: "Stream Setting"
        font.bold: true
        Layout.alignment: Qt.AlignVCenter
        color: "white"
    }

    RowLayout {
        Label {
            text: "Remote IP"
            font.bold: true
            Layout.alignment: Qt.AlignVCenter
            color: "white"
        }
        TextField {
            id: remoteIpField
            placeholderText: "Enter remote IP"
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
            text: "Port"
            font.bold: true
            Layout.alignment: Qt.AlignVCenter
            color: "white"
        }
        TextField {
            id: remotePortField
            placeholderText: "Enter port number"
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
            text: "Bitrate"
            font.bold: true
            Layout.alignment: Qt.AlignVCenter
            color: "white"
        }
        TextField {
            id: remoteBitrateField
            placeholderText: "Enter bitrate"
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
            var portNumber = parseInt(remotePortField.text);
            if (!isNaN(portNumber) && portNumber >= 0 && portNumber <= 65535) {
                server1.port = portNumber;
            } else {
                console.log("Invalid port number.");
                server1.port = 8000;
            }

            var bitrate = parseInt(remoteBitrateField.text);
            if (!isNaN(bitrate) && bitrate > 0 && bitrate <= 62500) {
                server1.bitrate = bitrate;
            } else {
                console.log("Invalid bitrate.");
                server1.bitrate = 1000;
            }

            server1.addr = remoteIpField.text
        }
    }
}
