import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    width: 1280
    height: 800

    StackView {
        id: rootStack
        anchors.fill: parent
        initialItem: splashComponent
    }

    Component {
        id: splashComponent
        Splash {
            id: splashItem

            onLoadingChanged: {
                if (loading) {
                    // This is where th real startup work goes: initializing
                    // providers, checking auth/token, warming caches, etc.
                    // Call splashItem.appReady() the moment that work resolves.
                    // If Shell (or something it depends on) has its own signal
                    // for "ready", connect to that instead of this placeholder.
                    startupTimer.start()
                }
            }

            onFinished: rootStack.replace(shellComponent)

            Timer {
                id: startupTimer
                interval: 400
                onTriggered: splashItem.appReady()
            }
        }
    }

    Component {
        id: shellComponent
        Shell {}
    }
}