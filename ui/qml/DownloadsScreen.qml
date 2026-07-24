import QtQuick 2.15
import QtQuick.Layouts 1.15

Item {
    id: downloadsScreen

    ListModel { id: itemsModel }

    Connections {
        target: queueBridge
        function onItemsReady(items) {
            itemsModel.clear()
            for (var i = 0; i < items.length; i++) itemsModel.append(items[i])
        }
    }

    Component.onCompleted: queueBridge.refresh()

    Rectangle {
        anchors.fill: parent
        color: "#0b0b12"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 16

        Text {
            text: "Downloads"
            color: "white"
            font.pixelSize: 28
            font.bold: true
        }

        Text {
            text: "Saved to " + (queueBridge ? queueBridge.downloadFolder() : "") +
                  " — downloads keep going in the background even if you leave this screen or stop watching."
            color: "#6f6a92"
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Text {
            text: "Nothing here yet. Start a stream or download from Search."
            color: "#8b86a8"
            font.pixelSize: 13
            visible: itemsModel.count === 0
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: list.implicitHeight
            clip: true

            ColumnLayout {
                id: list
                width: parent.width
                spacing: 10

                Repeater {
                    model: itemsModel

                    delegate: Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 72
                        radius: 10
                        color: rowMouse.containsMouse ? "#211a3d" : "#171225"

                        property bool finished: model.state === "Finished"

                        Behavior on color { ColorAnimation { duration: 150 } }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: 4
                            radius: 2
                            color: finished ? "#22c55e" : "#06b6d4"
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 20
                            anchors.rightMargin: 20
                            spacing: 16

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Text {
                                    text: model.title
                                    color: "white"
                                    font.pixelSize: 15
                                    font.bold: true
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                RowLayout {
                                    spacing: 10
                                    visible: !finished

                                    Rectangle {
                                        Layout.preferredWidth: 160
                                        Layout.preferredHeight: 6
                                        radius: 3
                                        color: "#2a2a3a"

                                        Rectangle {
                                            width: parent.width * model.progress
                                            height: parent.height
                                            radius: 3
                                            color: "#7c3aed"
                                        }
                                    }

                                    Text {
                                        text: model.state + " · " + Math.round(model.progress * 100) + "%"
                                        color: "#a1a1c9"
                                        font.pixelSize: 11
                                    }
                                }

                                Text {
                                    text: "Ready to play"
                                    color: "#4ade80"
                                    font.pixelSize: 11
                                    visible: finished
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: 84
                                Layout.preferredHeight: 34
                                radius: 8
                                visible: model.readyToPlay
                                color: playMouse.containsMouse ? "#8b5cf6" : "#7c3aed"

                                Behavior on color { ColorAnimation { duration: 120 } }

                                Text {
                                    anchors.centerIn: parent
                                    text: "▶ Play"
                                    color: "white"
                                    font.pixelSize: 12
                                    font.bold: true
                                }

                                MouseArea {
                                    id: playMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: appController.startStream(model.title, model.filePath)
                                }
                            }
                        }

                        MouseArea {
                            id: rowMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }
                }
            }
        }
    }
}
