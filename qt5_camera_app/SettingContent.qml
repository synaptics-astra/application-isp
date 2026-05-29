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
