import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

Item {
    id: root

    function notify(message, isError) {
        var w = ApplicationWindow.window
        if (w && w.showToast)
            w.showToast(message, isError)
    }

    function current() {
        return list.currentIndex >= 0 && list.currentIndex < userCtrl.users.length ? userCtrl.users[list.currentIndex] : null
    }

    function toggleActive() {
        var u = current()
        if (!u)
            return
        var r = userCtrl.setActive(u.id, !u.active)
        notify(r.ok ? (u.active ? qsTr("Usuario desactivado.") : qsTr("Usuario reactivado.")) : r.error, !r.ok)
    }

    onVisibleChanged: if (visible) { userCtrl.refresh(); list.forceActiveFocus() }

    Shortcut { sequence: "Ctrl+N"; enabled: root.visible && !userDialog.visible; onActivated: userDialog.openCreate() }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: Theme.spacing

        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("Usuarios"); font.pixelSize: Theme.fontLarge; font.bold: true; Layout.fillWidth: true }
            HelpButton { topic: "usuarios" }
            PrimaryButton { text: qsTr("Nuevo usuario (Ctrl+N)"); focusPolicy: Qt.NoFocus; onClicked: userDialog.openCreate(); HelpTip { text: qsTr("Crea un cajero o administrador con su PIN.") } }
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.textMuted
            text: qsTr("Administrador: acceso completo. Cajero: solo Venta y Caja. Cada venta y movimiento queda registrado con el usuario que lo hizo.")
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surface
            border.color: Theme.border
            radius: Theme.radius
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 0
                Rectangle {
                    Layout.fillWidth: true
                    height: 34
                    color: Theme.surfaceAlt
                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        Label { width: list.colName; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Nombre"); font.bold: true; color: Theme.textMuted }
                        Label { width: 160; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Rol"); font.bold: true; color: Theme.textMuted }
                        Label { width: 120; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Estado"); font.bold: true; color: Theme.textMuted }
                        Label { width: 180; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Último acceso"); font.bold: true; color: Theme.textMuted }
                    }
                }
                ListView {
                    id: list
                    readonly property int colName: Math.max(160, width - 24 - 160 - 120 - 180)
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: userCtrl.users
                    highlightMoveDuration: 0
                    boundsBehavior: Flickable.StopAtBounds
                    Keys.onReturnPressed: { var u = root.current(); if (u) userDialog.openEdit(u) }
                    Keys.onEnterPressed: { var u = root.current(); if (u) userDialog.openEdit(u) }
                    Keys.onDeletePressed: root.toggleActive()

                    delegate: Rectangle {
                        width: list.width
                        height: Theme.rowHeight
                        color: ListView.isCurrentItem ? Theme.selection : (index % 2 ? Theme.surfaceAlt : Theme.surface)
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            opacity: modelData.active ? 1.0 : 0.5
                            Label {
                                width: list.colName; height: parent.height; verticalAlignment: Text.AlignVCenter
                                text: modelData.name + (modelData.isSelf ? "  " + qsTr("(tú)") : "")
                                font.bold: modelData.isSelf
                                elide: Text.ElideRight
                            }
                            Label { width: 160; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.roleText }
                            Label {
                                width: 120; height: parent.height; verticalAlignment: Text.AlignVCenter
                                text: !modelData.active ? qsTr("Inactivo") : (modelData.hasPin ? qsTr("Activo") : qsTr("Sin PIN"))
                                color: !modelData.active ? Theme.textMuted : (modelData.hasPin ? Theme.success : Theme.warning)
                            }
                            Label { width: 180; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.lastLoginText; color: Theme.textMuted }
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: { list.currentIndex = index; list.forceActiveFocus() }
                            onDoubleClicked: { list.currentIndex = index; userDialog.openEdit(modelData) }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Enter: editar · Supr: desactivar/reactivar")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Editar")
                enabled: root.current() !== null
                focusPolicy: Qt.NoFocus
                onClicked: userDialog.openEdit(root.current())
                HelpTip { text: qsTr("Cambia el nombre o el rol (también Enter o doble clic).") }
            }
            Button {
                text: qsTr("Asignar PIN")
                enabled: root.current() !== null
                focusPolicy: Qt.NoFocus
                onClicked: userDialog.openPin(root.current())
                HelpTip { text: qsTr("Pone un PIN nuevo sin conocer el anterior: para quien lo olvidó.") }
            }
            Button {
                text: root.current() && !root.current().active ? qsTr("Reactivar") : qsTr("Desactivar")
                enabled: root.current() !== null
                focusPolicy: Qt.NoFocus
                onClicked: root.toggleActive()
                HelpTip { text: qsTr("Un usuario desactivado no puede entrar, pero su historial se conserva. Siempre debe quedar un administrador activo.") }
            }
        }
    }

    UserDialog {
        id: userDialog
        onDone: root.notify(qsTr("Usuario guardado."), false)
        onClosed: list.forceActiveFocus()
    }
}
