import QtQuick 2.15

Rectangle {
    id: splash
    color: "#0d0d0d"
    signal finished()

    Text {
        id: logoText
        anchors.verticalCenter: parent.verticalCenter
        x: -width
        text: "Nothing Movies"
        color: "white"
        font.pixelSize: 32
        font.bold: true

        NumberAnimation on x {
            to: (splash.width - logoText.width) / 2
            duration: 450
            easing.type: Easing.OutCubic
        }
    }

    Timer {
        interval: 1200
        running: true
        onTriggered: splash.finished()
    }
}
