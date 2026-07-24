import QtQuick 2.15

Rectangle {
    id: card
    color: mouseArea.containsMouse ? "#231b3f" : "#181425"
    radius: 8
    border.width: mouseArea.containsMouse ? 1 : 0
    border.color: "#a78bfa"

    Behavior on color { ColorAnimation { duration: 150 } }

    property string movieTitle: ""
    property string movieYear: ""
    property string posterUrl: ""

    scale: mouseArea.containsMouse ? 1.045 : 1.0
    Behavior on scale { NumberAnimation { duration: 130; easing.type: Easing.OutQuad } }

    Rectangle {
        id: poster
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 6
        height: parent.height * 0.72
        radius: 6
        color: "#2a2a3a"
        clip: true

        Image {
            anchors.fill: parent
            source: card.posterUrl
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            visible: card.posterUrl !== ""
        }

        Text {
            anchors.centerIn: parent
            text: "🎬"
            font.pixelSize: 32
            visible: card.posterUrl === ""
        }

        // colorful gradient wash on hover
        Rectangle {
            anchors.fill: parent
            visible: mouseArea.containsMouse
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 1.0; color: "#557c3aed" }
            }
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
        color: "#a1a1c9"
        font.pixelSize: 11
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: console.log("Selected:", card.movieTitle)
    }
}
