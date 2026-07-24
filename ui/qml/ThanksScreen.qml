import QtQuick 2.15
import QtQuick.Layouts 1.15

Item {
    id: thanksScreen

    // Fill this in with real people. Each entry:
    //   name         - display name
    //   role         - "Contributor" / "Idea Gifter" / "Inspiration" / "Git Contributor" / ...
    //   avatarUrl    - link to a profile picture (e.g. their GitHub avatar,
    //                  which is just https://github.com/<username>.png)
    //   gitUrl       - link to their GitHub/GitLab profile
    //   contribution - short phrase for what they did
    //   note         - a small personal note/thank-you
    //   socials      - list of { icon, url } shown as clickable chips
    property var creditsData: [
        {
            name: "SaltyAom",
            role: "Inspiration",
            avatarUrl: "https://github.com/SaltyAom.png",
            gitUrl: "https://github.com/SaltyAom",
            contribution: "Creator of ElysiaJS, the Bun framework much of this stack is built on.",
            note: "The only person in the entier world i would notmind meeting and accomplishing in life",
            socials: [
                { icon: "🐦", url: "https://twitter.com/saltyAom" },
                { icon: "🌐", url: "https://elysiajs.com" }
            ]
        },
        {
            name: "Your Name Here",
            role: "Contributor",
            avatarUrl: "",
            gitUrl: "https://github.com/yourusername",
            contribution: "Built the core app and UI.",
            note: "Couldn't have shipped this without late nights and coffee.",
            socials: [
                { icon: "🐦", url: "https://twitter.com/yourusername" },
                { icon: "🌐", url: "https://yourwebsite.com" }
            ]
        },
        {
            name: "Someone Helpful",
            role: "Idea Gifter",
            avatarUrl: "",
            gitUrl: "https://github.com/someonehelpful",
            contribution: "Suggested the vertical home layout.",
            note: "One offhand comment turned into this whole redesign.",
            socials: []
        },
        {
            name: "Someone Inspiring",
            role: "Inspiration",
            avatarUrl: "",
            gitUrl: "",
            contribution: "Their project sparked this one.",
            note: "Thanks for showing what was possible.",
            socials: [
                { icon: "🐦", url: "https://twitter.com/someoneinspiring" }
            ]
        },
        {
            name: "octocat",
            role: "Git Contributor",
            avatarUrl: "https://github.com/octocat.png",
            gitUrl: "https://github.com/octocat",
            contribution: "Opened a PR that fixed a bug.",
            note: "Fast, clean fix. Appreciated.",
            socials: []
        }
    ]

    property var roleColors: ({
        "Contributor":      "#7c3aed",
        "Idea Gifter":      "#06b6d4",
        "Inspiration":       "#f472b6",
        "Git Contributor":  "#22c55e"
    })

    Rectangle {
        anchors.fill: parent
        color: "#0b0b12"
    }

    Flickable {
        anchors.fill: parent
        contentHeight: column.height
        clip: true

        ColumnLayout {
            id: column
            width: parent.width
            spacing: 24

            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 32
                Layout.bottomMargin: 0
                spacing: 6

                Text {
                    text: "Thanks"
                    color: "white"
                    font.pixelSize: 30
                    font.bold: true
                }
                Text {
                    text: "Everyone who helped make Nothing Movies happen."
                    color: "#a1a1c9"
                    font.pixelSize: 14
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 2
                    Layout.topMargin: 8
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#7c3aed" }
                        GradientStop { position: 1.0; color: "#06b6d4" }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 32
                Layout.rightMargin: 32
                spacing: 14

                Repeater {
                    model: thanksScreen.creditsData

                    delegate: Rectangle {
                        id: card
                        Layout.fillWidth: true
                        implicitHeight: cardContent.implicitHeight + 32
                        radius: 12
                        color: card.hovered ? "#211a3d" : "#171225"
                        border.width: card.hovered ? 1 : 0
                        border.color: thanksScreen.roleColors[modelData.role] || "#7c3aed"

                        property bool hovered: false
                        Behavior on color { ColorAnimation { duration: 150 } }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: 4
                            radius: 2
                            color: thanksScreen.roleColors[modelData.role] || "#7c3aed"
                        }

                        RowLayout {
                            id: cardContent
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.leftMargin: 20
                            anchors.rightMargin: 20
                            anchors.topMargin: 16
                            anchors.bottomMargin: 16
                            spacing: 18

                            // Avatar
                            Rectangle {
                                Layout.preferredWidth: 64
                                Layout.preferredHeight: 64
                                Layout.alignment: Qt.AlignTop
                                radius: 32
                                clip: true
                                color: "#2a2a3a"
                                border.width: 2
                                border.color: thanksScreen.roleColors[modelData.role] || "#7c3aed"

                                Image {
                                    anchors.fill: parent
                                    source: modelData.avatarUrl || ""
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                    visible: modelData.avatarUrl && modelData.avatarUrl.length > 0
                                }

                                Text {
                                    anchors.centerIn: parent
                                    text: "🙂"
                                    font.pixelSize: 24
                                    visible: !modelData.avatarUrl || modelData.avatarUrl.length === 0
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 10

                                    Text {
                                        text: modelData.name
                                        color: "white"
                                        font.pixelSize: 17
                                        font.bold: true
                                    }

                                    Rectangle {
                                        radius: 12
                                        color: thanksScreen.roleColors[modelData.role] || "#7c3aed"
                                        Layout.preferredHeight: 22
                                        Layout.preferredWidth: roleLabel.implicitWidth + 18

                                        Text {
                                            id: roleLabel
                                            anchors.centerIn: parent
                                            text: modelData.role
                                            color: "white"
                                            font.pixelSize: 10
                                            font.bold: true
                                        }
                                    }

                                    Item { Layout.fillWidth: true }
                                }

                                Text {
                                    text: modelData.contribution || ""
                                    color: "#e5e0ff"
                                    font.pixelSize: 13
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                    visible: (modelData.contribution || "").length > 0
                                }

                                Text {
                                    text: modelData.note || ""
                                    color: "#8b86a8"
                                    font.pixelSize: 12
                                    font.italic: true
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                    visible: (modelData.note || "").length > 0
                                }

                                // Links: git profile + socials, as clickable chips
                                Row {
                                    Layout.topMargin: 4
                                    spacing: 8
                                    visible: (modelData.gitUrl && modelData.gitUrl.length > 0) || (modelData.socials && modelData.socials.length > 0)

                                    Rectangle {
                                        visible: modelData.gitUrl && modelData.gitUrl.length > 0
                                        width: gitRow.implicitWidth + 16
                                        height: 26
                                        radius: 13
                                        color: gitMouse.containsMouse ? "#332a55" : "#211c38"

                                        Behavior on color { ColorAnimation { duration: 120 } }

                                        Row {
                                            id: gitRow
                                            anchors.centerIn: parent
                                            spacing: 6
                                            Text { text: "🐙"; font.pixelSize: 12 }
                                            Text { text: "GitHub"; color: "white"; font.pixelSize: 11 }
                                        }

                                        MouseArea {
                                            id: gitMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: Qt.openUrlExternally(modelData.gitUrl)
                                        }
                                    }

                                    Repeater {
                                        model: modelData.socials || []

                                        delegate: Rectangle {
                                            width: 30
                                            height: 26
                                            radius: 13
                                            color: socialMouse.containsMouse ? "#332a55" : "#211c38"

                                            Behavior on color { ColorAnimation { duration: 120 } }

                                            Text {
                                                anchors.centerIn: parent
                                                text: modelData.icon
                                                font.pixelSize: 13
                                            }

                                            MouseArea {
                                                id: socialMouse
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: Qt.openUrlExternally(modelData.url)
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            propagateComposedEvents: true
                            onEntered: card.hovered = true
                            onExited: card.hovered = false
                            // let clicks fall through to the chip MouseAreas above
                            acceptedButtons: Qt.NoButton
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 32 }
        }
    }
}
