import QtQuick 2.12
import QtQuick.Controls 2.12
import "../theme"

// Boton «?» que abre el manual en una seccion.
Button {
    id: control
    property string topic: ""

    text: "?"
    font.bold: true
    font.pixelSize: Theme.fontSize + 1
    focusPolicy: Qt.NoFocus
    implicitWidth: 34
    implicitHeight: 34

    onClicked: {
        var w = ApplicationWindow.window
        if (w && w.openHelp)
            w.openHelp(control.topic)
    }

    background: Rectangle {
        radius: width / 2
        color: control.down ? Theme.selection : (control.hovered ? Theme.surfaceAlt : Theme.surface)
        border.color: control.hovered ? Theme.accent : Theme.border
    }

    HelpTip { text: qsTr("Manual: %1 (F1)").arg(help.title(control.topic)) }
}
