import QtQuick 2.15
import QtQuick.Layouts 1.15

Item {
    id: homeScreen

    ListModel { id: trendingModel }
    ListModel { id: upcomingModel }

    property string heroBackdrop: ""
    property string heroTitle: ""
    property string heroYear: ""

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

    Component.onCompleted: {
        tmdbBridge.loadTrending()
        tmdbBridge.loadUpcoming()
    }

    // base color behind everything
    Rectangle {
        anchors.fill: parent
        color: "#0b0b12"
    }

    Flickable {
        anchors.fill: parent
        contentHeight: mainColumn.height
        clip: true

        ColumnLayout {
            id: mainColumn
            width: parent.width
            spacing: 28

            // --- Hero banner using a TMDB backdrop image ---
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

                // dark-to-color gradient so text stays readable and the
                // page doesn't feel like flat black
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

                    Text {
                        text: "Nothing Movies"
                        color: "#c4b5fd"
                        font.pixelSize: 14
                        font.bold: true
                        font.letterSpacing: 2
                    }

                    Text {
                        text: homeScreen.heroTitle
                        color: "white"
                        font.pixelSize: 34
                        font.bold: true
                        visible: homeScreen.heroTitle !== ""
                    }

                    Text {
                        text: homeScreen.heroYear
                        color: "#d8d3ff"
                        font.pixelSize: 15
                        visible: homeScreen.heroYear !== ""
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 24
                Layout.leftMargin: 32
                Layout.rightMargin: 32
                spacing: 32

                // --- Trending section: vertical list, scrolls downward ---
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Trending"
                            color: "white"
                            font.pixelSize: 20
                            font.bold: true
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 2
                            Layout.alignment: Qt.AlignVCenter
                            Layout.leftMargin: 12
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0.0; color: "#7c3aed" }
                                GradientStop { position: 1.0; color: "#00000000" }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Repeater {
                            model: trendingModel

                            delegate: MovieListItem {
                                Layout.fillWidth: true
                                movieTitle: model.title
                                movieYear: model.year.toString()
                                posterUrl: model.posterUrl
                                onClicked: console.log("Selected:", model.title)
                            }
                        }
                    }
                }

                // --- Upcoming section: also a downward vertical list ---
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Upcoming"
                            color: "white"
                            font.pixelSize: 20
                            font.bold: true
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 2
                            Layout.alignment: Qt.AlignVCenter
                            Layout.leftMargin: 12
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0.0; color: "#06b6d4" }
                                GradientStop { position: 1.0; color: "#00000000" }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Repeater {
                            model: upcomingModel

                            delegate: MovieListItem {
                                Layout.fillWidth: true
                                movieTitle: model.title
                                movieYear: model.year.toString()
                                posterUrl: model.posterUrl
                                onClicked: console.log("Selected:", model.title)
                            }
                        }
                    }
                }

                Item { Layout.preferredHeight: 24 }
            }
        }
    }
}
