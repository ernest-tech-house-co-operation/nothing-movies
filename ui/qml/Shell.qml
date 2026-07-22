import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: shell

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 90
            Layout.fillHeight: true
            color: "#151515"

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 20
                spacing: 24

                Repeater {
                    model: [
                        { label: "Home",      icon: "🏠" },
                        { label: "Search",    icon: "🔍" },
                        { label: "Downloads", icon: "⬇" },
                        { label: "Player",    icon: "▶" },
                        { label: "Sources",   icon: "🔌" },
                        { label: "Settings",  icon: "⚙" }
                    ]
                    delegate: NavItem {
                        Layout.alignment: Qt.AlignHCenter
                        label: modelData.label
                        icon: modelData.icon
                        selected: contentStack.currentIndex === index
                        onClicked: contentStack.currentIndex = index
                    }
                }
                Item { Layout.fillHeight: true }
            }
        }

        StackLayout {
            id: contentStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: 0

            HomeScreen {}
            SearchScreen {}
            DownloadsScreen {}
            PlayerScreen {}
            SourcesScreen {}
            SettingsScreen {}
        }
    }
}
