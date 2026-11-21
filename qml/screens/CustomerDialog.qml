import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Crear o editar un cliente. El limite de credito solo lo cambia un administrador.
Dialog {
    id: dlg
    onAboutToShow: help.enter("clientes")
    onAboutToHide: help.leave("clientes")

    property int customerId: 0
    property int savedId: 0
    property string errorText: ""
    signal saved()

    function openNew() {
        customerId = 0
        nameField.text = ""
        phoneField.text = ""
        noteField.text = ""
        limitField.text = ""
        errorText = ""
        open()
    }

    function openEdit(id) {
        var c = customerCtrl.get(id)
        if (!c.ok)
            return
        customerId = id
        nameField.text = c.name
        phoneField.text = c.phone
        noteField.text = c.note
        limitField.text = c.limitValue
        errorText = ""
        open()
    }

    function save() {
        var r = customerCtrl.save({
            "id": customerId,
            "name": nameField.text,
            "phone": phoneField.text,
            "note": noteField.text,
            "limit": limitField.text
        })
        if (!r.ok) {
            errorText = r.error
            return
        }
        savedId = r.id
        close()
        saved()
    }

    title: customerId > 0 ? qsTr("Editar cliente") : qsTr("Nuevo cliente")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(520, parent.width - 40)
    closePolicy: Popup.CloseOnEscape
    onOpened: nameField.forceActiveFocus()

    Shortcut { sequence: "Ctrl+S"; enabled: dlg.visible; onActivated: dlg.save() }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Theme.spacing
            rowSpacing: 10

            Label { text: qsTr("Nombre *") }
            TextField {
                id: nameField
                Layout.fillWidth: true
                maximumLength: 80
                placeholderText: qsTr("Ej.: María López (la vecina del 5)")
                onAccepted: phoneField.forceActiveFocus()
                HelpTip { text: qsTr("Cómo lo reconoces. Debe ser único: si hay dos con el mismo nombre, agrega algo que los distinga.") }
            }
            Label { text: qsTr("Teléfono") }
            TextField {
                id: phoneField
                Layout.fillWidth: true
                maximumLength: 30
                inputMethodHints: Qt.ImhDialableCharactersOnly
                onAccepted: noteField.forceActiveFocus()
                HelpTip { text: qsTr("Para avisarle cuando deba. Opcional.") }
            }
            Label { text: qsTr("Nota") }
            TextField {
                id: noteField
                Layout.fillWidth: true
                maximumLength: 200
                placeholderText: qsTr("Dirección, trabajo, quién lo recomendó…")
                onAccepted: app.isAdmin ? limitField.forceActiveFocus() : dlg.save()
            }
            Label { text: qsTr("Límite de crédito") }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: limitField
                    Layout.preferredWidth: 160
                    enabled: app.isAdmin
                    placeholderText: qsTr("sin límite")
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    onAccepted: dlg.save()
                    HelpTip { text: qsTr("Lo máximo que puede deber. Vacío = sin límite. Al cobrar a crédito, no deja pasar de este número. Solo lo cambia un administrador.") }
                }
                Label { text: saleCtrl.baseCurrency; color: Theme.textMuted }
            }
        }

        Label {
            Layout.preferredWidth: 480
            visible: dlg.errorText !== ""
            text: dlg.errorText
            color: Theme.danger
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            Button { text: qsTr("Cancelar (Esc)"); onClicked: dlg.close() }
            PrimaryButton { text: qsTr("Guardar (Ctrl+S)"); onClicked: dlg.save() }
        }
    }
}
