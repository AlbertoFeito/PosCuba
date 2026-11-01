import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Cambiar el PIN propio.
Dialog {
    id: dlg
    property string errorText: ""
    signal done()

    function openNew() {
        oldField.text = ""
        newField.text = ""
        confirmField.text = ""
        errorText = ""
        open()
    }

    function trySave() {
        var r = app.changeOwnPin(oldField.text, newField.text, confirmField.text)
        if (r.ok) {
            done()
            close()
        } else {
            errorText = r.error
        }
    }

    title: qsTr("Cambiar mi PIN")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 400
    onOpened: oldField.forceActiveFocus()

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing
        TextField {
            id: oldField
            Layout.fillWidth: true
            echoMode: TextInput.Password
            maximumLength: 8
            inputMethodHints: Qt.ImhDigitsOnly
            placeholderText: qsTr("PIN actual")
            onAccepted: newField.forceActiveFocus()
        }
        TextField {
            id: newField
            Layout.fillWidth: true
            echoMode: TextInput.Password
            maximumLength: 8
            inputMethodHints: Qt.ImhDigitsOnly
            placeholderText: qsTr("PIN nuevo (4 a 8 dígitos)")
            onAccepted: confirmField.forceActiveFocus()
        }
        TextField {
            id: confirmField
            Layout.fillWidth: true
            echoMode: TextInput.Password
            maximumLength: 8
            inputMethodHints: Qt.ImhDigitsOnly
            placeholderText: qsTr("Repite el PIN nuevo")
            onAccepted: dlg.trySave()
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
