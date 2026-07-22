import QtQuick 2.15

Rectangle {
    id: card
    color: "#1a1a1a"
    radius: 8

    property string movieTitle: ""
    property string movieYear: ""

    Rectangle {
        id: poster
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 6
        height: parent.height * 0.72
        radius: 6
        color: "#2a2a2a"

        Text {
            anchors.centerIn: parent
            text: "🎬"
            font.pixelSize: 32
        }
    }

    Text {
        anchors.top: poster.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 8
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        text: card.movieTitle
        color: "white"
        font.pixelSize: 13
        elide: Text.ElideRight
    }

    Text {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 8
        text: card.movieYear
        color: "#888"
        font.pixelSize: 11
    }

    MouseArea {
        anchors.fill: parent
        onClicked: console.log("Selected:", card.movieTitle)
    }
}