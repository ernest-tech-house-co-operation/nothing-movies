import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#0d0d0d"

    signal resultSelected(var result)

    ListModel { id: resultsModel }

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
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            TextField {
                id: queryField
                Layout.fillWidth: true
                placeholderText: "Search movies..."
                color: "white"
                onAccepted: searchButton.clicked()
            }

            Button {
                id: searchButton
                text: "Search"
                onClicked: {
                    if (queryField.text.trim().length === 0) {
                        return
                    }
                    statusLabel.text = "Searching..."
                    statusLabel.visible = true
                    searchBridge.search(queryField.text.trim())
                }
            }
        }

        Text {
            id: statusLabel
            color: "#aaaaaa"
            visible: false
        }

        GridView {
            id: resultsGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            cellWidth: 180
            cellHeight: 280
            model: resultsModel
            clip: true

            delegate: Item {
                width: 160
                height: 274

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.resultSelected({
                        id: model.id,
                        sourceName: model.sourceName,
                        title: model.title,
                        posterUrl: model.posterUrl,
                        year: model.year,
                        matched: model.matched,
                        rawTitle: model.rawTitle
                    })
                }

                Column {
                    anchors.fill: parent
                    spacing: 6

                    Image {
                        width: 160
                        height: 220
                        fillMode: Image.PreserveAspectCrop
                        source: posterUrl.length > 0 ? posterUrl : ""
                        asynchronous: true

                        Rectangle {
                            visible: posterUrl.length === 0
                            anchors.fill: parent
                            color: "#222222"
                            Text {
                                anchors.centerIn: parent
                                text: "No poster"
                                color: "#888888"
                                font.pixelSize: 12
                            }
                        }
                    }

                    Text {
                        width: 160
                        text: title + (year > 0 ? " (" + year + ")" : "")
                        color: "white"
                        font.pixelSize: 14
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        width: 160
                        text: sourceName
                        color: "#888888"
                        font.pixelSize: 11
                    }
                }
            }
        }
    }
}