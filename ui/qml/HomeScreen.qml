import QtQuick 2.15
import QtQuick.Layouts 1.15

Item {
    id: homeScreen

    signal infoRequested(var info)

    ListModel { id: trendingModel }
    ListModel { id: upcomingModel }
    ListModel { id: homepageModel }

    property string heroBackdrop: ""
    property string heroTitle: ""
    property string heroYear: ""
    property string fallbackMessage: ""
    property string currentSourceName: ""

    Connections {
        target: tmdbBridge
        function onTrendingReady(movies) {
            trendingModel.clear()
            for (var i = 0; i < movies.length; i++) trendingModel.append(movies[i])
            if (movies.length > 0) {
                heroBackdrop = movies[0].backdropUrl || movies[0].posterUrl
                heroTitle = movies[0].title
                heroYear = movies[0].year.toString()
            }
        }
        function onUpcomingReady(movies) {
            upcomingModel.clear()
            for (var i = 0; i < movies.length; i++) upcomingModel.append(movies[i])
        }
    }

    Connections {
        target: homepageBridge
        function onHomepageReady(items, sourceName, isFallback) {
            homepageModel.clear()
            for (var i = 0; i < items.length; i++) homepageModel.append(items[i])
            currentSourceName = sourceName
            fallbackMessage = isFallback
                ? sourceName + " has no homepage — showing TMDB trending data"
                : ""
        }
        function onHomepageError(message) {
            console.warn("HomepageBridge error:", message)
            fallbackMessage = ""
        }
        function onInfoReady(info) {
            homeScreen.infoRequested(info)
        }
        function onInfoError(message) {
            console.warn("HomepageBridge info error:", message)
        }
    }

    Component.onCompleted: {
        tmdbBridge.loadTrending()
        tmdbBridge.loadUpcoming()
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
                Layout.preferredHeight: 320

                Image {
                    anchors.fill: parent
                    source: homeScreen.heroBackdrop
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    visible: homeScreen.heroBackdrop !== ""
                }
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#000a0612" }
                        GradientStop { position: 0.55; color: "#800b0b18" }
                        GradientStop { position: 1.0; color: "#ff0b0b12" }
                    }
                }
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#663018a8" }
                        GradientStop { position: 0.4; color: "#00000000" }
                    }
                }
                ColumnLayout {
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    anchors.margins: 32
                    spacing: 8
                    Text { text: "Nothing Movies"; color: "#c4b5fd"; font.pixelSize: 14; font.bold: true; font.letterSpacing: 2 }
                    Text { text: homeScreen.heroTitle; color: "white"; font.pixelSize: 34; font.bold: true; visible: homeScreen.heroTitle !== "" }
                    Text { text: homeScreen.heroYear; color: "#d8d3ff"; font.pixelSize: 15; visible: homeScreen.heroYear !== "" }
                }
            }

            // ── Trending Grid ──
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 32
                Layout.rightMargin: 32
                spacing: 12

                Text { text: "Trending"; color: "white"; font.pixelSize: 20; font.bold: true }

                Grid {
                    columns: 5
                    spacing: 10

                    Repeater {
                        model: trendingModel
                        delegate: Rectangle {
                            width: (homeScreen.width - 64 - 4 * 10) / 5
                            height: width
                            radius: 8
                            color: "#1a1a2e"
                            clip: true

                            Image {
                                anchors.fill: parent
                                source: model.posterUrl || ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                visible: model.posterUrl !== ""
                            }
                            Text {
                                anchors.centerIn: parent
                                text: "🎬"
                                font.pixelSize: 28
                                visible: !model.posterUrl || model.posterUrl === ""
                            }
                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 36
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#00000000" }
                                    GradientStop { position: 1.0; color: "#cc000000" }
                                }
                            }
                            Text {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 6
                                text: model.title || ""
                                color: "white"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: console.log("Trending:", model.title)
                            }
                        }
                    }
                }
            }

            // ── Upcoming Row ──
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 32
                Layout.rightMargin: 32
                spacing: 12

                Text { text: "Upcoming"; color: "white"; font.pixelSize: 20; font.bold: true }

                Row {
                    spacing: 10
                    Repeater {
                        model: upcomingModel
                        delegate: Rectangle {
                            width: (homeScreen.width - 64 - 4 * 10) / 5
                            height: width
                            radius: 8
                            color: "#1a1a2e"
                            clip: true

                            Image {
                                anchors.fill: parent
                                source: model.posterUrl || ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                visible: model.posterUrl !== ""
                            }
                            Text {
                                anchors.centerIn: parent
                                text: "🎬"
                                font.pixelSize: 28
                                visible: !model.posterUrl || model.posterUrl === ""
                            }
                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 36
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#00000000" }
                                    GradientStop { position: 1.0; color: "#cc000000" }
                                }
                            }
                            Text {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 6
                                text: model.title || ""
                                color: "white"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: console.log("Upcoming:", model.title)
                            }
                        }
                    }
                }
            }

            // ── Source Homepage Grid ──
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 32
                Layout.rightMargin: 32
                spacing: 12

                Text {
                    text: currentSourceName !== "" ? currentSourceName : "From Source"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                }

                Text {
                    text: fallbackMessage
                    color: "#fbbf24"
                    font.pixelSize: 14
                    visible: fallbackMessage !== ""
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Grid {
                    columns: 5
                    spacing: 10

                    Repeater {
                        model: homepageModel
                        delegate: Rectangle {
                            id: card
                            width: (homeScreen.width - 64 - 4 * 10) / 5
                            height: width
                            radius: 8
                            color: "#13131f"
                            border.color: hoverArea.containsMouse ? "#7c3aed" : "#2a2a3e"
                            border.width: hoverArea.containsMouse ? 2 : 1
                            clip: true

                            Image {
                                anchors.fill: parent
                                source: model.posterUrl || ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                visible: model.posterUrl !== ""
                            }
                            Text {
                                anchors.centerIn: parent
                                text: "🎬"
                                font.pixelSize: 28
                                visible: !model.posterUrl || model.posterUrl === ""
                            }

                            // dark overlay on hover
                            Rectangle {
                                anchors.fill: parent
                                color: "#99000000"
                                visible: hoverArea.containsMouse
                            }

                            // "View Info" button — only if source hasInfo
                            Rectangle {
                                anchors.centerIn: parent
                                width: parent.width * 0.7
                                height: 32
                                radius: 6
                                color: "#7c3aed"
                                visible: hoverArea.containsMouse && (model.hasInfo === true)
                                z:10

                                Text {
                                    anchors.centerIn: parent
                                    text: "View Info"
                                    color: "white"
                                    font.pixelSize: 13
                                    font.bold: true
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    z:10
                                    onClicked: {
                                        homepageBridge.loadInfo(model.id, model.sourceName)
                                    }
                                }
                            }

                            // title + rating overlay at bottom
                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 50
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#00000000" }
                                    GradientStop { position: 1.0; color: "#ee000000" }
                                }
                            }
                            ColumnLayout {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 6
                                spacing: 2
                                Text {
                                    text: model.title || ""
                                    color: "white"
                                    font.pixelSize: 11
                                    font.bold: true
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                RowLayout {
                                    spacing: 3
                                    visible: model.rating > 0
                                    Text { text: "★"; color: "#f59e0b"; font.pixelSize: 10 }
                                    Text { text: model.rating ? model.rating.toFixed(1) : ""; color: "#d1d5db"; font.pixelSize: 10 }
                                }
                            }

                            MouseArea {
                                id: hoverArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: console.log("Homepage item:", model.title)
                            }
                        }
                    }
                }

                Item { Layout.preferredHeight: 24 }
            }
        }
    }
}