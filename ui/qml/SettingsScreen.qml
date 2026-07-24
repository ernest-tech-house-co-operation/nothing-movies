import QtQuick 2.15
import QtQuick.Layouts 1.15

Item {
    id: settingsScreen

    signal openThanksRequested()

    Rectangle {
        anchors.fill: parent
        color: "#0b0b12"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 20

        Text {
            text: "Settings"
            color: "white"
            font.pixelSize: 28
            font.bold: true
        }

        Item { Layout.preferredHeight: 12 }

        Rectangle {
            id: thanksLink
            Layout.preferredWidth: 220
            Layout.preferredHeight: 48
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

        Item { Layout.preferredHeight: 4 }

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

        Item { Layout.fillHeight: true }
    }
}
