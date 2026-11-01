import QtQuick 2.12
import QtQuick.Controls 2.12
import "../theme"

// Boton de accion principal (relleno con el color de acento).
Button {
    id: control
    highlighted: true

    contentItem: Label {
        text: control.text
        font: control.font
        color: Theme.accentText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 34
        radius: 4
        opacity: control.enabled ? 1.0 : 0.5
        color: control.down ? Qt.darker(Theme.accent, 1.2)
                            : (control.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent)
        border.color: control.visualFocus ? Theme.text : "transparent"
        border.width: control.visualFocus ? 2 : 0
    }
}
