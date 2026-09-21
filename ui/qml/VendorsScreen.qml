import QtQuick 2.15
import QtQuick.Layouts 1.15

Item {
    id: vendorsScreen

    ListModel { id: vendorsModel }
    property var statusByName: ({})

    function loadVendors() {
        vendorsModel.clear()
        if (typeof vendorBridge === "undefined" || !vendorBridge) return
        const vendors = vendorBridge.listVendors()
        for (var i = 0; i < vendors.length; i++) vendorsModel.append(vendors[i])
    }

    Component.onCompleted: loadVendors()

    Connections {
        target: typeof vendorBridge !== "undefined" ? vendorBridge : null
        function onUpdateStatus(vendorName, status) {
            var copy = Object.assign({}, vendorsScreen.statusByName)
            copy[vendorName] = status
            vendorsScreen.statusByName = copy
        }
        function onUpdateFinished(vendorName, updated, newTag, error) {
            var copy = Object.assign({}, vendorsScreen.statusByName)
            copy[vendorName] = error !== "" ? ("error: " + error)
                              : updated ? ("updated to " + newTag)
                              : "up to date"
            vendorsScreen.statusByName = copy
            vendorsScreen.loadVendors()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#0b0b12"
    }

    Flickable {
        anchors.fill: parent
        anchors.margins: 32
        contentHeight: mainColumn.implicitHeight
        clip: true

        ColumnLayout {
            id: mainColumn
            width: parent.width
            spacing: 20

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "External Tools"
                    color: "white"
                    font.pixelSize: 28
                    font.bold: true
                    Layout.fillWidth: true
                }
                Rectangle {
                    Layout.preferredWidth: 140
                    Layout.preferredHeight: 38
                    radius: 8
                    color: checkAllMouse.containsMouse ? "#8b5cf6" : "#7c3aed"
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Text {
                        anchors.centerIn: parent
                        text: "Check All"
                        color: "white"
                        font.pixelSize: 12
                        font.bold: true
                    }
                    MouseArea {
                        id: checkAllMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: vendorBridge.checkAll()
                    }
                }
            }

            Text {
                text: "Every tool below is an external binary the app downloads on your behalf " +
                      "(scraping engines, etc). Each one links to its own open-source repository " +
                      "so you can verify exactly what it does. All of them live under one folder " +
                      "on disk, kept separate from everything else."
                color: "#6f6a92"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Text {
                text: "No external tools registered yet."
                color: "#6f6a92"
                font.pixelSize: 13
                visible: vendorsModel.count === 0
            }

            Repeater {
                model: vendorsModel

                delegate: Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: cardColumn.implicitHeight + 32
                    radius: 12
                    color: "#131022"
                    border.width: 1
                    border.color: "#221c3a"

                    property string status: vendorsScreen.statusByName[model.name] || ""

                    ColumnLayout {
                        id: cardColumn
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: model.name; color: "white"; font.pixelSize: 16; font.bold: true }
                                Text {
                                    text: "Current: " + model.currentTag + (model.license ? "  ·  " + model.license : "")
                                    color: "#a1a1c9"
                                    font.pixelSize: 11
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: 130
                                Layout.preferredHeight: 34
                                radius: 8
                                color: checkMouse.containsMouse ? "#1c3a2e" : "#171225"
                                border.width: 1
                                border.color: "#22c55e"
                                Behavior on color { ColorAnimation { duration: 120 } }
                                Text {
                                    anchors.centerIn: parent
                                    text: "Check for updates"
                                    color: "white"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                                MouseArea {
                                    id: checkMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: vendorBridge.checkVendor(model.name)
                                }
                            }
                        }

                        Text {
                            text: "Source: " + model.sourceRepoUrl
                            color: "#8ab4f8"
                            font.pixelSize: 11
                            font.underline: true
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Qt.openUrlExternally(model.sourceRepoUrl)
                            }
                        }

                        Text {
                            text: status
                            color: status.indexOf("error") === 0 ? "#f87171"
                                 : status === "checking" ? "#fbbf24"
                                 : "#4ade80"
                            font.pixelSize: 11
                            visible: status !== ""
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }
}