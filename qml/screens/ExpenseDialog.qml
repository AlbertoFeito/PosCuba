import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Registrar un gasto. Si se paga con dinero de la caja, se descuenta de la caja abierta.
Dialog {
    id: dlg
    onAboutToShow: help.enter("gastos")
    onAboutToHide: help.leave("gastos")

    property string errorText: ""
    signal saved()

    function categoryIndex(id) {
        for (var i = 0; i < expenseCtrl.categories.length; ++i) {
            if (expenseCtrl.categories[i].id === id)
                return i
        }
        return -1
    }

    function openNew() {
        dateField.text = expenseCtrl.todayText()
        categoryCombo.currentIndex = -1
        descriptionField.text = ""
        amountField.text = ""
        fromCash.checked = cashCtrl.isOpen
        newCategory.visible = false
        newCategoryField.text = ""
        errorText = ""
        open()
    }

    function save() {
        var cat = categoryCombo.currentIndex >= 0 ? expenseCtrl.categories[categoryCombo.currentIndex].id : 0
        var r = expenseCtrl.add({
            "dateText": dateField.text,
            "categoryId": cat,
            "description": descriptionField.text,
            "amount": amountField.text,
            "fromCash": fromCash.checked
        })
        if (!r.ok) {
            errorText = r.error
            return
        }
        close()
        saved()
    }

    function addCategory() {
        var r = expenseCtrl.addCategory(newCategoryField.text)
        if (!r.ok) {
            errorText = r.error
            return
        }
        categoryCombo.currentIndex = categoryIndex(r.id)
        newCategory.visible = false
        newCategoryField.text = ""
        errorText = ""
        descriptionField.forceActiveFocus()
    }

    title: qsTr("Nuevo gasto")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(560, parent.width - 40)
    closePolicy: Popup.CloseOnEscape
    onOpened: categoryCombo.forceActiveFocus()

    Shortcut { sequence: "Ctrl+S"; enabled: dlg.visible; onActivated: dlg.save() }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Theme.spacing
            rowSpacing: 10

            Label { text: qsTr("Categoría *") }
            RowLayout {
                Layout.fillWidth: true
                ComboBox {
                    id: categoryCombo
                    Layout.fillWidth: true
                    model: expenseCtrl.categories
                    textRole: "name"
                    displayText: currentIndex >= 0 ? currentText : qsTr("Elegir…")
                    HelpTip { text: qsTr("Agrupa los gastos para ver en qué se va el dinero (alquiler, electricidad, salarios…).") }
                }
                Button {
                    text: "+"
                    implicitWidth: 36
                    focusPolicy: Qt.NoFocus
                    onClicked: { newCategory.visible = true; newCategoryField.forceActiveFocus() }
                    HelpTip { text: qsTr("Crear una categoría de gasto nueva.") }
                }
            }

            Item { width: 1; height: 1; visible: newCategory.visible }
            RowLayout {
                id: newCategory
                visible: false
                Layout.fillWidth: true
                TextField {
                    id: newCategoryField
                    Layout.fillWidth: true
                    maximumLength: 60
                    placeholderText: qsTr("Nombre de la categoría nueva")
                    onAccepted: dlg.addCategory()
                }
                Button { text: qsTr("Crear"); onClicked: dlg.addCategory() }
            }

            Label { text: qsTr("Descripción *") }
            TextField {
                id: descriptionField
                Layout.fillWidth: true
                maximumLength: 120
                placeholderText: qsTr("Ej.: Corriente de noviembre, taxi a la tienda…")
                onAccepted: amountField.forceActiveFocus()
            }

            Label { text: qsTr("Importe *") }
            RowLayout {
                TextField {
                    id: amountField
                    Layout.preferredWidth: 180
                    font.pixelSize: Theme.fontSize + 3
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    onAccepted: dlg.save()
                }
                Label { text: saleCtrl.baseCurrency; color: Theme.textMuted }
            }

            Label { text: qsTr("Fecha") }
            TextField {
                id: dateField
                Layout.preferredWidth: 140
                enabled: !fromCash.checked
                inputMask: "99/99/9999"
                onAccepted: dlg.save()
                HelpTip { text: qsTr("Día del gasto (dd/mm/aaaa). Puede ser anterior, por ejemplo una factura de la semana pasada. Un gasto pagado de la caja siempre es de hoy.") }
            }

            Item { width: 1; height: 1 }
            CheckBox {
                id: fromCash
                text: qsTr("Pagado con dinero de la caja")
                enabled: cashCtrl.isOpen
                onToggled: if (checked) dateField.text = expenseCtrl.todayText()
                HelpTip { text: qsTr("Marcado: el importe sale de la gaveta y se registra como retiro en la caja abierta. Sin marcar: se pagó con otro dinero (banco, bolsillo del dueño) y la caja no cambia.") }
            }
        }

        Label {
            Layout.preferredWidth: 520
            visible: !cashCtrl.isOpen
            text: qsTr("La caja está cerrada: el gasto se registra como pagado fuera de la caja.")
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.Wrap
        }

        Label {
            Layout.preferredWidth: 520
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
