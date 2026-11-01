import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

Dialog {
    id: dlg
    onAboutToShow: help.enter("productos")
    onAboutToHide: help.leave("productos")
    // Sin parametros en la señal: Qt 6 desaconseja inyectarlos en los handlers.
    property int createdId: 0
    signal created()

    function openNew() {
        nameField.text = ""
        errorLabel.text = ""
        open()
    }

    function tryCreate() {
        var r = productCtrl.addCategory(nameField.text)
        if (r.ok) {
            createdId = r.id
            created()
            close()
        } else {
            errorLabel.text = r.error
        }
    }

    title: qsTr("Nueva categoría")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 380
    onOpened: nameField.forceActiveFocus()

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        TextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: qsTr("Nombre de la categoría")
            onAccepted: dlg.tryCreate()
        }
        Label {
            id: errorLabel
            visible: text !== ""
            color: Theme.danger
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight
            Button { text: qsTr("Cancelar"); onClicked: dlg.close() }
            PrimaryButton { text: qsTr("Crear"); onClicked: dlg.tryCreate() }
        }
    }
}
