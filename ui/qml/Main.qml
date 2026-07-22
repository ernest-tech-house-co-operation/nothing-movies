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
            onFinished: rootStack.replace(shellComponent)
        }
    }

    Component {
        id: shellComponent
        Shell {}
    }
}
