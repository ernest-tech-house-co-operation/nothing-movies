import QtQuick 2.15

Rectangle {
    id: row
    height: 96
    radius: 10
    color: hovered ? "#211a3d" : "#171225"
    border.width: hovered ? 1 : 0
    border.color: "#7c3aed"

    Behavior on color { ColorAnimation { duration: 150 } }

    property string movieTitle: ""
    property string movieYear: ""
    property string posterUrl: ""
    property bool hovered: false

    signal clicked()

    // accent stripe on the left, brightens on hover
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 4
        radius: 2
        gradient: Gradient {
            GradientStop { position: 0.0; color: hovered ? "#a78bfa" : "#7c3aed" }
            GradientStop { position: 1.0; color: hovered ? "#22d3ee" : "#06b6d4" }
        }
    }

    Rectangle {
        id: poster
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        width: 60
        height: 80
        radius: 6
        color: "#2a2a3a"
        clip: true

        Image {
            anchors.fill: parent
            source: row.posterUrl
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            visible: row.posterUrl !== ""
        }

        Text {
            anchors.centerIn: parent
            text: "🎬"
            font.pixelSize: 22
            visible: row.posterUrl === ""
        }
    }

    Column {
        anchors.left: poster.right
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4

        Text {
            text: row.movieTitle
            color: "white"
            font.pixelSize: 15
            font.bold: true
            elide: Text.ElideRight
        }

        Text {
            text: row.movieYear
            color: "#a1a1c9"
            font.pixelSize: 12
        }
    }

    scale: hovered ? 1.01 : 1.0
    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: row.hovered = true
        onExited: row.hovered = false
        onClicked: row.clicked()
    }
}
