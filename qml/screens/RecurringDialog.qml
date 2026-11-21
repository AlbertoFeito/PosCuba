import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Gastos fijos mensuales: se registran solos cada mes (alquiler, salarios, cuota fija…).
Dialog {
    id: dlg
    onAboutToShow: help.enter("gastos")
    onAboutToHide: help.leave("gastos")

    property int editId: 0
    property string errorText: ""

    function clearForm() {
        editId = 0
        categoryCombo.currentIndex = -1
        descriptionField.text = ""
        amountField.text = ""
        dayField.value = 1
        errorText = ""
    }

    function edit(r) {
        editId = r.id
        for (var i = 0; i < expenseCtrl.categories.length; ++i) {
            if (expenseCtrl.categories[i].id === r.categoryId)
                categoryCombo.currentIndex = i
        }
        descriptionField.text = r.description
        amountField.text = r.amountText
        dayField.value = r.day
        errorText = ""
        descriptionField.forceActiveFocus()
    }

    function save() {
        var r = expenseCtrl.saveRecurring({
            "id": editId,
            "categoryId": categoryCombo.currentIndex >= 0 ? expenseCtrl.categories[categoryCombo.currentIndex].id : 0,
            "description": descriptionField.text,
            "amount": amountField.text,
            "day": dayField.value
        })
        if (!r.ok) {
            errorText = r.error
            return
        }
        clearForm()
    }

    title: qsTr("Gastos fijos mensuales")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(820, parent.width - 40)
    height: Math.min(600, parent.height - 40)
    closePolicy: Popup.CloseOnEscape
    onOpened: { clearForm(); categoryCombo.forceActiveFocus() }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        Label {
            Layout.preferredWidth: 760
            wrapMode: Text.Wrap
            color: Theme.textMuted
            text: qsTr("Cada mes, el día indicado, el gasto se registra solo como pagado fuera de la caja. Si el mes es más corto (día 31 en febrero), se registra el último día. Empieza este mes.")
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surface
            border.color: Theme.border
            radius: Theme.radius
            clip: true
            ListView {
                id: list
                anchors.fill: parent
                anchors.margins: 1
                clip: true
                model: expenseCtrl.recurring
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { }
                delegate: Rectangle {
                    width: list.width
                    height: 44
                    color: index % 2 ? Theme.surfaceAlt : Theme.surface
                    opacity: modelData.active ? 1.0 : 0.5
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        Column {
                            Layout.fillWidth: true
                            Label { text: modelData.description; font.bold: true; width: parent.width; elide: Text.ElideRight }
                            Label {
                                text: qsTr("%1 · día %2 · último: %3").arg(modelData.categoryName).arg(modelData.day).arg(modelData.lastText)
                                      + (modelData.active ? "" : " · " + qsTr("en pausa"))
                                color: Theme.textMuted
                                font.pixelSize: Theme.fontSmall
                            }
                        }
                        Label { text: modelData.amountText; font.bold: true }
                        Button { text: qsTr("Editar"); focusPolicy: Qt.NoFocus; onClicked: dlg.edit(modelData) }
                        Button {
                            text: modelData.active ? qsTr("Pausar") : qsTr("Reanudar")
                            focusPolicy: Qt.NoFocus
                            onClicked: {
                                var r = expenseCtrl.setRecurringActive(modelData.id, !modelData.active)
                                if (!r.ok)
                                    dlg.errorText = r.error
                            }
                            HelpTip { text: qsTr("En pausa no se registra. Al reanudar sigue desde el mes actual, sin recuperar los meses en pausa.") }
                        }
                    }
                }
                Label {
                    anchors.centerIn: parent
                    visible: list.count === 0
                    text: qsTr("No hay gastos fijos.")
                    color: Theme.textMuted
                }
            }
        }

        Label { text: dlg.editId > 0 ? qsTr("Editar gasto fijo") : qsTr("Agregar gasto fijo"); font.bold: true }
        RowLayout {
            Layout.fillWidth: true
            ComboBox {
                id: categoryCombo
                Layout.preferredWidth: 180
                model: expenseCtrl.categories
                textRole: "name"
                displayText: currentIndex >= 0 ? currentText : qsTr("Categoría…")
            }
            TextField {
                id: descriptionField
                Layout.fillWidth: true
                maximumLength: 120
                placeholderText: qsTr("Ej.: Alquiler del local")
                onAccepted: amountField.forceActiveFocus()
            }
            TextField {
                id: amountField
                Layout.preferredWidth: 120
                placeholderText: qsTr("Importe")
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onAccepted: dlg.save()
            }
            Label { text: qsTr("Día") }
            SpinBox {
                id: dayField
                from: 1
                to: 31
                value: 1
                editable: true
                Layout.preferredWidth: 110
                HelpTip { text: qsTr("Día del mes en que se registra (1 a 31).") }
            }
            PrimaryButton { text: dlg.editId > 0 ? qsTr("Guardar") : qsTr("Agregar"); onClicked: dlg.save() }
            Button { visible: dlg.editId > 0; text: qsTr("Cancelar"); onClicked: dlg.clearForm() }
        }

        Label {
            Layout.preferredWidth: 760
            visible: dlg.errorText !== ""
            text: dlg.errorText
            color: Theme.danger
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            Button { text: qsTr("Cerrar (Esc)"); onClicked: dlg.close() }
        }
    }
}
