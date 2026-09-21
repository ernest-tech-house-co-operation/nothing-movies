import QtQuick 2.15
import QtQuick.Layouts 1.15

Item {
    id: settingsScreen
    signal openThanksRequested()

    // ── Reusable bits (Qt 5.15+ inline components) ──

    component ToggleSwitch: Rectangle {
        id: toggle
        property bool checked: false
        signal toggled(bool value)
        width: 44
        height: 24
        radius: 12
        color: checked ? "#7c3aed" : "#2a2a3a"
        Behavior on color { ColorAnimation { duration: 150 } }
        Rectangle {
            width: 18
            height: 18
            radius: 9
            color: "white"
            anchors.verticalCenter: parent.verticalCenter
            x: toggle.checked ? toggle.width - width - 3 : 3
            Behavior on x { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                toggle.checked = !toggle.checked
                toggle.toggled(toggle.checked)
            }
        }
    }

    component OptionCycler: Rectangle {
        id: cycler
        property var options: []
        property int index: 0
        readonly property string value: options.length > 0 ? options[index] : ""
        signal changed(string value)
        width: label.implicitWidth + 28
        height: 30
        radius: 8
        color: "#211a35"
        border.width: 1
        border.color: "#3a2f5c"
        Text {
            id: label
            anchors.centerIn: parent
            text: cycler.value
            color: "#c4b5fd"
            font.pixelSize: 12
            font.bold: true
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                cycler.index = (cycler.index + 1) % cycler.options.length
                cycler.changed(cycler.value)
            }
        }
    }

    component SectionCard: ColumnLayout {
        id: section
        property string title: ""
        property string icon: ""
        default property alias content: inner.children
        Layout.fillWidth: true
        spacing: 0

        RowLayout {
            spacing: 8
            Layout.bottomMargin: 10
            Text { text: section.icon; font.pixelSize: 15 }
            Text { text: section.title; color: "white"; font.pixelSize: 16; font.bold: true }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: inner.implicitHeight + 24
            radius: 12
            color: "#131022"
            border.width: 1
            border.color: "#221c3a"

            ColumnLayout {
                id: inner
                anchors.fill: parent
                anchors.margins: 16
                spacing: 14
            }
        }
    }

    component SettingRow: RowLayout {
        id: row
        property string label: ""
        property string sublabel: ""
        default property alias control: controlSlot.children
        Layout.fillWidth: true
        spacing: 12

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text { text: row.label; color: "white"; font.pixelSize: 13 }
            Text {
                text: row.sublabel
                color: "#6f6a92"
                font.pixelSize: 11
                visible: text !== ""
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        Item {
            id: controlSlot
            Layout.preferredWidth: children.length > 0 ? children[0].implicitWidth : 0
            Layout.preferredHeight: 30
        }
    }

    // ── State ──
    ListModel { id: sourcesModel }
    property bool legalOverlayVisible: false
    property string legalOverlayType: "privacy"

    property string privacyPolicyText:
        "Privacy Policy\n\nNothing Movies respects your privacy. This policy explains what information we collect and how we use it.\n\n" +
        "We collect basic device and usage information to operate and improve the app. We do not sell your personal data.\n\n" +
        "By using the app, you acknowledge this policy. Contact us to request access, correction, or deletion of your data."

    property string termsOfUseText:
        "Terms of Use\n\nBy using Nothing Movies, you agree to use the app only for lawful purposes.\n\n" +
        "Content is provided for personal, non-commercial use. The app is provided \"as is\" without warranties of any kind.\n\n" +
        "We may revise these terms at any time; continued use after changes means you accept the revised terms."

    property string licensesText:
        "Open Source Licenses\n\n" +
        "This app is built with the help of open-source software, including:\n\n" +
        "• Qt / QML — LGPLv3\n" +
        "• nlohmann/json — MIT License\n" +
        "• mpv — GPLv2/LGPLv2.1\n\n" +
        "Full license texts are available in the project repository."

    function loadSources() {
        sourcesModel.clear()
        if (typeof homepageBridge === "undefined" || !homepageBridge) return
        const sources = homepageBridge.getSources()
        for (var i = 0; i < sources.length; i++) sourcesModel.append(sources[i])
    }

    Component.onCompleted: loadSources()

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
            spacing: 24

            Text {
                text: "Settings"
                color: "white"
                font.pixelSize: 28
                font.bold: true
            }

            // ── Playback ──
            SectionCard {
                title: "Playback"
                icon: "▶"

                SettingRow {
                    label: "Autoplay next episode"
                    ToggleSwitch { checked: true }
                }
                SettingRow {
                    label: "Resume from last position"
                    ToggleSwitch { checked: true }
                }
                SettingRow {
                    label: "Default subtitle language"
                    OptionCycler { options: ["English", "German", "Spanish", "Off"] }
                }
                SettingRow {
                    label: "Use external player (mpv)"
                    sublabel: "Falls back to the built-in player if unavailable"
                    ToggleSwitch { checked: false }
                }
            }

            // ── Downloads & Storage ──
            SectionCard {
                title: "Downloads & Storage"
                icon: "⬇"

                SettingRow {
                    label: "Download folder"
                    sublabel: queueBridge ? queueBridge.downloadFolder() : "Not available"
                    Rectangle {
                        width: 90; height: 30; radius: 8
                        color: "#211a35"
                        border.width: 1
                        border.color: "#3a2f5c"
                        Text { anchors.centerIn: parent; text: "Change"; color: "#8b86a8"; font.pixelSize: 11 }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: console.log("Change download folder — not wired up yet")
                        }
                    }
                }
                SettingRow {
                    label: "Max concurrent downloads"
                    OptionCycler { options: ["1", "2", "3", "5"]; index: 2 }
                }
                SettingRow {
                    label: "Auto-delete watched downloads"
                    sublabel: "After 30 days"
                    ToggleSwitch { checked: false }
                }
                SettingRow {
                    label: "Storage used"
                    sublabel: "Calculation not wired up yet"
                    Text { text: "—"; color: "#6f6a92"; font.pixelSize: 12 }
                }
            }

            // ── Sources ──
            SectionCard {
                title: "Sources"
                icon: "🔌"

                Text {
                    text: sourcesModel.count === 0 ? "No sources found." : ""
                    color: "#6f6a92"
                    font.pixelSize: 12
                    visible: sourcesModel.count === 0
                }

                Repeater {
                    model: sourcesModel
                    delegate: SettingRow {
                        label: model.name || "Unknown source"
                        sublabel: (model.version ? "v" + model.version : "") +
                                  (model.hasStream ? "  ·  Stream" : "") +
                                  (model.hasDownload ? "  ·  Download" : "")
                        ToggleSwitch { checked: true }
                    }
                }
            }

            // ── Network ──
            SectionCard {
                title: "Network"
                icon: "🌐"

                SettingRow {
                    label: "Use proxy"
                    ToggleSwitch { checked: false }
                }
                SettingRow {
                    label: "Bandwidth limit"
                    OptionCycler { options: ["Unlimited", "5 MB/s", "10 MB/s", "20 MB/s"] }
                }
            }

            // ── Appearance ──
            SectionCard {
                title: "Appearance"
                icon: "🎨"

                SettingRow {
                    label: "Theme"
                    sublabel: "Dark is the only option for now"
                    Text { text: "Dark"; color: "#6f6a92"; font.pixelSize: 12 }
                }
            }

            // ── About & Legal ──
            SectionCard {
                title: "About & Legal"
                icon: "ℹ"

                SettingRow {
                    label: "Version"
                    Text { text: "0.1.0"; color: "#6f6a92"; font.pixelSize: 12 }
                }

                Rectangle {
                    id: thanksLink
                    Layout.preferredWidth: 220
                    Layout.preferredHeight: 44
                    radius: 10
                    color: thanksMouseArea.containsMouse ? "#2a2140" : "#1a1530"
                    border.width: 1
                    border.color: "#7c3aed"
                    Behavior on color { ColorAnimation { duration: 150 } }
                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 8
                        Text { text: "💜"; font.pixelSize: 15 }
                        Text { text: "View Thanks / Credits"; color: "white"; font.pixelSize: 13; font.bold: true }
                    }
                    MouseArea {
                        id: thanksMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: settingsScreen.openThanksRequested()
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 220
                    Layout.preferredHeight: 40
                    radius: 10
                    color: privacyMouseArea.containsMouse ? "#211a35" : "#171225"
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Text {
                        anchors.centerIn: parent
                        text: "Privacy Policy"
                        color: "#8ab4f8"
                        font.pixelSize: 12
                        font.underline: true
                    }
                    MouseArea {
                        id: privacyMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            settingsScreen.legalOverlayType = "privacy"
                            settingsScreen.legalOverlayVisible = true
                        }
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 220
                    Layout.preferredHeight: 40
                    radius: 10
                    color: termsMouseArea.containsMouse ? "#211a35" : "#171225"
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Text {
                        anchors.centerIn: parent
                        text: "Terms of Use"
                        color: "#8ab4f8"
                        font.pixelSize: 12
                        font.underline: true
                    }
                    MouseArea {
                        id: termsMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            settingsScreen.legalOverlayType = "terms"
                            settingsScreen.legalOverlayVisible = true
                        }
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 220
                    Layout.preferredHeight: 40
                    radius: 10
                    color: licensesMouseArea.containsMouse ? "#211a35" : "#171225"
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Text {
                        anchors.centerIn: parent
                        text: "Open Source Licenses"
                        color: "#8ab4f8"
                        font.pixelSize: 12
                        font.underline: true
                    }
                    MouseArea {
                        id: licensesMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            settingsScreen.legalOverlayType = "licenses"
                            settingsScreen.legalOverlayVisible = true
                        }
                    }
                }
            }

            // ── Developer ──
            SectionCard {
                title: "Developer"
                icon: "🧪"

                Rectangle {
                    Layout.preferredWidth: 220
                    Layout.preferredHeight: 44
                    radius: 10
                    color: testMouseArea.containsMouse ? "#1c3a2e" : "#171225"
                    border.width: 1
                    border.color: "#22c55e"
                    Behavior on color { ColorAnimation { duration: 150 } }
                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 8
                        Text { text: "🧪"; font.pixelSize: 16 }
                        Text { text: "Test Player (sample stream)"; color: "white"; font.pixelSize: 12; font.bold: true }
                    }
                    MouseArea {
                        id: testMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        // Public HLS test stream (Mux's test-streams service, a
                        // CC-licensed clip published specifically for exercising
                        // players). No torrent/queue involved -- isolates whether
                        // the mpv embed itself works.
                        onClicked: appController.startStream(
                            "Sample Test Stream",
                            "https://test-streams.mux.dev/x36xhzz/x36xhzz.m3u8"
                        )
                    }
                }

                SettingRow {
                    label: "Show debug logs"
                    ToggleSwitch { checked: false }
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }

    // ── Legal text overlay (Privacy / Terms / Licenses) ──
    Rectangle {
        anchors.fill: parent
        color: "#000000cc"
        visible: settingsScreen.legalOverlayVisible
        z: 10

        MouseArea {
            anchors.fill: parent
            onClicked: settingsScreen.legalOverlayVisible = false
        }

        Rectangle {
            width: parent.width * 0.7
            height: parent.height * 0.75
            anchors.centerIn: parent
            color: "#1a1a1a"
            radius: 12
            border.color: "#333333"
            border.width: 1

            MouseArea {
                anchors.fill: parent
                onClicked: {}
            }

            Column {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 14

                Text {
                    text: {
                        if (settingsScreen.legalOverlayType === "privacy") return "Privacy Policy"
                        if (settingsScreen.legalOverlayType === "terms") return "Terms of Use"
                        return "Open Source Licenses"
                    }
                    color: "white"
                    font.bold: true
                    font.pixelSize: 20
                }

                Flickable {
                    width: parent.width
                    height: parent.height - 90
                    contentHeight: legalText.paintedHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    Text {
                        id: legalText
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: "#cccccc"
                        font.pixelSize: 13
                        lineHeight: 1.3
                        text: {
                            if (settingsScreen.legalOverlayType === "privacy") return settingsScreen.privacyPolicyText
                            if (settingsScreen.legalOverlayType === "terms") return settingsScreen.termsOfUseText
                            return settingsScreen.licensesText
                        }
                    }
                }

                Rectangle {
                    width: 80; height: 32; radius: 6
                    color: "#2a2a3a"
                    Text { anchors.centerIn: parent; text: "Close"; color: "white"; font.pixelSize: 12 }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: settingsScreen.legalOverlayVisible = false
                    }
                }
            }
        }
    }
}