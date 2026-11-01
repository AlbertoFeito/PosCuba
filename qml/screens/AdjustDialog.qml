import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Ajuste de stock: se indica el stock real contado y el motivo.
Dialog {
    id: dlg

    property var product: ({})
    property string errorText: ""
    signal done()

    readonly property real countedValue: fmt.parseQty(countedField.text)
    readonly property real diff: countedValue - (product.stockMilli || 0)

    function openFor(p) {
        product = p
        countedField.text = ""
        reasonCombo.currentIndex = 0
        noteField.text = ""
        errorText = ""
        open()
    }

    function trySave() {
        var r = inventoryCtrl.adjust({
            "productId": product.id,
            "counted": countedField.text,
            "reason": reasonCombo.currentText,
            "note": noteField.text
        })
        if (r.ok) {
            done()
            close()
        } else {
            errorText = r.error
        }
    }

    title: qsTr("Ajuste de stock")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(560, parent.width - 40)
    closePolicy: Popup.CloseOnEscape
    onOpened: countedField.forceActiveFocus()

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
            text: qsTr("Stock en el sistema: %1 %2").arg(dlg.product.stockNegative ? "-" + dlg.product.stockText : dlg.product.stockText)
                                                    .arg(dlg.product.unit || "")
            color: Theme.textMuted
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Theme.spacing
            rowSpacing: 10

            Label { text: qsTr("Stock contado *") }
            RowLayout {
                TextField {
                    id: countedField
                    Layout.preferredWidth: 160
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    color: text !== "" && dlg.countedValue < 0 ? Theme.danger : Theme.text
                    onAccepted: dlg.trySave()
                }
                Label { text: dlg.product.unit || ""; color: Theme.textMuted }
            }

            Label { text: qsTr("Motivo *") }
            ComboBox {
                id: reasonCombo
                Layout.preferredWidth: 260
                model: inventoryCtrl.adjustReasons
            }

            Label { text: qsTr("Nota") }
            TextField {
                id: noteField
                Layout.fillWidth: true
                maximumLength: 200
                placeholderText: qsTr("Opcional")
                onAccepted: dlg.trySave()
            }
        }

        Label {
            Layout.fillWidth: true
            visible: countedField.text !== "" && dlg.countedValue >= 0
            text: dlg.diff === 0 ? qsTr("Sin diferencia.")
                                 : qsTr("Diferencia: %1%2 %3").arg(dlg.diff > 0 ? "+" : "-")
                                                             .arg(fmt.qty(Math.abs(dlg.diff)))
                                                             .arg(dlg.product.unit || "")
            color: dlg.diff < 0 ? Theme.danger : (dlg.diff > 0 ? Theme.success : Theme.textMuted)
            font.bold: true
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
            PrimaryButton { text: qsTr("Aplicar ajuste (Enter)"); onClicked: dlg.trySave() }
        }
    }
}
