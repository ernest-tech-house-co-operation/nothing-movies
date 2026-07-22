import QtQuick 2.15

Rectangle {
    id: splash
    color: "#0d0d0d"
    signal finished()

    Text {
        anchors.centerIn: parent
        text: "Nothing Movies"
        color: "white"
        font.pixelSize: 32
        font.bold: true
    }

    Timer {
        interval: 1200
        running: true
        onTriggered: splash.finished()
    }
}
