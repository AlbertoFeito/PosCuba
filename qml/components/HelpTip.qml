import QtQuick 2.12
import QtQuick.Controls 2.12
import "../theme"

// Ayuda emergente al dejar el mouse quieto sobre un elemento.
// Se declara DENTRO del elemento que explica (no como hermano en un Layout):
//     Button { text: qsTr("Cobrar"); HelpTip { text: qsTr("Abre el cobro...") } }
// Se oculta al hacer clic o escribir en el elemento y no vuelve hasta que el mouse sale y entra.
// Se puede apagar desde el manual (F1) con «Mostrar ayudas al pasar el mouse».
Item {
    id: tip
    property string text: ""
    property bool active: true
    property bool dismissed: false

    readonly property string targetText: parent && parent.text !== undefined ? String(parent.text) : ""
    readonly property bool targetPressed: parent && parent.pressed === true

    onTargetTextChanged: dismissed = true
    onTargetPressedChanged: if (targetPressed) dismissed = true
    onVisibleChanged: dismissed = false

    anchors.fill: parent

    HoverHandler {
        id: hover
        enabled: tip.active && tip.text !== "" && help.showTips
        onHoveredChanged: if (!hovered) tip.dismissed = false
    }

    Timer {
        interval: 15000
        running: tt.visible
        onTriggered: tip.dismissed = true
    }

    ToolTip {
        id: tt
        parent: tip
        // tip.visible: falso si alguna pantalla o panel que lo contiene esta oculto
        // (HoverHandler puede quedar "hovered" al ocultarse el elemento).
        visible: tip.visible && hover.hovered && hover.enabled && !tip.dismissed
        delay: 700
        text: tip.text
        width: Math.min(implicitWidth, 380)
        // Centrada sobre el elemento (por defecto usa el ancho sin ajustar y se corre a un lado).
        x: Math.round((tip.width - width) / 2)
        padding: 8

        contentItem: Label {
            text: tt.text
            wrapMode: Text.Wrap
            color: Theme.text
            font.pixelSize: Theme.fontSmall + 1
        }
        background: Rectangle {
            color: Theme.surface
            border.color: Theme.accent
            radius: 4
        }
    }
}
