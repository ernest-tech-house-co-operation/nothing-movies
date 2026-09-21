import QtQuick 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: sourcesScreen
    color: "#0d0d0d"

    property var sources: []
    property int expandedIndex: -1
    property int totalSlots: 8

    Component.onCompleted: {
        sources = homepageBridge.getSources()
    }

    Flickable {
        anchors.fill: parent
        contentHeight: mainCol.implicitHeight
        clip: true

        ColumnLayout {
            id: mainCol
            width: parent.width
            spacing: 0

            // ── header ──
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
                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 32
                    text: "Sources"
                    color: "white"
                    font.pixelSize: 22
                    font.bold: true
                }
                Text {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 32
                    text: sourcesScreen.sources.length + " / " + sourcesScreen.totalSlots + " slots used"
                    color: "#6b7280"
                    font.pixelSize: 13
                }
            }

            // ── 4x2 grid ──
            Grid {
                Layout.fillWidth: true
                Layout.margins: 32
                columns: 4
                spacing: 16

                Repeater {
                    model: sourcesScreen.totalSlots
                    delegate: Item {
                        property bool hasSource: index < sourcesScreen.sources.length
                        property var src: hasSource ? sourcesScreen.sources[index] : null
                        property bool expanded: sourcesScreen.expandedIndex === index

                        width: (sourcesScreen.width - 64 - 3 * 16) / 4
                        height: expanded ? 280 : 120

                        Behavior on height { SmoothedAnimation { duration: 200 } }

                        Rectangle {
                            anchors.fill: parent
                            radius: 12
                            color: hasSource ? (expanded ? "#1a1a2e" : "#13131f") : "#0d0d14"
                            border.color: hasSource ? (expanded ? "#7c3aed" : "#2a2a3e") : "#1a1a2e"
                            border.width: hasSource ? (expanded ? 2 : 1) : 1
                            clip: true

                            // empty slot
                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 8
                                visible: !hasSource
                                Text { Layout.alignment: Qt.AlignHCenter; text: "＋"; color: "#2a2a3e"; font.pixelSize: 28 }
                                Text { Layout.alignment: Qt.AlignHCenter; text: "Empty Slot"; color: "#2a2a3e"; font.pixelSize: 12 }
                            }

                            // source card
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 10
                                visible: hasSource

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 12

                                    Rectangle {
                                        width: 48; height: 48; radius: 24
                                        color: "#1f2937"; clip: true
                                        Image {
                                            anchors.fill: parent
                                            source: src ? (src.profilePicture || "") : ""
                                            fillMode: Image.PreserveAspectCrop
                                            asynchronous: true
                                            visible: src && src.profilePicture && src.profilePicture.length > 0
                                        }
                                        Text {
                                            anchors.centerIn: parent
                                            text: src ? src.name.charAt(0).toUpperCase() : ""
                                            color: "#a78bfa"; font.pixelSize: 20; font.bold: true
                                            visible: !src || !src.profilePicture || src.profilePicture.length === 0
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        Text {
                                            text: src ? src.name : ""
                                            color: "white"; font.pixelSize: 15; font.bold: true
                                            elide: Text.ElideRight; Layout.fillWidth: true
                                        }
                                        Text {
                                            text: src ? ("v" + src.version) : ""
                                            color: "#6b7280"; font.pixelSize: 12
                                        }
                                    }

                                    Text { text: expanded ? "▲" : "▼"; color: "#a78bfa"; font.pixelSize: 12 }
                                }

                                Flow {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    Repeater {
                                        model: {
                                            if (!src) return []
                                            var pills = []
                                            if (src.hasHomepage) pills.push({ label: "Homepage",  color: "#1d4ed8" })
                                            if (src.hasInfo)     pills.push({ label: "Info",      color: "#6d28d9" })
                                            if (src.hasStream)   pills.push({ label: src.streamType === "torrent" ? "Torrent" : "Stream", color: "#0e7490" })
                                            if (src.hasDownload) pills.push({ label: src.downloadType === "torrent" ? "DL Torrent" : "Download", color: "#065f46" })
                                            return pills
                                        }
                                        delegate: Rectangle {
                                            width: pillText.implicitWidth + 12; height: 20; radius: 10
                                            color: modelData.color
                                            Text { id: pillText; anchors.centerIn: parent; text: modelData.label; color: "white"; font.pixelSize: 10; font.bold: true }
                                        }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    visible: expanded
                                    opacity: expanded ? 1.0 : 0.0
                                    Behavior on opacity { SmoothedAnimation { duration: 150 } }

                                    Rectangle { Layout.fillWidth: true; height: 1; color: "#2a2a3e" }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text { text: "Stream via";   color: "#6b7280"; font.pixelSize: 12; Layout.fillWidth: true }
                                        Text { text: src ? (src.streamType || "—") : "—";   color: "#e5e7eb"; font.pixelSize: 12; font.bold: true }
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text { text: "Download via"; color: "#6b7280"; font.pixelSize: 12; Layout.fillWidth: true }
                                        Text { text: src ? (src.downloadType || "—") : "—"; color: "#e5e7eb"; font.pixelSize: 12; font.bold: true }
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text { text: "Homepage";  color: "#6b7280"; font.pixelSize: 12; Layout.fillWidth: true }
                                        Text { text: src ? (src.hasHomepage ? "Yes" : "No") : "—"; color: src && src.hasHomepage ? "#4ade80" : "#ef4444"; font.pixelSize: 12; font.bold: true }
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text { text: "Info pages"; color: "#6b7280"; font.pixelSize: 12; Layout.fillWidth: true }
                                        Text { text: src ? (src.hasInfo ? "Yes" : "No") : "—"; color: src && src.hasInfo ? "#4ade80" : "#ef4444"; font.pixelSize: 12; font.bold: true }
                                    }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: hasSource
                                onClicked: sourcesScreen.expandedIndex = (sourcesScreen.expandedIndex === index) ? -1 : index
                            }
                        }
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
            }

            // ── info section ──
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 32
                spacing: 28

                // slot rules
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Text { text: "How slots work"; color: "#a78bfa"; font.pixelSize: 16; font.bold: true }

                    Text {
                        text: "Nothing Movies supports exactly 8 source slots — this is intentional, not a technical limit. Fewer, vetted sources means better reliability and less risk for everyone using the app."
                        color: "#9ca3af"; font.pixelSize: 13; wrapMode: Text.WordWrap; Layout.fillWidth: true; lineHeight: 1.6
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Rectangle {
                            width: 6; height: 6; radius: 3; color: "#7c3aed"
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Text { text: "Slot 1 — Reserved: Nothing Movies core torrent service"; color: "#d1d5db"; font.pixelSize: 13; Layout.fillWidth: true }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Rectangle { width: 6; height: 6; radius: 3; color: "#7c3aed"; Layout.alignment: Qt.AlignVCenter }
                        Text { text: "Slot 2 — Reserved: Ernest Tech House (project sponsor)"; color: "#d1d5db"; font.pixelSize: 13; Layout.fillWidth: true }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Rectangle { width: 6; height: 6; radius: 3; color: "#059669"; Layout.alignment: Qt.AlignVCenter }
                        Text { text: "Slots 3–8 — Open for community-submitted sources"; color: "#d1d5db"; font.pixelSize: 13; Layout.fillWidth: true }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#1f2937" }

                // submission rules
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Text { text: "Want to submit a source?"; color: "#a78bfa"; font.pixelSize: 16; font.bold: true }

                    Text {
                        text: "Building a working source module is not enough for acceptance. Every submission is reviewed against these criteria:"
                        color: "#9ca3af"; font.pixelSize: 13; wrapMode: Text.WordWrap; Layout.fillWidth: true; lineHeight: 1.6
                    }

                    Repeater {
                        model: [
                            "Must be a well-known torrent indexer or genuinely popular streaming site — no personal or indie sites",
                            "Must be written in pure C++ — no Python, JS, or shell scripts",
                            "Must work on both Windows and Linux",
                            "Must live in its own Git repository with real version history",
                            "Any external tool dependency must be wired into the vendor_updater system",
                            "No hardcoded API keys, personal credentials, or machine-specific paths",
                            "No free-text URL inputs — every source is compiled into the binary"
                        ]
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            Text { text: "✓"; color: "#059669"; font.pixelSize: 13; font.bold: true }
                            Text { text: modelData; color: "#d1d5db"; font.pixelSize: 13; wrapMode: Text.WordWrap; Layout.fillWidth: true; lineHeight: 1.5 }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#1f2937" }

                // ownership note
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text { text: "Ownership & licensing"; color: "#a78bfa"; font.pixelSize: 16; font.bold: true }

                    Text {
                        text: "Accepted sources become joint property between the original author and Nothing Movies / Ernest Tech House. Lead programmers may modify your source directly. You keep credit — you give up exclusive control. Read MOVIE_SOURCE_LICENSE.md in full before submitting."
                        color: "#9ca3af"; font.pixelSize: 13; wrapMode: Text.WordWrap; Layout.fillWidth: true; lineHeight: 1.6
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#1f2937" }

                // community links
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Text { text: "Community & source code"; color: "#a78bfa"; font.pixelSize: 16; font.bold: true }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 12

                        Rectangle {
                            width: githubLabel.implicitWidth + 24
                            height: 38; radius: 8
                            color: "#1f2937"
                            border.color: "#374151"; border.width: 1

                            Text {
                                id: githubLabel
                                anchors.centerIn: parent
                                text: "⭐  GitHub"
                                color: "white"; font.pixelSize: 13
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: Qt.openUrlExternally("https://github.com/ernest-tech-house-co-operation/nothing-movies")
                            }
                        }

                        Rectangle {
                            width: waLabel.implicitWidth + 24
                            height: 38; radius: 8
                            color: "#064e3b"
                            border.color: "#065f46"; border.width: 1

                            Text {
                                id: waLabel
                                anchors.centerIn: parent
                                text: "💬  WhatsApp Channel"
                                color: "white"; font.pixelSize: 13
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: Qt.openUrlExternally("https://whatsapp.com/channel/0029VbBzoXuCxoArtvaslR0U")
                            }
                        }
                    }

                    Text {
                        text: "Nothing Movies — We have what? Everything."
                        color: "#374151"; font.pixelSize: 12; font.italic: true
                    }
                }
            }

            Item { Layout.preferredHeight: 40 }
        }
    }
}