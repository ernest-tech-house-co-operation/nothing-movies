import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: splash
    color: "#0d0d0d"
    signal finished()

    // Point this at your actual hosted logo image
    property url logoUrl: "https://example.com/logo.png"

    // Set to true right after the user taps Agree; call splash.appReady()
    // from Main.qml once your actual startup work (loading providers, etc.) is done.
    property bool loading: false
    property bool readySignaled: false

    // Call this from outside (e.g. Main.qml) when the app has finished
    // whatever it needed to do in the background.
    function appReady() {
        readySignaled = true
        if (loading && !minSpinnerTimer.running) {
            splash.finished()
        }
    }

    // Keeps the spinner visible at least this long so it doesn't just flash
    Timer {
        id: minSpinnerTimer
        interval: 600
        onTriggered: {
            if (splash.readySignaled) {
                splash.finished()
            }
        }
    }

    property string privacyPolicyText:
        "Privacy Policy\n\n" +
        "Nothing Movies (\"we\", \"us\") respects your privacy. This policy explains what information we collect and how we use it.\n\n" +
        "1. Information We Collect\n" +
        "We may collect basic device information, app usage data, and any information you voluntarily provide (such as an account email).\n\n" +
        "2. How We Use It\n" +
        "We use collected information to operate and improve the app, personalize your experience, and communicate important updates.\n\n" +
        "3. Sharing\n" +
        "We do not sell your personal data. We may share limited data with service providers who help us run the app, under confidentiality obligations.\n\n" +
        "4. Data Security\n" +
        "We take reasonable measures to protect your data, but no method of transmission or storage is 100% secure.\n\n" +
        "5. Your Choices\n" +
        "You may request access to, correction of, or deletion of your personal data by contacting us.\n\n" +
        "6. Changes\n" +
        "We may update this policy from time to time. Continued use of the app after changes constitutes acceptance.\n\n" +
        "By tapping Agree, you acknowledge that you have read and understood this Privacy Policy."

    property string termsOfUseText:
        "Terms of Use\n\n" +
        "Welcome to Nothing Movies. By using this app, you agree to the following terms.\n\n" +
        "1. Use of the App\n" +
        "You agree to use the app only for lawful purposes and in accordance with these terms.\n\n" +
        "2. Content\n" +
        "All content provided through the app is for personal, non-commercial use unless otherwise stated.\n\n" +
        "3. Accounts\n" +
        "If you create an account, you are responsible for maintaining the confidentiality of your credentials.\n\n" +
        "4. Intellectual Property\n" +
        "All trademarks, logos, and content within the app remain the property of Nothing Movies or its licensors.\n\n" +
        "5. Limitation of Liability\n" +
        "The app is provided \"as is\" without warranties of any kind. We are not liable for any indirect or consequential damages arising from your use of the app.\n\n" +
        "6. Termination\n" +
        "We reserve the right to suspend or terminate access to the app for violation of these terms.\n\n" +
        "7. Changes to Terms\n" +
        "We may revise these terms at any time. Continued use after changes means you accept the revised terms.\n\n" +
        "By tapping Agree, you accept these Terms of Use."

    Column {
        id: content
        anchors.centerIn: parent
        width: parent.width * 0.85
        spacing: 26
        opacity: 0

        NumberAnimation on opacity {
            to: 1
            duration: 600
            easing.type: Easing.OutCubic
        }

        Image {
            id: logoImage
            source: splash.logoUrl
            anchors.horizontalCenter: parent.horizontalCenter
            width: 110
            height: 110
            fillMode: Image.PreserveAspectFit
            visible: status === Image.Ready
        }

        Text {
            id: logoText
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Nothing Movies"
            color: "white"
            font.family: "Century Schoolbook"
            font.bold: true
            font.pixelSize: 34
            font.letterSpacing: 2
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 14

            Text {
                text: "Privacy Policy"
                color: "#8ab4f8"
                font.pixelSize: 14
                font.underline: true
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: overlay.showPolicy("privacy")
                }
            }

            Text {
                text: "•"
                color: "#555555"
                font.pixelSize: 14
            }

            Text {
                text: "Terms of Use"
                color: "#8ab4f8"
                font.pixelSize: 14
                font.underline: true
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: overlay.showPolicy("terms")
                }
            }
        }

        Button {
            id: agreeButton
            text: "Agree & Continue"
            anchors.horizontalCenter: parent.horizontalCenter
            implicitWidth: 220
            implicitHeight: 46
            visible: !splash.loading

            onClicked: {
                splash.loading = true
                minSpinnerTimer.start()
            }

            background: Rectangle {
                radius: 8
                color: agreeButton.down ? "#c62828" : "#e53935"
            }

            contentItem: Text {
                text: agreeButton.text
                color: "white"
                font.bold: true
                font.pixelSize: 15
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        // Spinner shown in place of the button while loading == true
        Item {
            id: spinner
            visible: splash.loading
            anchors.horizontalCenter: parent.horizontalCenter
            width: 32
            height: 32

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: "transparent"
                border.width: 3
                border.color: "#3a3a3a"
            }

            Rectangle {
                width: parent.width
                height: parent.height
                radius: width / 2
                color: "transparent"
                border.width: 3
                border.color: "#e53935"
                // Trick to only show a quarter-arc: mask via opacity gradient isn't
                // available on border, so we rotate a small arc segment instead.
                Rectangle {
                    width: 3
                    height: parent.height / 2
                    color: "#e53935"
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                }

                RotationAnimation on rotation {
                    running: splash.loading
                    loops: Animation.Infinite
                    from: 0
                    to: 360
                    duration: 800
                }
            }
        }
    }

    // Terms / Privacy overlay
    Rectangle {
        id: overlay
        anchors.fill: parent
        color: "#000000cc"
        visible: false
        z: 10

        property string currentType: "privacy"

        function showPolicy(type) {
            overlay.currentType = type
            overlay.visible = true
        }

        MouseArea {
            anchors.fill: parent
            onClicked: overlay.visible = false
        }

        Rectangle {
            width: parent.width * 0.88
            height: parent.height * 0.75
            anchors.centerIn: parent
            color: "#1a1a1a"
            radius: 12
            border.color: "#333333"
            border.width: 1

            MouseArea {
                anchors.fill: parent
                onClicked: {} // absorb clicks so they don't close the overlay
            }

            Column {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 14

                Text {
                    text: overlay.currentType === "privacy" ? "Privacy Policy" : "Terms of Use"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 20
                }

                Flickable {
                    width: parent.width
                    height: parent.height - 90
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
                        text: overlay.currentType === "privacy" ? splash.privacyPolicyText : splash.termsOfUseText
                    }
                }

                Button {
                    text: "Close"
                    anchors.right: parent.right
                    onClicked: overlay.visible = false
                }
            }
        }
    }
}