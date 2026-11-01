import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"

ItemDelegate {
    id: control
    property bool selected: false
    property string shortcutText: ""
    property bool alert: false

    implicitHeight: 48
    focusPolicy: Qt.NoFocus

    contentItem: RowLayout {
        spacing: 8
        Label {
            text: control.text
            font.bold: control.selected
            color: !control.enabled ? Theme.textMuted : (control.alert ? Theme.danger : Theme.text)
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
        Label {
            text: control.shortcutText
            font.pixelSize: Theme.fontSmall
            color: Theme.textMuted
        }
    }

    background: Rectangle {
        color: control.selected ? Theme.selection
                                : (control.hovered ? Theme.surfaceAlt : "transparent")
        Rectangle {
            width: 4
            height: parent.height
            color: Theme.accent
            visible: control.selected
        }
    }
}
