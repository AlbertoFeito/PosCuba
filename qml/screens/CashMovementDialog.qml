import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Ingreso o retiro manual de efectivo (moneda base).
Dialog {
    id: dlg
    property bool withdrawal: true
    property string errorText: ""
    signal done()

    function openFor(isWithdrawal) {
        withdrawal = isWithdrawal
        amountField.text = ""
        reasonField.text = ""
        errorText = ""
        open()
    }

    function trySave() {
        var r = cashCtrl.addMovement(amountField.text, withdrawal, reasonField.text)
        if (r.ok) {
            done()
            close()
        } else {
            errorText = r.error
        }
    }

    title: withdrawal ? qsTr("Retiro de efectivo") : qsTr("Ingreso de efectivo")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 440
    onOpened: amountField.forceActiveFocus()

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Theme.spacing
            Label { text: qsTr("Importe *") }
            TextField {
                id: amountField
                Layout.fillWidth: true
                font.pixelSize: Theme.fontSize + 4
                placeholderText: "0.00"
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onAccepted: reasonField.forceActiveFocus()
            }
            Label { text: qsTr("Motivo *") }
            TextField {
                id: reasonField
                Layout.fillWidth: true
                maximumLength: 120
                placeholderText: dlg.withdrawal ? qsTr("Ej.: pago a proveedor, depósito") : qsTr("Ej.: cambio adicional")
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
            PrimaryButton { text: qsTr("Registrar (Enter)"); onClicked: dlg.trySave() }
        }
    }
}
