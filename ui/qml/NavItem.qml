import QtQuick 2.15

Column {
    id: item
    property string label: ""
    property string icon: ""
    property bool selected: false
    signal clicked()

    spacing: 4
    width: 70

    Rectangle {
        width: 50; height: 50
        radius: 12
        anchors.horizontalCenter: parent.horizontalCenter
        color: selected ? "#2a2a2a" : "transparent"

        Text {
            anchors.centerIn: parent
            text: item.icon
            font.pixelSize: 20
        }

        MouseArea {
            anchors.fill: parent
            onClicked: item.clicked()
        }
    }

    Text {
        text: item.label
        color: selected ? "white" : "#777"
        font.pixelSize: 10
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
