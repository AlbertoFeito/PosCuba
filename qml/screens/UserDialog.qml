import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Crear usuario, editar nombre/rol o asignar un PIN nuevo (mode: "create" | "edit" | "pin").
Dialog {
    id: dlg
    onAboutToShow: help.enter("usuarios")
    onAboutToHide: help.leave("usuarios")
    property string mode: "create"
    property int userId: 0
    property string userName: ""
    property string errorText: ""
    signal done()

    readonly property bool showIdentity: mode !== "pin"
    readonly property bool showPin: mode !== "edit"

    function openCreate() {
        mode = "create"
        userId = 0
        nameField.text = ""
        roleCombo.currentIndex = 1
        pinField.text = ""
        confirmField.text = ""
        errorText = ""
        open()
    }

    function openEdit(u) {
        mode = "edit"
        userId = u.id
        userName = u.name
        nameField.text = u.name
        roleCombo.currentIndex = u.role === "admin" ? 0 : 1
        errorText = ""
        open()
    }

    function openPin(u) {
        mode = "pin"
        userId = u.id
        userName = u.name
        pinField.text = ""
        confirmField.text = ""
        errorText = ""
        open()
    }

    function trySave() {
        var role = roleCombo.currentIndex === 0 ? "admin" : "cashier"
        var r
        if (mode === "create")
            r = userCtrl.create(nameField.text, role, pinField.text, confirmField.text)
        else if (mode === "edit")
            r = userCtrl.update(userId, nameField.text, role)
        else
            r = userCtrl.resetPin(userId, pinField.text, confirmField.text)
        if (r.ok) {
            done()
            close()
        } else {
            errorText = r.error
        }
    }

    title: mode === "create" ? qsTr("Nuevo usuario")
           : mode === "edit" ? qsTr("Editar usuario") : qsTr("PIN nuevo para %1").arg(userName)
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 440
    onOpened: {
        if (showIdentity)
            nameField.forceActiveFocus()
        else
            pinField.forceActiveFocus()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Theme.spacing
            rowSpacing: 10

            Label { text: qsTr("Nombre *"); visible: dlg.showIdentity }
            TextField {
                id: nameField
                visible: dlg.showIdentity
                Layout.fillWidth: true
                maximumLength: 40
                onAccepted: dlg.showPin ? pinField.forceActiveFocus() : dlg.trySave()
                HelpTip { text: qsTr("Nombre que aparece en la pantalla de entrada, los tickets y los reportes. Único.") }
            }

            Label { text: qsTr("Rol"); visible: dlg.showIdentity }
            ComboBox {
                id: roleCombo
                visible: dlg.showIdentity
                Layout.fillWidth: true
                model: [qsTr("Administrador: acceso completo"), qsTr("Cajero: venta y caja")]
                HelpTip { text: qsTr("Administrador: todo. Cajero: solo Venta y Caja.") }
            }

            Label { text: qsTr("PIN *"); visible: dlg.showPin }
            TextField {
                id: pinField
                visible: dlg.showPin
                Layout.fillWidth: true
                echoMode: TextInput.Password
                maximumLength: 8
                inputMethodHints: Qt.ImhDigitsOnly
                placeholderText: qsTr("4 a 8 dígitos")
                onAccepted: confirmField.forceActiveFocus()
                HelpTip { text: qsTr("Solo números, de 4 a 8 dígitos.") }
            }

            Label { text: qsTr("Repetir PIN *"); visible: dlg.showPin }
            TextField {
                id: confirmField
                visible: dlg.showPin
                Layout.fillWidth: true
                echoMode: TextInput.Password
                maximumLength: 8
                inputMethodHints: Qt.ImhDigitsOnly
                onAccepted: dlg.trySave()
            }
        }

        Label {
            Layout.fillWidth: true
            visible: dlg.errorText !== ""
            text: dlg.errorText
            color: Theme.danger
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            Button { text: qsTr("Cancelar (Esc)"); onClicked: dlg.close() }
            PrimaryButton { text: qsTr("Guardar (Enter)"); onClicked: dlg.trySave() }
        }
    }
}
