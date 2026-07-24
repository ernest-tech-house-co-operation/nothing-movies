import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#0d0d0d"

    // Set by whoever pushes this onto the stack:
    // { id, sourceName, title, posterUrl, year, matched, rawTitle }
    property var result: ({})

    property bool resolving: false
    property string pendingAction: ""

    signal backRequested()

    Connections {
        target: searchBridge
        function onStreamUrlReady(title, url) {
            if (!resolving) return
            resolving = false
            if (pendingAction === "download") {
                const ok = queueBridge.enqueue(title, url)
                statusLabel.color = ok ? "#4ade80" : "#ff8080"
                statusLabel.text = ok ? "Added to Downloads." : "Couldn't start download."
                statusLabel.visible = true
            } else {
                appController.startStream(title, url)
            }
            pendingAction = ""
        }
        function onStreamUrlError(message) {
            if (!resolving) return
            resolving = false
            pendingAction = ""
            statusLabel.color = "#ff8080"
            statusLabel.text = "Error: " + message
            statusLabel.visible = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Button {
            text: "< Back"
            onClicked: root.backRequested()
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 24

            Image {
                Layout.preferredWidth: 260
                Layout.preferredHeight: 380
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                source: (result.posterUrl && result.posterUrl.length > 0) ? result.posterUrl : ""

                Rectangle {
                    visible: !result.posterUrl || result.posterUrl.length === 0
                    anchors.fill: parent
                    color: "#222222"
                    Text {
                        anchors.centerIn: parent
                        text: "No poster"
                        color: "#888888"
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 12

                Text {
                    text: result.title || ""
                    color: "white"
                    font.pixelSize: 28
                    font.bold: true
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Text {
                    text: (result.year && result.year > 0 ? result.year + " · " : "") + (result.sourceName || "")
                    color: "#888888"
                    font.pixelSize: 14
                }

                RowLayout {
                    spacing: 10

                    Button {
                        text: resolving && pendingAction === "stream" ? "Resolving..." : "Stream"
                        enabled: !resolving
                        onClicked: {
                            pendingAction = "stream"
                            resolving = true
                            statusLabel.visible = false
                            searchBridge.getStreamUrl(result.id, result.sourceName, result.title)
                        }
                    }

                    Button {
                        text: resolving && pendingAction === "download" ? "Resolving..." : "⬇ Download"
                        enabled: !resolving
                        onClicked: {
                            pendingAction = "download"
                            resolving = true
                            statusLabel.visible = false
                            searchBridge.getStreamUrl(result.id, result.sourceName, result.title)
                        }
                    }
                }

                Text {
                    id: statusLabel
                    color: "#ff8080"
                    visible: false
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}
