import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

Item {
    id: settingOptions
    property int option : 1

    onSendActiveButtonNumber: {
        option = buttonNumber
    }

    PhotoSetting {
        id: photoContent
        visible: option == 0
    }

    VideoSetting {
        id: videoContent
        visible: option == 1
    }

    StreamSetting {
        id: streamContent
        visible: option == 2
    }
}



//ColumnLayout {
//    spacing: 10
//    TabBar {
//        id: tabBarLeft
//        Layout.fillWidth: true
//        Layout.fillHeight: true
//        currentIndex: 0

//        TabButton {
//            id: photo
//            font.bold: true
//            onClicked: stackLayoutLeft.currentIndex = 0

//            Text {
//                text: qsTr("Photo")
//                color: tabBarLeft.currentIndex == 0 ? "white" : "grey"
//                font.bold: tabBarLeft.currentIndex == 0 ? true : false
//                anchors.centerIn: parent
//            }

//            width: tabBarLeft.currentIndex == 0 ? 100 : 90
//            height: tabBarLeft.currentIndex == 0 ? 50 : 40
//            background: Rectangle {
//                color: "black"
//                opacity: 0.5
//            }
//        }

//        TabButton {
//            id: video

//            onClicked: stackLayoutLeft.currentIndex = 1

//            Text {
//                text: qsTr("Video")
//                color: tabBarLeft.currentIndex == 1 ? "white" : "grey"
//                font.bold: tabBarLeft.currentIndex == 1 ? true : false
//                anchors.centerIn: parent
//            }

//            width: tabBarLeft.currentIndex == 1 ? 100 : 90
//            height: tabBarLeft.currentIndex == 1 ? 50 : 40
//            background: Rectangle {
//                color: "black"
//                opacity: 0.5
//            }
//        }


////        TabButton {
////            id: rtp
////            text: "RTP-Stream"
////            font.bold: true
////            onClicked: stackLayoutLeft.currentIndex = 2

////            width: 90
////            height: 30
////        }
//    }

////    StackLayout {
////        id: stackLayoutLeft
////        Layout.fillWidth: true
////        Layout.fillHeight: true
////        currentIndex: 0

////        Loader {
////            source: "PhotoSetting.qml"
////            Layout.fillWidth: true
////            Layout.fillHeight: true
////        }

////        Loader {
////            source: "VideoSetting.qml"
////            Layout.fillWidth: true
////            Layout.fillHeight: true
////        }

////        /*
////        Loader  {
////            source: "RTPStream.qml"
////            Layout.fillHeight: true
////            Layout.fillWidth: true
////        }
////        */
////    }
//}
