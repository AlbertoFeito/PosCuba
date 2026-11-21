import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Gastos del negocio: por categoria y periodo, gastos fijos mensuales.
Item {
    id: root

    readonly property bool interactive: visible && !expenseDialog.visible && !recurringDialog.visible && !confirmDelete.visible
    readonly property var categoryModel: [{ "id": 0, "name": qsTr("Todas las categorías") }].concat(expenseCtrl.categories)

    function notify(message, isError) {
        var w = ApplicationWindow.window
        if (w && w.showToast)
            w.showToast(message, isError)
    }

    function syncFields() {
        fromField.text = expenseCtrl.fromText
        toField.text = expenseCtrl.toText
    }

    function applyRange() {
        var r = expenseCtrl.setRange(fromField.text, toField.text)
        if (!r.ok)
            notify(r.error, true)
    }

    function current() {
        return list.currentIndex >= 0 && list.currentIndex < expenseCtrl.expenses.length ? expenseCtrl.expenses[list.currentIndex] : null
    }

    onVisibleChanged: if (visible) { expenseCtrl.reload(); syncFields() }

    Shortcut { sequence: "Ctrl+N"; enabled: root.interactive; onActivated: expenseDialog.openNew() }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("Gastos"); font.pixelSize: Theme.fontLarge; font.bold: true; Layout.fillWidth: true }
            HelpButton { topic: "gastos" }
            Button {
                text: qsTr("Gastos fijos…")
                focusPolicy: Qt.NoFocus
                onClicked: recurringDialog.open()
                HelpTip { text: qsTr("Gastos que se repiten cada mes (alquiler, salarios, cuota). Se registran solos el día indicado.") }
            }
            Button {
                text: qsTr("Exportar CSV")
                focusPolicy: Qt.NoFocus
                onClicked: {
                    var r = expenseCtrl.exportCsv("es")
                    root.notify(r.ok ? qsTr("%1 gasto(s) guardados en %2").arg(r.count).arg(r.path) : r.error, !r.ok)
                }
                HelpTip { text: qsTr("Guarda los gastos del período en un CSV para Excel (Documentos\\PosCuba\\Reportes).") }
            }
            PrimaryButton {
                text: qsTr("Nuevo gasto (Ctrl+N)")
                focusPolicy: Qt.NoFocus
                onClicked: expenseDialog.openNew()
                HelpTip { text: qsTr("Registra un gasto: alquiler, electricidad, transporte… Puede salir de la caja o de otro dinero.") }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Repeater {
                model: [ { "key": "month", "label": qsTr("Este mes") }, { "key": "lastMonth", "label": qsTr("Mes anterior") },
                         { "key": "last30", "label": qsTr("30 días") }, { "key": "year", "label": qsTr("Este año") } ]
                delegate: Button {
                    text: modelData.label
                    focusPolicy: Qt.NoFocus
                    highlighted: expenseCtrl.preset === modelData.key
                    onClicked: { expenseCtrl.setPreset(modelData.key); root.syncFields() }
                }
            }
            Item { width: 12 }
            TextField { id: fromField; Layout.preferredWidth: 110; inputMask: "99/99/9999"; onAccepted: root.applyRange() }
            Label { text: "—" }
            TextField { id: toField; Layout.preferredWidth: 110; inputMask: "99/99/9999"; onAccepted: root.applyRange() }
            Button { text: qsTr("Aplicar"); focusPolicy: Qt.NoFocus; onClicked: root.applyRange() }
            Item { Layout.fillWidth: true }
            ComboBox {
                id: categoryFilter
                Layout.preferredWidth: 220
                model: root.categoryModel
                textRole: "name"
                onActivated: expenseCtrl.categoryFilter = root.categoryModel[currentIndex].id
                HelpTip { text: qsTr("Muestra solo los gastos de una categoría en la lista.") }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            // ---------- Por categoria ----------
            Rectangle {
                Layout.preferredWidth: 380
                Layout.fillHeight: true
                color: Theme.surface
                border.color: Theme.border
                radius: Theme.radius
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8
                    Label { text: qsTr("Total del período"); color: Theme.textMuted }
                    Label {
                        text: expenseCtrl.totalText + " " + saleCtrl.baseCurrency
                        font.pixelSize: 26
                        font.bold: true
                        HelpTip { text: qsTr("Todos los gastos del período. En Reportes se restan de la ganancia para dar la ganancia neta.") }
                    }
                    Label { text: qsTr("Por categoría"); font.bold: true; Layout.topMargin: 8 }
                    ListView {
                        id: totals
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 6
                        model: expenseCtrl.totals
                        boundsBehavior: Flickable.StopAtBounds
                        delegate: Column {
                            width: totals.width
                            spacing: 2
                            RowLayout {
                                width: parent.width
                                Label { text: modelData.name; Layout.fillWidth: true; elide: Text.ElideRight }
                                Label { text: modelData.amountText; font.bold: true }
                            }
                            Rectangle {
                                width: parent.width
                                height: 6
                                radius: 3
                                color: Theme.surfaceAlt
                                Rectangle {
                                    width: parent.width * modelData.share
                                    height: parent.height
                                    radius: 3
                                    color: Theme.accent
                                }
                            }
                        }
                        Label {
                            anchors.centerIn: parent
                            visible: totals.count === 0
                            text: qsTr("Sin gastos en el período.")
                            color: Theme.textMuted
                        }
                    }
                }
            }

            // ---------- Lista ----------
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.surface
                border.color: Theme.border
                radius: Theme.radius
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 0
                    Rectangle {
                        Layout.fillWidth: true
                        height: 34
                        color: Theme.surfaceAlt
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            Label { width: 100; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Fecha"); font.bold: true; color: Theme.textMuted }
                            Label { width: 150; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Categoría"); font.bold: true; color: Theme.textMuted }
                            Label { width: list.descWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Descripción"); font.bold: true; color: Theme.textMuted }
                            Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Importe"); font.bold: true; color: Theme.textMuted }
                            Label {
                                width: 90; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                text: qsTr("Pagado"); font.bold: true; color: Theme.textMuted
                                HelpTip { text: qsTr("«Caja»: salió de la gaveta (retiro en la caja). «Fuera»: se pagó con otro dinero. «Fijo»: lo registró un gasto fijo mensual.") }
                            }
                        }
                    }
                    ListView {
                        id: list
                        readonly property int descWidth: Math.max(120, width - 20 - 100 - 150 - 110 - 90)
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: expenseCtrl.expenses
                        highlightMoveDuration: 0
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { }
                        Keys.onDeletePressed: if (root.current()) confirmDelete.open()
                        delegate: Rectangle {
                            width: list.width
                            height: 36
                            color: ListView.isCurrentItem ? Theme.selection : (index % 2 ? Theme.surfaceAlt : Theme.surface)
                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                Label { width: 100; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.dateText }
                                Label { width: 150; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.categoryName; elide: Text.ElideRight; color: Theme.textMuted }
                                Label { width: list.descWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.description; elide: Text.ElideRight }
                                Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: modelData.amountText; font.bold: true }
                                Label {
                                    width: 90; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                    text: modelData.recurring ? qsTr("Fijo") : modelData.fromCash ? qsTr("Caja") : qsTr("Fuera")
                                    color: modelData.fromCash ? Theme.warning : Theme.textMuted
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: { list.currentIndex = index; list.forceActiveFocus() }
                            }
                        }
                        Label {
                            anchors.centerIn: parent
                            visible: list.count === 0
                            text: qsTr("No hay gastos en este período. Registra uno con «Nuevo gasto».")
                            color: Theme.textMuted
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: qsTr("Ctrl+N nuevo gasto · Supr borrar el elegido · Los gastos fijos se registran solos cada mes")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideRight
            }
            Button {
                text: qsTr("Borrar gasto")
                enabled: root.current() !== null
                focusPolicy: Qt.NoFocus
                onClicked: confirmDelete.open()
                HelpTip { text: qsTr("Borra el gasto elegido. Uno pagado de la caja solo se puede borrar mientras esa caja siga abierta (se borra también su retiro).") }
            }
        }
    }

    ExpenseDialog {
        id: expenseDialog
        onSaved: root.notify(qsTr("Gasto registrado."), false)
        onClosed: list.forceActiveFocus()
    }

    RecurringDialog {
        id: recurringDialog
        onClosed: list.forceActiveFocus()
    }

    Dialog {
        id: confirmDelete
        onAboutToShow: help.enter("gastos")
        onAboutToHide: help.leave("gastos")
        title: qsTr("Borrar gasto")
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 440
        onOpened: deleteYes.forceActiveFocus()
        onClosed: list.forceActiveFocus()
        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.spacing
            Label {
                Layout.preferredWidth: 400
                wrapMode: Text.Wrap
                text: root.current() ? qsTr("¿Borrar «%1» de %2 (%3)?").arg(root.current().description)
                                                   .arg(root.current().amountText).arg(root.current().dateText) : ""
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button { text: qsTr("No (Esc)"); onClicked: confirmDelete.close() }
                PrimaryButton {
                    id: deleteYes
                    text: qsTr("Sí, borrar (Enter)")
                    function go() {
                        var c = root.current()
                        confirmDelete.close()
                        if (!c)
                            return
                        var r = expenseCtrl.remove(c.id)
                        root.notify(r.ok ? qsTr("Gasto borrado.") : r.error, !r.ok)
                    }
                    onClicked: go()
                    Keys.onReturnPressed: go()
                    Keys.onEnterPressed: go()
                }
            }
        }
    }
}
