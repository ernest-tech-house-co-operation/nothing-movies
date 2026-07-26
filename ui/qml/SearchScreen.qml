import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#0d0d0d"

    signal resultSelected(var result)

    ListModel { id: resultsModel }

    // listen for homepageBridge.loadInfo() results triggered from search cards
    Connections {
        target: homepageBridge
        function onInfoReady(info) {
            root.resultSelected(info)
        }
    }

    Connections {
        target: searchBridge
        function onResultsReady(results) {
            resultsModel.clear()
            for (var i = 0; i < results.length; i++) {
                resultsModel.append(results[i])
            }
            statusLabel.text = results.length === 0
                ? "No results."
                : results.length + " result(s)"
            statusLabel.visible = true
        }
        function onSearchError(message) {
            statusLabel.text = "Search error: " + message
            statusLabel.visible = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── top bar ──
        Rectangle {
            Layout.fillWidth: true
            height: 64
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#1a1030" }
                GradientStop { position: 1.0; color: "#0f1230" }
            }
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: "#2a2450"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                spacing: 12

                // search field
                Rectangle {
                    Layout.fillWidth: true
                    height: 38
                    radius: 8
                    color: "#13131f"
                    border.color: queryField.activeFocus ? "#7c3aed" : "#2a2a3e"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 8

                        Text { text: "🔍"; font.pixelSize: 14; color: "#6b7280" }

                        TextInput {
                            id: queryField
                            Layout.fillWidth: true
                            color: "white"
                            font.pixelSize: 14
                            clip: true
                            onAccepted: doSearch()

                            Text {
                                anchors.fill: parent
                                text: "Search movies, series..."
                                color: "#4b5563"
                                font.pixelSize: 14
                                visible: queryField.text.length === 0
                            }
                        }

                        // clear button
                        Text {
                            text: "✕"
                            color: "#6b7280"
                            font.pixelSize: 13
                            visible: queryField.text.length > 0
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    queryField.text = ""
                                    resultsModel.clear()
                                    statusLabel.visible = false
                                }
                            }
                        }
                    }
                }

                // search button
                Rectangle {
                    width: 90
                    height: 38
                    radius: 8
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#7c3aed" }
                        GradientStop { position: 1.0; color: "#6d28d9" }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "Search"
                        color: "white"
                        font.pixelSize: 14
                        font.bold: true
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: doSearch()
                    }
                }
            }
        }

        // status
        Text {
            id: statusLabel
            Layout.fillWidth: true
            Layout.leftMargin: 24
            Layout.topMargin: 12
            color: "#6b7280"
            font.pixelSize: 13
            visible: false
        }

        // ── results grid ──
        GridView {
            id: resultsGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 16
            Layout.topMargin: 8
            cellWidth: 180
            cellHeight: 290
            model: resultsModel
            clip: true

            delegate: Item {
                width: 164
                height: 284

                Rectangle {
                    id: card
                    anchors.fill: parent
                    anchors.margins: 6
                    radius: 10
                    color: "#13131f"
                    border.color: cardHover.containsMouse ? "#7c3aed" : "#2a2a3e"
                    border.width: cardHover.containsMouse ? 2 : 1
                    clip: true

                    // poster
                    Image {
                        id: posterImg
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: parent.height * 0.72
                        fillMode: Image.PreserveAspectCrop
                        source: model.posterUrl && model.posterUrl.length > 0 ? model.posterUrl : ""
                        asynchronous: true
                        clip: true

                        Rectangle {
                            visible: !model.posterUrl || model.posterUrl.length === 0
                            anchors.fill: parent
                            color: "#1a1a2e"
                            Text {
                                anchors.centerIn: parent
                                text: "🎬"
                                font.pixelSize: 32
                            }
                        }
                    }

                    // gradient over bottom of poster
                    Rectangle {
                        anchors.bottom: posterImg.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 40
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#00000000" }
                            GradientStop { position: 1.0; color: "#ff13131f" }
                        }
                    }

                    // info below poster
                    ColumnLayout {
                        anchors.top: posterImg.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 8
                        spacing: 3

                        Text {
                            text: model.title || ""
                            color: "white"
                            font.pixelSize: 13
                            font.bold: true
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            spacing: 6
                            Text {
                                text: model.year > 0 ? model.year.toString() : ""
                                color: "#6b7280"
                                font.pixelSize: 11
                                visible: model.year > 0
                            }
                            Rectangle {
                                width: sourceLabel.implicitWidth + 10
                                height: 16
                                radius: 3
                                color: "#1f2937"
                                Text {
                                    id: sourceLabel
                                    anchors.centerIn: parent
                                    text: model.sourceName || ""
                                    color: "#a78bfa"
                                    font.pixelSize: 10
                                }
                            }
                        }
                    }

                    // hover overlay
                    Rectangle {
                        anchors.fill: parent
                        color: "#88000000"
                        visible: cardHover.containsMouse
                        radius: 10
                    }

                    // "View Info" button — only for hasInfo sources
                    Rectangle {
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: -10
                        width: card.width * 0.75
                        height: 34
                        radius: 8
                        color: "#7c3aed"
                        visible: cardHover.containsMouse && model.hasInfo === true
                        z: 10

                        Text {
                            anchors.centerIn: parent
                            text: "View Info"
                            color: "white"
                            font.pixelSize: 13
                            font.bold: true
                        }
                        MouseArea {
                            anchors.fill: parent
                            z: 10
                            onClicked: homepageBridge.loadInfo(model.id, model.sourceName)
                        }
                    }

                    // "Stream / Download" buttons for non-hasInfo sources on hover
                    ColumnLayout {
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: -10
                        spacing: 6
                        visible: cardHover.containsMouse && model.hasInfo !== true
                        z: 10

                        Rectangle {
                            width: card.width * 0.75
                            height: 32
                            radius: 8
                            color: "#7c3aed"
                            visible: model.hasStream === true
                            Layout.alignment: Qt.AlignHCenter

                            Text {
                                anchors.centerIn: parent
                                text: "▶  Stream"
                                color: "white"
                                font.pixelSize: 12
                                font.bold: true
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: root.resultSelected({
                                    id: model.id,
                                    sourceName: model.sourceName,
                                    title: model.title,
                                    posterUrl: model.posterUrl,
                                    year: model.year,
                                    matched: model.matched,
                                    rawTitle: model.rawTitle,
                                    hasStream: model.hasStream,
                                    hasDownload: model.hasDownload,
                                    streamType: model.streamType,
                                    downloadType: model.downloadType,
                                    hasInfo: false
                                })
                            }
                        }

                        Rectangle {
                            width: card.width * 0.75
                            height: 32
                            radius: 8
                            color: "#059669"
                            visible: model.hasDownload === true
                            Layout.alignment: Qt.AlignHCenter

                            Text {
                                anchors.centerIn: parent
                                text: "⬇  Download"
                                color: "white"
                                font.pixelSize: 12
                                font.bold: true
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: root.resultSelected({
                                    id: model.id,
                                    sourceName: model.sourceName,
                                    title: model.title,
                                    posterUrl: model.posterUrl,
                                    year: model.year,
                                    matched: model.matched,
                                    rawTitle: model.rawTitle,
                                    hasStream: model.hasStream,
                                    hasDownload: model.hasDownload,
                                    streamType: model.streamType,
                                    downloadType: model.downloadType,
                                    hasInfo: false
                                })
                            }
                        }
                    }

                    MouseArea {
                        id: cardHover
                        anchors.fill: parent
                        hoverEnabled: true
                    }
                }
            }
        }
    }

    function doSearch() {
        if (queryField.text.trim().length === 0) return
        statusLabel.text = "Searching..."
        statusLabel.visible = true
        resultsModel.clear()
        searchBridge.search(queryField.text.trim())
    }
}