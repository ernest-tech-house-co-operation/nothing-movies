import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#0d0d0d"

    property var result: ({})
    property bool resolving: false
    property string pendingAction: ""

    signal backRequested()

    property bool isSeries: result.type === "series"
    property int  selectedSeason: 1

    Connections {
        target: searchBridge
        function onStreamUrlReady(title, url) {
            if (!resolving) return
            if (pendingAction === "download") {
                const ok = queueBridge.enqueue(title, url)
                statusLabel.color = ok ? "#4ade80" : "#ff8080"
                statusLabel.text = ok ? "Added to Downloads." : "Couldn't start download."
                statusLabel.visible = true
                resolving = false
                pendingAction = ""
            } else {
                // stream — route by streamType
                const sType = result.streamType || ""
                if (sType === "torrent") {
                    statusLabel.color = "#a78bfa"
                    statusLabel.text = "Buffering torrent... will play when ready."
                    statusLabel.visible = true
                    queueBridge.streamTorrent(title, url)
                    // resolving stays true until torrentReadyToPlay fires
                } else {
                    // http or unset — play immediately
                    resolving = false
                    pendingAction = ""
                    appController.startStream(title, url)
                }
            }
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

    Connections {
        target: queueBridge
        function onTorrentReadyToPlay(title, filePath) {
            if (pendingAction !== "stream") return
            resolving = false
            pendingAction = ""
            statusLabel.visible = false

            // load subtitles if source supports them
            if (result.hasSubtitles === true) {
                var subs = homepageBridge.getSubtitleUrls(result.id, result.sourceName)
                for (var i = 0; i < subs.length; i++) {
                    playerModule.loadSubtitle(subs[i])
                }
            }

            appController.startStream(title, filePath)
        }
        function onStreamError(message) {
            resolving = false
            pendingAction = ""
            statusLabel.color = "#ff8080"
            statusLabel.text = "Stream error: " + message
            statusLabel.visible = true
        }
    }

    // ── backdrop ──
    Image {
        anchors.fill: parent
        source: result.backdropUrl || ""
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        opacity: 0.12
        visible: result.backdropUrl && result.backdropUrl.length > 0
    }

    // dark gradient over backdrop
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#cc0d0d0d" }
            GradientStop { position: 0.3; color: "#880d0d0d" }
            GradientStop { position: 1.0; color: "#ff0d0d0d" }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── top bar ──
        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: "#00000000"

            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#cc1a1030" }
                    GradientStop { position: 1.0; color: "#880f1230" }
                }
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
                anchors.leftMargin: 20
                anchors.rightMargin: 24
                spacing: 16

                // back button
                Rectangle {
                    width: 100
                    height: 36
                    radius: 8
                    color: backHover.containsMouse ? "#2d2d4e" : "#1a1a2e"
                    border.color: backHover.containsMouse ? "#7c3aed" : "#2a2a3e"
                    border.width: 1

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        Text { text: "←"; color: "#a78bfa"; font.pixelSize: 16 }
                        Text { text: "Back"; color: "white"; font.pixelSize: 14 }
                    }

                    MouseArea {
                        id: backHover
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.backRequested()
                    }
                }

                // title in top bar
                Text {
                    Layout.fillWidth: true
                    text: result.title || ""
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    elide: Text.ElideRight
                }

                // type badge in top bar
                Rectangle {
                    width: badgeLabel.implicitWidth + 16
                    height: 26
                    radius: 5
                    color: isSeries ? "#0e7490" : "#6d28d9"
                    Text {
                        id: badgeLabel
                        anchors.centerIn: parent
                        text: isSeries ? "SERIES" : "MOVIE"
                        color: "white"
                        font.pixelSize: 11
                        font.bold: true
                        font.letterSpacing: 1
                    }
                }
            }
        }

        // ── scrollable content ──
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: contentCol.implicitHeight
            clip: true

            ColumnLayout {
                id: contentCol
                width: parent.width
                spacing: 0

                // ── poster + core info ──
                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 32
                    spacing: 32
                    Layout.alignment: Qt.AlignTop

                    // poster
                    Rectangle {
                        width: 220
                        height: 330
                        radius: 10
                        color: "#222233"
                        clip: true

                        Image {
                            anchors.fill: parent
                            source: result.posterUrl || ""
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            visible: result.posterUrl && result.posterUrl.length > 0
                        }
                        Text {
                            anchors.centerIn: parent
                            text: "🎬"
                            font.pixelSize: 48
                            visible: !result.posterUrl || result.posterUrl.length === 0
                        }
                    }

                    // info column
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 12

                        // meta row
                        RowLayout {
                            spacing: 12

                            Text {
                                text: result.year > 0 ? result.year.toString() : ""
                                color: "#9ca3af"
                                font.pixelSize: 14
                                visible: result.hasYear && result.year > 0
                            }
                            Text {
                                text: "·"
                                color: "#4b5563"
                                font.pixelSize: 14
                                visible: result.hasLength && result.lengthMins > 0
                            }
                            Text {
                                text: result.lengthMins > 0
                                    ? Math.floor(result.lengthMins / 60) + "h " + (result.lengthMins % 60) + "m"
                                    : ""
                                color: "#9ca3af"
                                font.pixelSize: 14
                                visible: result.hasLength && result.lengthMins > 0
                            }
                            Text {
                                text: "·"
                                color: "#4b5563"
                                font.pixelSize: 14
                                visible: result.hasQuality && result.quality && result.quality.length > 0
                            }
                            Rectangle {
                                width: qualityLabel.implicitWidth + 12
                                height: 22
                                radius: 4
                                color: "#1f2937"
                                border.color: "#4b5563"
                                border.width: 1
                                visible: result.hasQuality && result.quality && result.quality.length > 0
                                Text {
                                    id: qualityLabel
                                    anchors.centerIn: parent
                                    text: result.quality || ""
                                    color: "#e5e7eb"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                            }
                            RowLayout {
                                spacing: 4
                                visible: result.hasRating && result.rating > 0
                                Text { text: "★"; color: "#f59e0b"; font.pixelSize: 16 }
                                Text {
                                    text: result.rating ? result.rating.toFixed(1) : ""
                                    color: "#f59e0b"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                            }
                        }

                        // source name
                        Text {
                            text: result.sourceName || ""
                            color: "#6b7280"
                            font.pixelSize: 13
                        }

                        // synopsis inline under title (short version)
                        Text {
                            text: result.synopsis || ""
                            color: "#d1d5db"
                            font.pixelSize: 14
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            lineHeight: 1.6
                            visible: result.hasSynopsis && result.synopsis && result.synopsis.length > 0
                        }

                        // ── action buttons ──
                        RowLayout {
                            spacing: 12
                            Layout.topMargin: 8

                            // Stream button
                            Rectangle {
                                width: 150
                                height: 44
                                radius: 10
                                visible: result.hasStream === true
                                opacity: resolving ? 0.6 : 1.0
                                gradient: Gradient {
                                    orientation: Gradient.Horizontal
                                    GradientStop { position: 0.0; color: "#7c3aed" }
                                    GradientStop { position: 1.0; color: "#6d28d9" }
                                }

                                Text {
                                    anchors.centerIn: parent
                                    text: resolving && pendingAction === "stream" ? "Buffering..." : "▶  Stream"
                                    color: "white"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    enabled: !resolving
                                    onClicked: {
                                        pendingAction = "stream"
                                        resolving = true
                                        statusLabel.visible = false
                                        // get the url first, then route by streamType
                                        searchBridge.getStreamUrl(result.id, result.sourceName, result.title)
                                    }
                                }
                            }

                            // Download button
                            Rectangle {
                                width: 150
                                height: 44
                                radius: 10
                                visible: result.hasDownload === true
                                opacity: resolving ? 0.6 : 1.0
                                gradient: Gradient {
                                    orientation: Gradient.Horizontal
                                    GradientStop { position: 0.0; color: "#059669" }
                                    GradientStop { position: 1.0; color: "#047857" }
                                }

                                Text {
                                    anchors.centerIn: parent
                                    text: resolving && pendingAction === "download" ? "Queuing..." : "⬇  Download"
                                    color: "white"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    enabled: !resolving
                                    onClicked: {
                                        pendingAction = "download"
                                        resolving = true
                                        statusLabel.visible = false
                                        searchBridge.getStreamUrl(result.id, result.sourceName, result.title)
                                    }
                                }
                            }

                            // Trailer button
                            Rectangle {
                                width: 150
                                height: 44
                                radius: 10
                                color: "#1f2937"
                                border.color: "#374151"
                                border.width: 1
                                visible: result.hasTrailer && result.trailerUrl && result.trailerUrl.length > 0

                                Text {
                                    anchors.centerIn: parent
                                    text: "🎬  Trailer"
                                    color: "white"
                                    font.pixelSize: 14
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: Qt.openUrlExternally(result.trailerUrl)
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
                    }
                }

                // ── divider ──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 32
                    Layout.rightMargin: 32
                    height: 1
                    color: "#1f2937"
                    visible: (result.hasCast && result.cast && result.cast.length > 0) || isSeries
                }

                // ── cast ──
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 32
                    Layout.rightMargin: 32
                    Layout.topMargin: 24
                    Layout.bottomMargin: 24
                    spacing: 12
                    visible: result.hasCast && result.cast && result.cast.length > 0

                    Text {
                        text: "Cast"
                        color: "#a78bfa"
                        font.pixelSize: 16
                        font.bold: true
                        font.letterSpacing: 1
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: result.cast || []
                            delegate: Rectangle {
                                width: actorName.implicitWidth + 20
                                height: 32
                                radius: 16
                                color: "#1f2937"
                                border.color: "#374151"
                                border.width: 1
                                Text {
                                    id: actorName
                                    anchors.centerIn: parent
                                    text: modelData
                                    color: "#e5e7eb"
                                    font.pixelSize: 13
                                }
                            }
                        }
                    }
                }

                // ── series episode section ──
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 32
                    Layout.rightMargin: 32
                    Layout.bottomMargin: 40
                    spacing: 16
                    visible: isSeries && result.episodes && result.episodes.length > 0

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#1f2937"
                    }

                    Text {
                        text: "Episodes"
                        color: "#a78bfa"
                        font.pixelSize: 16
                        font.bold: true
                        font.letterSpacing: 1
                    }

                    // season tabs
                    Flow {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: {
                                if (!result.episodes) return []
                                var seasons = []
                                for (var i = 0; i < result.episodes.length; i++) {
                                    var s = result.episodes[i].season
                                    if (seasons.indexOf(s) === -1) seasons.push(s)
                                }
                                seasons.sort(function(a, b) { return a - b })
                                return seasons
                            }
                            delegate: Rectangle {
                                width: sLabel.implicitWidth + 24
                                height: 36
                                radius: 8
                                color: root.selectedSeason === modelData ? "#7c3aed" : "#1a1a2e"
                                border.color: root.selectedSeason === modelData ? "#7c3aed" : "#2a2a3e"
                                border.width: 1
                                Text {
                                    id: sLabel
                                    anchors.centerIn: parent
                                    text: "Season " + modelData
                                    color: "white"
                                    font.pixelSize: 13
                                    font.bold: root.selectedSeason === modelData
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.selectedSeason = modelData
                                }
                            }
                        }
                    }

                    // episode rows
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: {
                                if (!result.episodes) return []
                                var eps = []
                                for (var i = 0; i < result.episodes.length; i++) {
                                    if (result.episodes[i].season === root.selectedSeason)
                                        eps.push(result.episodes[i])
                                }
                                return eps
                            }
                            delegate: Rectangle {
                                Layout.fillWidth: true
                                width: contentCol.width - 64
                                height: 68
                                radius: 10
                                color: "#13131f"
                                border.color: "#2a2a3e"
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 14
                                    spacing: 16

                                    Rectangle {
                                        width: 44
                                        height: 44
                                        radius: 8
                                        color: "#1f2937"
                                        Text {
                                            anchors.centerIn: parent
                                            text: "E" + modelData.episode
                                            color: "#a78bfa"
                                            font.pixelSize: 13
                                            font.bold: true
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 3
                                        Text {
                                            text: modelData.title || ("Episode " + modelData.episode)
                                            color: "white"
                                            font.pixelSize: 14
                                            font.bold: true
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                        Text {
                                            text: modelData.synopsis || ""
                                            color: "#6b7280"
                                            font.pixelSize: 12
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                            visible: modelData.synopsis && modelData.synopsis.length > 0
                                        }
                                    }

                                    Rectangle {
                                        width: 90
                                        height: 36
                                        radius: 8
                                        visible: result.hasStream === true
                                        gradient: Gradient {
                                            orientation: Gradient.Horizontal
                                            GradientStop { position: 0.0; color: "#7c3aed" }
                                            GradientStop { position: 1.0; color: "#6d28d9" }
                                        }
                                        Text {
                                            anchors.centerIn: parent
                                            text: "▶ Play"
                                            color: "white"
                                            font.pixelSize: 13
                                            font.bold: true
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: {
                                                if (modelData.streamUrl && modelData.streamUrl.length > 0)
                                                    appController.startStream(modelData.title, modelData.streamUrl)
                                                else
                                                    searchBridge.getStreamUrl(result.id, result.sourceName, modelData.title)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Item { Layout.preferredHeight: 40 }
            }
        }
    }
}