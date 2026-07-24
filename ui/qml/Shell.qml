import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: shell

    Rectangle {
        anchors.fill: parent
        color: "#0b0b12"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Top navigation bar
        Rectangle {
            id: topBar
            Layout.fillWidth: true
            Layout.preferredHeight: 64

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

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                spacing: 24

                Text {
                    text: "Nothing Movies"
                    color: "white"
                    font.pixelSize: 18
                    font.bold: true

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.bottom
                        anchors.topMargin: 2
                        height: 2
                        radius: 1
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "#7c3aed" }
                            GradientStop { position: 1.0; color: "#06b6d4" }
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Row {
                    spacing: 8

                    Repeater {
                        model: [
                            { label: "Home",      icon: "🏠" },
                            { label: "Search",    icon: "🔍" },
                            { label: "Downloads", icon: "⬇" },
                            { label: "Sources",   icon: "🔌" },
                            { label: "Thanks",    icon: "💜" },
                            { label: "Settings",  icon: "⚙" }
                        ]
                        delegate: NavItem {
                            label: modelData.label
                            icon: modelData.icon
                            selected: contentStack.currentIndex === index
                            onClicked: contentStack.currentIndex = index
                        }
                    }
                }
            }
        }

        StackLayout {
            id: contentStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: 0

            HomeScreen {}

            Item {
                id: searchSection

                StackView {
                    id: searchStack
                    anchors.fill: parent
                    initialItem: searchScreenComponent
                }

                Component {
                    id: searchScreenComponent
                    SearchScreen {
                        onResultSelected: (result) => {
                            searchStack.push(infoScreenComponent, { result: result })
                        }
                    }
                }

                Component {
                    id: infoScreenComponent
                    InfoScreen {
                        onBackRequested: searchStack.pop()
                    }
                }
            }

            DownloadsScreen {}
            SourcesScreen {}
            ThanksScreen {}
            SettingsScreen {
                onOpenThanksRequested: contentStack.currentIndex = 4
            }
        }
    }
}
