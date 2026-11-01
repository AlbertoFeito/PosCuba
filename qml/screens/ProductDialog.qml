import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

Dialog {
    id: dlg

    property int editingId: 0
    property string currentStock: ""
    property string errorText: ""
    property int savedId: 0
    signal saved()

    // Valores numericos para el calculo de ganancia (-1 = invalido)
    readonly property real costValue: costField.text.trim() === "" ? 0 : fmt.parseMoney(costField.text)
    readonly property real priceValue: fmt.parseMoney(priceField.text)

    function openNew() {
        editingId = 0
        currentStock = ""
        codeField.text = productCtrl.nextCode()
        barcodeField.text = ""
        nameField.text = ""
        categoryCombo.currentIndex = 0
        unitCombo.currentIndex = 0
        costField.text = ""
        priceField.text = ""
        minStockField.text = ""
        errorText = ""
        open()
    }

    function openEdit(productId) {
        var p = productCtrl.get(productId)
        if (!p.id)
            return
        editingId = p.id
        currentStock = p.stock + " " + p.unit
        codeField.text = p.code
        barcodeField.text = p.barcode
        nameField.text = p.name
        categoryCombo.currentIndex = productCtrl.categoryModel.rowOfId(p.categoryId)
        unitCombo.currentIndex = Math.max(0, productCtrl.units.indexOf(p.unit))
        costField.text = p.cost
        priceField.text = p.price
        minStockField.text = p.minStock === "0" ? "" : p.minStock
        errorText = ""
        open()
    }

    function trySave() {
        var r = productCtrl.save({
            "id": editingId,
            "code": codeField.text,
            "barcode": barcodeField.text,
            "name": nameField.text,
            "categoryId": productCtrl.categoryModel.idAt(categoryCombo.currentIndex),
            "cost": costField.text,
            "price": priceField.text,
            "unit": unitCombo.currentText,
            "minStock": minStockField.text
        })
        if (r.ok) {
            savedId = r.id
            saved()
            close()
        } else {
            errorText = r.error
        }
    }

    title: editingId ? qsTr("Editar producto") : qsTr("Nuevo producto")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(640, parent.width - 40)
    closePolicy: Popup.CloseOnEscape
    onOpened: nameField.forceActiveFocus()

    Shortcut {
        sequence: "Ctrl+S"
        enabled: dlg.visible && !categoryDialog.visible
        onActivated: dlg.trySave()
    }

    CategoryDialog {
        id: categoryDialog
        onCreated: categoryCombo.currentIndex = productCtrl.categoryModel.rowOfId(categoryDialog.createdId)
    }

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
                maximumLength: 120
                onAccepted: dlg.trySave()
            }

            Label { text: qsTr("Código *") }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: codeField
                    Layout.fillWidth: true
                    maximumLength: 32
                    onAccepted: dlg.trySave()
                }
                Button {
                    text: qsTr("Generar")
                    focusPolicy: Qt.NoFocus
                    onClicked: codeField.text = productCtrl.nextCode()
                }
            }

            Label { text: qsTr("Código de barras") }
            TextField {
                id: barcodeField
                Layout.fillWidth: true
                maximumLength: 64
                placeholderText: qsTr("Escanear o escribir (opcional)")
                // El lector envía Enter al final: pasar al siguiente campo, no guardar.
                onAccepted: categoryCombo.forceActiveFocus()
            }

            Label { text: qsTr("Categoría") }
            RowLayout {
                Layout.fillWidth: true
                ComboBox {
                    id: categoryCombo
                    Layout.fillWidth: true
                    model: productCtrl.categoryModel
                    textRole: "name"
                }
                Button {
                    text: "+"
                    focusPolicy: Qt.NoFocus
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Nueva categoría")
                    onClicked: categoryDialog.openNew()
                }
            }

            Label { text: qsTr("Unidad") }
            ComboBox {
                id: unitCombo
                Layout.preferredWidth: 140
                model: productCtrl.units
            }

            Label { text: qsTr("Precio de costo") }
            TextField {
                id: costField
                Layout.preferredWidth: 200
                placeholderText: "0.00"
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                color: dlg.costValue < 0 ? Theme.danger : Theme.text
                onAccepted: dlg.trySave()
            }

            Label { text: qsTr("Precio de venta *") }
            TextField {
                id: priceField
                Layout.preferredWidth: 200
                placeholderText: "0.00"
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                color: priceField.text !== "" && dlg.priceValue < 0 ? Theme.danger : Theme.text
                onAccepted: dlg.trySave()
            }

            Label { text: qsTr("Stock mínimo") }
            RowLayout {
                TextField {
                    id: minStockField
                    Layout.preferredWidth: 200
                    placeholderText: qsTr("Sin alerta")
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    onAccepted: dlg.trySave()
                }
                Label {
                    text: dlg.currentStock !== "" ? qsTr("Stock actual: %1").arg(dlg.currentStock) : ""
                    color: Theme.textMuted
                }
            }
        }

        Label {
            id: marginLabel
            Layout.fillWidth: true
            visible: dlg.priceValue >= 0 && dlg.costValue >= 0 && priceField.text !== ""
            text: {
                var gain = dlg.priceValue - dlg.costValue
                if (gain < 0)
                    return qsTr("El precio de venta es menor que el costo (pérdida de %1 por unidad).")
                           .arg(fmt.money(-gain))
                var pct = dlg.costValue > 0 ? " (" + Math.round(gain * 100 / dlg.costValue) + " %)" : ""
                return qsTr("Ganancia por unidad: %1%2").arg(fmt.money(gain)).arg(pct)
            }
            color: dlg.priceValue < dlg.costValue ? Theme.warning : Theme.success
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
            Label {
                text: qsTr("El stock se modifica en Inventario.")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Cancelar (Esc)")
                onClicked: dlg.close()
            }
            PrimaryButton {
                text: qsTr("Guardar (Ctrl+S)")
                onClicked: dlg.trySave()
            }
        }
    }
}
