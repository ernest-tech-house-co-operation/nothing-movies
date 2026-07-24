import QtQuick 2.15

Item {
    id: item
    property string label: ""
    property string icon: ""
    property bool selected: false
    signal clicked()

    width: row.implicitWidth + 28
    height: 40

    property bool hovered: false

    Rectangle {
        id: pill
        anchors.fill: parent
        radius: 20
        color: item.selected ? "#7c3aed" : (item.hovered ? "#2a2140" : "transparent")

        Behavior on color { ColorAnimation { duration: 150 } }

        // subtle glow underline for the selected item
        Rectangle {
            visible: item.selected
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -6
            width: parent.width * 0.5
            height: 3
            radius: 2
            color: "#a78bfa"
        }
    }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 8

        Text {
            text: item.icon
            font.pixelSize: 16
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: item.label
            color: item.selected ? "white" : (item.hovered ? "#e5e0ff" : "#9a94b8")
            font.pixelSize: 13
            font.bold: item.selected
            anchors.verticalCenter: parent.verticalCenter

            Behavior on color { ColorAnimation { duration: 150 } }
        }
    }

    scale: item.hovered ? 1.06 : 1.0
    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: item.hovered = true
        onExited: item.hovered = false
        onClicked: item.clicked()
    }
}
