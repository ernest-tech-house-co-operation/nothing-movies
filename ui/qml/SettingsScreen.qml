import QtQuick 2.15
import QtQuick.Layouts 1.15

Item {
    id: settingsScreen

    signal openThanksRequested()

    // ── Local-only state for placeholder toggles (not persisted, not wired
    // to any backend yet — purely so the UI has something to show/animate) ──
    property bool autoplayNext: true
    property bool resumePlayback: true
    property bool useExternalPlayer: false
    property bool autoDeleteWatched: false
    property bool vpnPassthrough: false

    property string appVersion: "0.1.0"

    ListModel { id: sourcesModel }

    Component.onCompleted: {
        if (typeof homepageBridge !== "undefined" && homepageBridge) {
            var srcs = homepageBridge.getSources()
            for (var i = 0; i < srcs.length; i++) sourcesModel.append(srcs[i])
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
            spacing: 28

            Text {
                text: "Settings"
                color: "white"
                font.pixelSize: 28
                font.bold: true
            }

            // ══════════════════════ PLAYBACK ══════════════════════
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10

                Text { text: "Playback"; color: "#a78bfa"; font.pixelSize: 13; font.bold: true; font.letterSpacing: 1 }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: settingsRows1.implicitHeight + 16
                    radius: 10
                    color: "#171225"

                    ColumnLayout {
                        id: settingsRows1
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Text { text: "Autoplay next episode"; color: "white"; font.pixelSize: 13; Layout.fillWidth: true }
                            Rectangle {
                                width: 44; height: 24; radius: 12
                                color: settingsScreen.autoplayNext ? "#7c3aed" : "#2a2a3a"
                                Behavior on color { ColorAnimation { duration: 120 } }
                                Rectangle {
                                    width: 18; height: 18; radius: 9; color: "white"
                                    anchors.verticalCenter: parent.verticalCenter
                                    x: settingsScreen.autoplayNext ? parent.width - width - 3 : 3
                                    Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                                }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: settingsScreen.autoplayNext = !settingsScreen.autoplayNext }
                            }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#221c33" }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Text { text: "Resume from last position"; color: "white"; font.pixelSize: 13; Layout.fillWidth: true }
                            Rectangle {
                                width: 44; height: 24; radius: 12
                                color: settingsScreen.resumePlayback ? "#7c3aed" : "#2a2a3a"
                                Behavior on color { ColorAnimation { duration: 120 } }
                                Rectangle {
                                    width: 18; height: 18; radius: 9; color: "white"
                                    anchors.verticalCenter: parent.verticalCenter
                                    x: settingsScreen.resumePlayback ? parent.width - width - 3 : 3
                                    Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                                }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: settingsScreen.resumePlayback = !settingsScreen.resumePlayback }
                            }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#221c33" }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Text { text: "Subtitle language"; color: "white"; font.pixelSize: 13; Layout.fillWidth: true }
                            Text { text: "English ›"; color: "#8b86a8"; font.pixelSize: 13 }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#221c33" }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Text { text: "Use external player"; color: "white"; font.pixelSize: 13; Layout.fillWidth: true }
                            Rectangle {
                                width: 44; height: 24; radius: 12
                                color: settingsScreen.useExternalPlayer ? "#7c3aed" : "#2a2a3a"
                                Behavior on color { ColorAnimation { duration: 120 } }
                                Rectangle {
                                    width: 18; height: 18; radius: 9; color: "white"
                                    anchors.verticalCenter: parent.verticalCenter
                                    x: settingsScreen.useExternalPlayer ? parent.width - width - 3 : 3
                                    Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                                }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: settingsScreen.useExternalPlayer = !settingsScreen.useExternalPlayer }
                            }
                        }
                    }
                }
            }

            // ══════════════════════ DOWNLOADS & STORAGE ══════════════════════
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10

                Text { text: "Downloads & Storage"; color: "#a78bfa"; font.pixelSize: 13; font.bold: true; font.letterSpacing: 1 }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: settingsRows2.implicitHeight + 16
                    radius: 10
                    color: "#171225"

                    ColumnLayout {
                        id: settingsRows2
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 56
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: "Download folder"; color: "white"; font.pixelSize: 13 }
                                Text {
                                    text: (typeof queueBridge !== "undefined" && queueBridge) ? queueBridge.downloadFolder() : "./downloads"
                                    color: "#8b86a8"
                                    font.pixelSize: 11
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                            }
                            Text { text: "Change ›"; color: "#a78bfa"; font.pixelSize: 12 }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#221c33" }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Text { text: "Max concurrent downloads"; color: "white"; font.pixelSize: 13; Layout.fillWidth: true }
                            Text { text: "3"; color: "#8b86a8"; font.pixelSize: 13 }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#221c33" }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Text { text: "Auto-delete watched downloads"; color: "white"; font.pixelSize: 13; Layout.fillWidth: true }
                            Rectangle {
                                width: 44; height: 24; radius: 12
                                color: settingsScreen.autoDeleteWatched ? "#7c3aed" : "#2a2a3a"
                                Behavior on color { ColorAnimation { duration: 120 } }
                                Rectangle {
                                    width: 18; height: 18; radius: 9; color: "white"
                                    anchors.verticalCenter: parent.verticalCenter
                                    x: settingsScreen.autoDeleteWatched ? parent.width - width - 3 : 3
                                    Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                                }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: settingsScreen.autoDeleteWatched = !settingsScreen.autoDeleteWatched }
                            }
                        }
                    }
                }
            }

            // ══════════════════════ SOURCES ══════════════════════
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10
                visible: sourcesModel.count > 0

                Text { text: "Sources"; color: "#a78bfa"; font.pixelSize: 13; font.bold: true; font.letterSpacing: 1 }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: sourcesColumn.implicitHeight + 16
                    radius: 10
                    color: "#171225"

                    ColumnLayout {
                        id: sourcesColumn
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 0

                        Repeater {
                            model: sourcesModel
                            delegate: ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 48
                                    Layout.leftMargin: 12
                                    Layout.rightMargin: 12
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 1
                                        Text { text: model.name || "Unknown source"; color: "white"; font.pixelSize: 13 }
                                        Text { text: model.version ? ("v" + model.version) : ""; color: "#6f6a92"; font.pixelSize: 10; visible: !!model.version }
                                    }
                                    // Visual-only for now — no backend hook yet to actually
                                    // disable a source from the aggregator.
                                    Rectangle {
                                        width: 44; height: 24; radius: 12
                                        color: "#7c3aed"
                                        Rectangle {
                                            width: 18; height: 18; radius: 9; color: "white"
                                            anchors.verticalCenter: parent.verticalCenter
                                            x: parent.width - width - 3
                                        }
                                    }
                                }
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 1
                                    color: "#221c33"
                                    visible: index < sourcesModel.count - 1
                                }
                            }
                        }
                    }
                }
            }

            // ══════════════════════ NETWORK ══════════════════════
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10

                Text { text: "Network"; color: "#a78bfa"; font.pixelSize: 13; font.bold: true; font.letterSpacing: 1 }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: settingsRows3.implicitHeight + 16
                    radius: 10
                    color: "#171225"

                    ColumnLayout {
                        id: settingsRows3
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Text { text: "Route traffic through VPN/proxy"; color: "white"; font.pixelSize: 13; Layout.fillWidth: true }
                            Rectangle {
                                width: 44; height: 24; radius: 12
                                color: settingsScreen.vpnPassthrough ? "#7c3aed" : "#2a2a3a"
                                Behavior on color { ColorAnimation { duration: 120 } }
                                Rectangle {
                                    width: 18; height: 18; radius: 9; color: "white"
                                    anchors.verticalCenter: parent.verticalCenter
                                    x: settingsScreen.vpnPassthrough ? parent.width - width - 3 : 3
                                    Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                                }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: settingsScreen.vpnPassthrough = !settingsScreen.vpnPassthrough }
                            }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#221c33" }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Text { text: "Bandwidth limit"; color: "white"; font.pixelSize: 13; Layout.fillWidth: true }
                            Text { text: "Unlimited"; color: "#8b86a8"; font.pixelSize: 13 }
                        }
                    }
                }
            }

            // ══════════════════════ APPEARANCE ══════════════════════
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10

                Text { text: "Appearance"; color: "#a78bfa"; font.pixelSize: 13; font.bold: true; font.letterSpacing: 1 }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: settingsRows4.implicitHeight + 16
                    radius: 10
                    color: "#171225"

                    ColumnLayout {
                        id: settingsRows4
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Text { text: "Theme"; color: "white"; font.pixelSize: 13; Layout.fillWidth: true }
                            Text { text: "Dark"; color: "#8b86a8"; font.pixelSize: 13 }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#221c33" }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Text { text: "Home grid density"; color: "white"; font.pixelSize: 13; Layout.fillWidth: true }
                            Text { text: "Comfortable ›"; color: "#8b86a8"; font.pixelSize: 13 }
                        }
                    }
                }
            }

            // ══════════════════════ ABOUT & LEGAL ══════════════════════
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10

                Text { text: "About & Legal"; color: "#a78bfa"; font.pixelSize: 13; font.bold: true; font.letterSpacing: 1 }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: settingsRows5.implicitHeight + 16
                    radius: 10
                    color: "#171225"

                    ColumnLayout {
                        id: settingsRows5
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Text { text: "Version"; color: "white"; font.pixelSize: 13; Layout.fillWidth: true }
                            Text { text: settingsScreen.appVersion; color: "#8b86a8"; font.pixelSize: 13 }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#221c33" }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Text { text: "Privacy Policy"; color: "white"; font.pixelSize: 13; Layout.fillWidth: true }
                            Text { text: "›"; color: "#8b86a8"; font.pixelSize: 13 }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: legalOverlay.showPolicy("privacy") }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#221c33" }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Text { text: "Terms of Use"; color: "white"; font.pixelSize: 13; Layout.fillWidth: true }
                            Text { text: "›"; color: "#8b86a8"; font.pixelSize: 13 }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: legalOverlay.showPolicy("terms") }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#221c33" }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Text { text: "Open-source licenses"; color: "white"; font.pixelSize: 13; Layout.fillWidth: true }
                            Text { text: "›"; color: "#8b86a8"; font.pixelSize: 13 }
                        }
                    }
                }

                Rectangle {
                    id: thanksLink
                    Layout.preferredWidth: 220
                    Layout.preferredHeight: 48
                    Layout.topMargin: 4
                    radius: 10
                    color: thanksMouseArea.containsMouse ? "#2a2140" : "#171225"
                    border.width: 1
                    border.color: "#7c3aed"
                    Behavior on color { ColorAnimation { duration: 150 } }
                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 8
                        Text { text: "💜"; font.pixelSize: 16 }
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
            }

            // ══════════════════════ DEVELOPER ══════════════════════
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10

                Text { text: "Developer"; color: "#a78bfa"; font.pixelSize: 13; font.bold: true; font.letterSpacing: 1 }

                Rectangle {
                    Layout.preferredWidth: 220
                    Layout.preferredHeight: 48
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
            }

            Item { Layout.preferredHeight: 24 }
        }
    }

    // ── Privacy Policy / Terms of Use overlay ──
    // NOTE: duplicated here from Splash.qml for now since the two screens
    // are separate component instances. Worth moving both copies of this
    // text into one shared singleton (e.g. a Legal.qml with pragma Singleton)
    // later so they can't drift out of sync.
    Rectangle {
        id: legalOverlay
        anchors.fill: parent
        color: "#000000cc"
        visible: false
        z: 10

        property string currentType: "privacy"
        property string privacyPolicyText:
            "Privacy Policy\n\nNothing Movies (\"we\", \"us\") respects your privacy. This policy explains what information we collect and how we use it.\n\n1. Information We Collect\nWe may collect basic device information, app usage data, and any information you voluntarily provide.\n\n2. How We Use It\nWe use collected information to operate and improve the app and communicate important updates.\n\n3. Sharing\nWe do not sell your personal data.\n\n4. Data Security\nWe take reasonable measures to protect your data, but no method of storage is 100% secure.\n\n5. Your Choices\nYou may request access to, correction of, or deletion of your personal data by contacting us.\n\n6. Changes\nWe may update this policy from time to time."
        property string termsOfUseText:
            "Terms of Use\n\nWelcome to Nothing Movies. By using this app, you agree to the following terms.\n\n1. Use of the App\nYou agree to use the app only for lawful purposes.\n\n2. Content\nAll content provided through the app is for personal, non-commercial use unless otherwise stated.\n\n3. Accounts\nYou are responsible for maintaining the confidentiality of your credentials.\n\n4. Intellectual Property\nAll trademarks, logos, and content remain the property of Nothing Movies or its licensors.\n\n5. Limitation of Liability\nThe app is provided \"as is\" without warranties of any kind.\n\n6. Termination\nWe reserve the right to suspend or terminate access for violation of these terms.\n\n7. Changes to Terms\nWe may revise these terms at any time."

        function showPolicy(type) {
            legalOverlay.currentType = type
            legalOverlay.visible = true
        }

        MouseArea {
            anchors.fill: parent
            onClicked: legalOverlay.visible = false
        }

        Rectangle {
            width: parent.width * 0.7
            height: parent.height * 0.75
            anchors.centerIn: parent
            color: "#1a1a1a"
            radius: 12
            border.color: "#333333"
            border.width: 1

            MouseArea { anchors.fill: parent; onClicked: {} }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 14

                Text {
                    text: legalOverlay.currentType === "privacy" ? "Privacy Policy" : "Terms of Use"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 20
                }

                Flickable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentHeight: policyText.paintedHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    Text {
                        id: policyText
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: "#cccccc"
                        font.pixelSize: 13
                        lineHeight: 1.3
                        text: legalOverlay.currentType === "privacy" ? legalOverlay.privacyPolicyText : legalOverlay.termsOfUseText
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 90
                    Layout.preferredHeight: 34
                    Layout.alignment: Qt.AlignRight
                    radius: 8
                    color: closeMouseArea.containsMouse ? "#2a2140" : "#171225"
                    border.width: 1
                    border.color: "#7c3aed"
                    Text { anchors.centerIn: parent; text: "Close"; color: "white"; font.pixelSize: 12 }
                    MouseArea {
                        id: closeMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: legalOverlay.visible = false
                    }
                }
            }
        }
    }
}