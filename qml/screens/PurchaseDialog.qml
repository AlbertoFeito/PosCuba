import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Entrada de mercancia (compra) para el producto seleccionado.
Dialog {
    id: dlg

    property var product: ({})
    property string errorText: ""
    signal done()

    readonly property real qtyValue: fmt.parseQty(qtyField.text)

    function openFor(p) {
        product = p
        qtyField.text = ""
        costField.text = p.costText
        updateCost.checked = true
        noteField.text = ""
        errorText = ""
        open()
    }

    function trySave() {
        var r = inventoryCtrl.registerPurchase({
            "productId": product.id,
            "qty": qtyField.text,
            "cost": costField.text,
            "updateCost": updateCost.checked,
            "note": noteField.text
        })
        if (r.ok) {
            done()
            close()
        } else {
            errorText = r.error
        }
    }

    title: qsTr("Entrada de mercancía")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(560, parent.width - 40)
    closePolicy: Popup.CloseOnEscape
    onOpened: qtyField.forceActiveFocus()

    Shortcut {
        sequence: "Ctrl+S"
        enabled: dlg.visible
        onActivated: dlg.trySave()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        Label {
            Layout.fillWidth: true
            text: dlg.product.name || ""
            font.bold: true
            font.pixelSize: Theme.fontSize + 2
            elide: Text.ElideRight
        }
        Label {
            text: qsTr("Stock actual: %1 %2").arg(dlg.product.stockNegative ? "-" + dlg.product.stockText : dlg.product.stockText)
                                             .arg(dlg.product.unit || "")
            color: Theme.textMuted
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Theme.spacing
            rowSpacing: 10

            Label { text: qsTr("Cantidad *") }
            RowLayout {
                TextField {
                    id: qtyField
                    Layout.preferredWidth: 160
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    color: text !== "" && dlg.qtyValue < 0 ? Theme.danger : Theme.text
                    onAccepted: dlg.trySave()
                }
                Label { text: dlg.product.unit || ""; color: Theme.textMuted }
            }

            Label { text: qsTr("Costo unitario") }
            TextField {
                id: costField
                Layout.preferredWidth: 160
                placeholderText: qsTr("Opcional")
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onAccepted: dlg.trySave()
            }

            Item { width: 1; height: 1 }
            CheckBox {
                id: updateCost
                text: qsTr("Actualizar el costo del producto")
                enabled: costField.text.trim() !== ""
            }

            Label { text: qsTr("Proveedor / nota") }
            TextField {
                id: noteField
                Layout.fillWidth: true
                maximumLength: 200
                onAccepted: dlg.trySave()
            }
        }

        Label {
            Layout.fillWidth: true
            visible: dlg.qtyValue > 0
            text: qsTr("Quedará en stock: %1 %2").arg(fmt.qty(dlg.product.stockMilli + dlg.qtyValue)).arg(dlg.product.unit || "")
            color: Theme.success
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
            PrimaryButton { text: qsTr("Registrar entrada (Enter)"); onClicked: dlg.trySave() }
        }
    }
}
