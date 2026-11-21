import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Abono de un cliente a su deuda. El dinero entra a la caja abierta.
Dialog {
    id: dlg
    onAboutToShow: help.enter("clientes")
    onAboutToHide: help.leave("clientes")

    property var customer: ({})
    property string errorText: ""
    property string resultText: ""
    signal paid()

    function openFor(id) {
        customer = customerCtrl.get(id)
        amountField.text = customer.ok && customer.balance > 0 ? fmt.money(customer.balance) : ""
        cashRadio.checked = true
        noteField.text = ""
        errorText = ""
        open()
    }

    function save() {
        var r = customerCtrl.registerPayment(customer.id, amountField.text, transferRadio.checked ? "transfer" : "cash",
                                             noteField.text)
        if (!r.ok) {
            errorText = r.error
            return
        }
        resultText = qsTr("Abono de %1 registrado. %2 debe ahora %3.").arg(r.amountText).arg(customer.name).arg(r.balanceText)
        close()
        paid()
    }

    title: qsTr("Registrar abono")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(480, parent.width - 40)
    closePolicy: Popup.CloseOnEscape
    onOpened: { amountField.forceActiveFocus(); amountField.selectAll() }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        Label { text: dlg.customer.name || ""; font.bold: true; font.pixelSize: Theme.fontSize + 2 }
        Label { text: qsTr("Debe: %1 %2").arg(dlg.customer.balanceText || "").arg(saleCtrl.baseCurrency); color: Theme.textMuted }

        GridLayout {
            columns: 2
            columnSpacing: Theme.spacing
            rowSpacing: 10
            Label { text: qsTr("Importe") }
            TextField {
                id: amountField
                Layout.preferredWidth: 200
                font.pixelSize: Theme.fontSize + 4
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onAccepted: dlg.save()
                HelpTip { text: qsTr("Lo que paga ahora. Viene con toda la deuda; cámbialo si paga solo una parte.") }
            }
            Label { text: qsTr("Paga con") }
            RowLayout {
                RadioButton {
                    id: cashRadio
                    text: qsTr("Efectivo")
                    checked: true
                    HelpTip { text: qsTr("El dinero entra a la gaveta: suma al efectivo esperado de la caja.") }
                }
                RadioButton { id: transferRadio; text: qsTr("Transferencia") }
            }
            Label { text: qsTr("Nota") }
            TextField {
                id: noteField
                Layout.preferredWidth: 280
                maximumLength: 120
                placeholderText: qsTr("Opcional")
                onAccepted: dlg.save()
            }
        }

        Label {
            Layout.preferredWidth: 440
            visible: dlg.errorText !== ""
            text: dlg.errorText
            color: Theme.danger
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            Button { text: qsTr("Cancelar (Esc)"); onClicked: dlg.close() }
            PrimaryButton { text: qsTr("Registrar abono (Enter)"); onClicked: dlg.save() }
        }
    }
}
