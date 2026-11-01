import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

Dialog {
    id: dlg
    onAboutToShow: help.enter("venta")
    onAboutToHide: help.leave("venta")
    property string errorText: ""

    function openNew() {
        valueField.text = ""
        errorText = ""
        open()
    }

    function apply(value) {
        var r = saleCtrl.setDiscount(value, percentRadio.checked)
        if (r.ok)
            close()
        else
            errorText = r.error
    }

    title: qsTr("Descuento a la venta")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 420
    onOpened: valueField.forceActiveFocus()

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        Label { text: qsTr("Subtotal: %1").arg(saleCtrl.subtotalText); color: Theme.textMuted }
        RowLayout {
            RadioButton { id: amountRadio; text: qsTr("Importe"); checked: true; focusPolicy: Qt.NoFocus; HelpTip { text: qsTr("Descuento de un importe fijo en la moneda base, por ejemplo 50.00.") } }
            RadioButton { id: percentRadio; text: qsTr("Porcentaje (%)"); focusPolicy: Qt.NoFocus; HelpTip { text: qsTr("Descuento como porcentaje del subtotal, por ejemplo 10.") } }
        }
        TextField {
            id: valueField
            Layout.fillWidth: true
            font.pixelSize: Theme.fontSize + 4
            placeholderText: percentRadio.checked ? qsTr("Ej.: 10") : qsTr("Ej.: 50.00")
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            onAccepted: dlg.apply(valueField.text)
            Keys.onTabPressed: {
                if (amountRadio.checked) percentRadio.checked = true
                else amountRadio.checked = true
            }
            HelpTip { text: qsTr("Valor del descuento. Tab cambia entre importe y porcentaje. No puede superar el subtotal.") }
        }
        Label {
            text: qsTr("Tab: cambiar entre importe y porcentaje")
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
        }
        Label {
            Layout.fillWidth: true
            visible: dlg.errorText !== ""
            text: dlg.errorText
            color: Theme.danger
            wrapMode: Text.Wrap
        }
        RowLayout {
            Layout.fillWidth: true
            Button { text: qsTr("Quitar descuento"); onClicked: dlg.apply(""); HelpTip { text: qsTr("Elimina el descuento de esta venta.") } }
            Item { Layout.fillWidth: true }
            Button { text: qsTr("Cancelar (Esc)"); onClicked: dlg.close() }
            PrimaryButton { text: qsTr("Aplicar (Enter)"); onClicked: dlg.apply(valueField.text) }
        }
    }
}
