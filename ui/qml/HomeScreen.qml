import QtQuick 2.15
import QtQuick.Layouts 1.15

Item {
    id: homeScreen

    signal infoRequested(var info)

    ListModel { id: homepageModel }

    property string heroBackdrop: ""
    property string heroTitle: ""
    property string heroYear: ""
    property string heroGenre: ""
    property double heroRating: 0
    property string currentSourceName: ""
    property string fallbackMessage: ""
    property bool dataLoaded: false

    function populateFromItems(items, sourceName) {
        homepageModel.clear()
        for (var i = 0; i < items.length; i++) homepageModel.append(items[i])
        currentSourceName = sourceName
        if (items.length > 0) {
            heroBackdrop = items[0].backdropUrl || items[0].posterUrl || ""
            heroTitle    = items[0].title || ""
            heroYear     = items[0].year ? items[0].year.toString() : ""
            heroGenre    = items[0].genre || ""
            heroRating   = items[0].rating || 0
        }
        dataLoaded = true
    }

    Connections {
        target: homepageBridge
        function onHomepageReady(items, sourceName, isFallback) {
            homeScreen.populateFromItems(items, sourceName)
            fallbackMessage = isFallback ? "Showing fallback data" : ""
        }
        function onHomepageError(message) {
            console.warn("HomepageBridge error:", message)
        }
        function onInfoReady(info) {
            homeScreen.infoRequested(info)
        }
        function onInfoError(message) {
            console.warn("HomepageBridge info error:", message)
        }
    }

    Component.onCompleted: {
        homepageBridge.loadHomepage()
    }

    Rectangle {
        anchors.fill: parent
        color: "#0b0b12"
    }

    Flickable {
        anchors.fill: parent
        contentHeight: mainColumn.implicitHeight
        clip: true

        ColumnLayout {
            id: mainColumn
            width: parent.width
            spacing: 28

            // ── Hero banner ──
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 340

                Image {
                    anchors.fill: parent
                    source: heroBackdrop
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    visible: heroBackdrop !== ""
                }
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#000b0b12" }
                        GradientStop { position: 0.55; color: "#880b0b18" }
                        GradientStop { position: 1.0; color: "#ff0b0b12" }
                    }
                }
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#883018a8" }
                        GradientStop { position: 0.45; color: "#00000000" }
                    }
                }
                ColumnLayout {
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    anchors.margins: 32
                    spacing: 6
                    Text { text: "Nothing Movies"; color: "#c4b5fd"; font.pixelSize: 13; font.bold: true; font.letterSpacing: 2 }
                    Text { text: heroTitle; color: "white"; font.pixelSize: 36; font.bold: true; visible: heroTitle !== "" }
                    RowLayout {
                        spacing: 12
                        Text { text: heroYear; color: "#d8d3ff"; font.pixelSize: 14; visible: heroYear !== "" }
                        Text { text: heroGenre; color: "#a78bfa"; font.pixelSize: 14; visible: heroGenre !== "" }
                        RowLayout {
                            spacing: 4
                            visible: heroRating > 0
                            Text { text: "★"; color: "#f59e0b"; font.pixelSize: 14 }
                            Text { text: heroRating.toFixed(1); color: "#fcd34d"; font.pixelSize: 14; font.bold: true }
                        }
                    }
                }

                // loading indicator
                Text {
                    anchors.centerIn: parent
                    text: "Loading..."
                    color: "#6b7280"
                    font.pixelSize: 18
                    visible: !dataLoaded && heroTitle === ""
                }
            }

            // ── Trending from source ──
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 32
                Layout.rightMargin: 32
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: currentSourceName !== "" ? currentSourceName + " — Trending" : "Trending"
                        color: "white"
                        font.pixelSize: 20
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: fallbackMessage
                        color: "#fbbf24"
                        font.pixelSize: 13
                        visible: fallbackMessage !== ""
                    }
                }

                Grid {
                    id: movieGrid
                    columns: 5
                    spacing: 12
                    width: parent.width

                    Repeater {
                        model: homepageModel
                        delegate: Rectangle {
                            width: (movieGrid.width - 4 * 12) / 5
                            height: width * 1.5
                            radius: 10
                            color: "#13131f"
                            border.color: hoverArea.containsMouse ? "#7c3aed" : "#1e1e30"
                            border.width: hoverArea.containsMouse ? 2 : 1
                            clip: true

                            Image {
                                anchors.fill: parent
                                source: model.posterUrl || ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                            }

                            Rectangle {
                                anchors.fill: parent
                                color: "#aa000000"
                                visible: hoverArea.containsMouse
                                radius: 10
                            }

                            Rectangle {
                                anchors.centerIn: parent
                                width: parent.width * 0.72
                                height: 34
                                radius: 7
                                color: "#7c3aed"
                                visible: hoverArea.containsMouse && (model.hasInfo === true)
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

                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 60
                                radius: 10
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#00000000" }
                                    GradientStop { position: 1.0; color: "#f0000000" }
                                }
                            }
                            ColumnLayout {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 8
                                spacing: 2
                                Text {
                                    text: model.title || ""
                                    color: "white"
                                    font.pixelSize: 12
                                    font.bold: true
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                RowLayout {
                                    spacing: 4
                                    visible: model.rating > 0
                                    Text { text: "★"; color: "#f59e0b"; font.pixelSize: 10 }
                                    Text { text: model.rating ? model.rating.toFixed(1) : ""; color: "#d1d5db"; font.pixelSize: 10 }
                                    Text { text: model.year ? "· " + model.year : ""; color: "#9ca3af"; font.pixelSize: 10 }
                                }
                            }

                            MouseArea {
                                id: hoverArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    if (model.hasInfo) homepageBridge.loadInfo(model.id, model.sourceName)
                                }
                            }
                        }
                    }
                }

                Item { Layout.preferredHeight: 24 }
            }
        }
    }
}