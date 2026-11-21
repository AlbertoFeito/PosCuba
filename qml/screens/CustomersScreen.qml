import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Clientes y fiado: quien debe, cuanto, su historial y los abonos.
Item {
    id: root

    property var entries: []
    readonly property bool interactive: visible && !customerDialog.visible && !paymentDialog.visible

    function notify(message, isError) {
        var w = ApplicationWindow.window
        if (w && w.showToast)
            w.showToast(message, isError)
    }

    function current() {
        return list.currentIndex >= 0 && list.currentIndex < customerCtrl.customers.length
               ? customerCtrl.customers[list.currentIndex] : null
    }

    function loadEntries() {
        var c = current()
        entries = c ? customerCtrl.entries(c.id) : []
    }

    function selectId(id) {
        for (var i = 0; i < customerCtrl.customers.length; ++i) {
            if (customerCtrl.customers[i].id === id) {
                list.currentIndex = i
                break
            }
        }
        loadEntries()
    }

    function payCurrent() {
        var c = current()
        if (c)
            paymentDialog.openFor(c.id)
    }

    function editCurrent() {
        var c = current()
        if (c)
            customerDialog.openEdit(c.id)
    }

    function toggleActive() {
        var c = current()
        if (!c)
            return
        var r = customerCtrl.setActive(c.id, !c.active)
        notify(r.ok ? (c.active ? qsTr("Cliente desactivado.") : qsTr("Cliente reactivado.")) : r.error, !r.ok)
        loadEntries()
    }

    onVisibleChanged: if (visible) { customerCtrl.reload(); loadEntries(); searchField.forceActiveFocus() }

    Shortcut { sequence: "Ctrl+N"; enabled: root.interactive; onActivated: customerDialog.openNew() }
    Shortcut { sequence: "F2"; enabled: root.interactive; onActivated: { searchField.forceActiveFocus(); searchField.selectAll() } }
    Shortcut { sequence: "F5"; enabled: root.interactive && root.current() !== null; onActivated: root.payCurrent() }
    Shortcut { sequence: "F3"; enabled: root.interactive && root.current() !== null; onActivated: root.editCurrent() }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: Theme.spacing

        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("Clientes"); font.pixelSize: Theme.fontLarge; font.bold: true }
            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                text: customerCtrl.debtorsCount > 0
                      ? qsTr("Por cobrar: %1 %2 · %3 cliente(s) con deuda").arg(customerCtrl.receivablesText)
                            .arg(saleCtrl.baseCurrency).arg(customerCtrl.debtorsCount)
                      : qsTr("Nadie debe nada.")
                color: customerCtrl.debtorsCount > 0 ? Theme.warning : Theme.textMuted
                font.bold: true
                elide: Text.ElideRight
                HelpTip { text: qsTr("Suma de lo que deben todos los clientes (cuentas por cobrar).") }
            }
            HelpButton { topic: "clientes" }
            Button {
                visible: app.isAdmin
                text: qsTr("Exportar deudas")
                focusPolicy: Qt.NoFocus
                onClicked: {
                    var r = customerCtrl.exportReceivables("es")
                    root.notify(r.ok ? qsTr("%1 cliente(s) con deuda guardados en %2").arg(r.count).arg(r.path) : r.error, !r.ok)
                }
                HelpTip { text: qsTr("Guarda en CSV (Excel) la lista de clientes que deben, con su teléfono y su deuda. Se guarda en Documentos\\PosCuba\\Reportes.") }
            }
            PrimaryButton {
                text: qsTr("Nuevo cliente (Ctrl+N)")
                focusPolicy: Qt.NoFocus
                onClicked: customerDialog.openNew()
                HelpTip { text: qsTr("Crea un cliente para venderle a crédito. También se puede crear desde el cobro.") }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            // ---------- Lista ----------
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 560
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Buscar por nombre o teléfono (F2)")
                        selectByMouse: true
                        onTextChanged: { customerCtrl.search = text; list.currentIndex = customerCtrl.customers.length > 0 ? 0 : -1; root.loadEntries() }
                        Keys.onDownPressed: list.forceActiveFocus()
                        Keys.onEscapePressed: text = ""
                        HelpTip { text: qsTr("Filtra mientras escribes. ↓ baja a la lista.") }
                    }
                    CheckBox {
                        text: qsTr("Solo con deuda")
                        checked: customerCtrl.onlyDebt
                        focusPolicy: Qt.NoFocus
                        onToggled: { customerCtrl.onlyDebt = checked; list.currentIndex = customerCtrl.customers.length > 0 ? 0 : -1; root.loadEntries() }
                        HelpTip { text: qsTr("Muestra solo los clientes que deben algo: para cobrar.") }
                    }
                    CheckBox {
                        text: qsTr("Inactivos")
                        checked: customerCtrl.includeInactive
                        focusPolicy: Qt.NoFocus
                        onToggled: customerCtrl.includeInactive = checked
                        HelpTip { text: qsTr("Incluye los clientes desactivados.") }
                    }
                }

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
                                Label { width: list.nameWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Cliente"); font.bold: true; color: Theme.textMuted }
                                Label { width: 120; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Teléfono"); font.bold: true; color: Theme.textMuted }
                                Label {
                                    width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                    text: qsTr("Debe"); font.bold: true; color: Theme.textMuted
                                    HelpTip { text: qsTr("Lo que debe hoy. En rojo si pasa de su límite; en verde si tiene saldo a favor.") }
                                }
                                Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Límite"); font.bold: true; color: Theme.textMuted }
                            }
                        }
                        ListView {
                            id: list
                            readonly property int nameWidth: Math.max(140, width - 20 - 120 - 220)
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: customerCtrl.customers
                            highlightMoveDuration: 0
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar { }
                            onCurrentIndexChanged: root.loadEntries()
                            Keys.onReturnPressed: root.editCurrent()
                            Keys.onEnterPressed: root.editCurrent()
                            Keys.onDeletePressed: if (app.isAdmin) root.toggleActive()
                            Keys.onEscapePressed: searchField.forceActiveFocus()

                            delegate: Rectangle {
                                width: list.width
                                height: 38
                                color: ListView.isCurrentItem ? Theme.selection : (index % 2 ? Theme.surfaceAlt : Theme.surface)
                                opacity: modelData.active ? 1.0 : 0.5
                                Row {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    Label { width: list.nameWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.name; elide: Text.ElideRight; font.bold: modelData.balance > 0 }
                                    Label { width: 120; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.phone; elide: Text.ElideRight; color: Theme.textMuted }
                                    Label {
                                        width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                        text: modelData.balance === 0 ? "—" : modelData.balanceText
                                        font.bold: true
                                        color: modelData.overLimit ? Theme.danger : modelData.balance < 0 ? Theme.success
                                               : modelData.balance > 0 ? Theme.warning : Theme.textMuted
                                    }
                                    Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: modelData.limitText; color: Theme.textMuted }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: { list.currentIndex = index; list.forceActiveFocus() }
                                    onDoubleClicked: { list.currentIndex = index; root.editCurrent() }
                                }
                            }

                            Label {
                                anchors.centerIn: parent
                                width: parent.width - 40
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.Wrap
                                visible: list.count === 0
                                text: searchField.text !== "" || customerCtrl.onlyDebt ? qsTr("Ningún cliente coincide.")
                                      : qsTr("Todavía no hay clientes. Crea uno con «Nuevo cliente» o al cobrar a crédito (F9).")
                                color: Theme.textMuted
                            }
                        }
                    }
                }
            }

            // ---------- Detalle ----------
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 460
                Layout.fillWidth: true
                color: Theme.surface
                border.color: Theme.border
                radius: Theme.radius
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8
                    visible: root.current() !== null

                    Label {
                        Layout.fillWidth: true
                        text: root.current() ? root.current().name : ""
                        font.pixelSize: Theme.fontLarge
                        font.bold: true
                        elide: Text.ElideRight
                    }
                    Label {
                        Layout.fillWidth: true
                        text: root.current() ? [root.current().phone, root.current().note].filter(function(t) { return t !== "" }).join(" · ") : ""
                        visible: text !== ""
                        color: Theme.textMuted
                        elide: Text.ElideRight
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: !root.current() ? "" : root.current().balance > 0 ? qsTr("Debe %1").arg(root.current().balanceText)
                                  : root.current().balance < 0 ? qsTr("Saldo a favor %1").arg(fmt.money(-root.current().balance))
                                  : qsTr("No debe nada")
                            font.pixelSize: 24
                            font.bold: true
                            color: !root.current() ? Theme.text : root.current().overLimit ? Theme.danger
                                   : root.current().balance > 0 ? Theme.warning : Theme.success
                            Layout.fillWidth: true
                        }
                        Label {
                            text: root.current() ? qsTr("Límite: %1").arg(root.current().limitText) : ""
                            color: Theme.textMuted
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        PrimaryButton {
                            text: qsTr("Abono (F5)")
                            enabled: root.current() !== null && root.current().balance > 0
                            focusPolicy: Qt.NoFocus
                            onClicked: root.payCurrent()
                            HelpTip { text: qsTr("El cliente paga parte o toda su deuda. El dinero entra a la caja abierta.") }
                        }
                        Button {
                            text: qsTr("Editar (F3)")
                            focusPolicy: Qt.NoFocus
                            onClicked: root.editCurrent()
                            HelpTip { text: qsTr("Cambia nombre, teléfono, nota y (administrador) el límite de crédito.") }
                        }
                        Button {
                            visible: app.isAdmin
                            text: root.current() && !root.current().active ? qsTr("Reactivar") : qsTr("Desactivar")
                            focusPolicy: Qt.NoFocus
                            onClicked: root.toggleActive()
                            HelpTip { text: qsTr("Un cliente desactivado no aparece al cobrar. No se puede desactivar mientras deba dinero.") }
                        }
                    }

                    Label { text: qsTr("Cuenta"); font.bold: true; Layout.topMargin: 6 }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Theme.bg
                        border.color: Theme.border
                        radius: Theme.radius
                        clip: true
                        ListView {
                            id: ledger
                            anchors.fill: parent
                            anchors.margins: 1
                            clip: true
                            model: root.entries
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar { }
                            delegate: Rectangle {
                                width: ledger.width
                                height: 46
                                color: index % 2 ? Theme.surfaceAlt : Theme.surface
                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    anchors.topMargin: 4
                                    anchors.bottomMargin: 4
                                    spacing: 0
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label { text: modelData.kindText; font.bold: true; Layout.fillWidth: true; elide: Text.ElideRight }
                                        Label {
                                            text: modelData.amountText
                                            font.bold: true
                                            color: modelData.kind === "charge" ? Theme.warning : Theme.success
                                        }
                                        Label { text: qsTr("saldo %1").arg(modelData.balanceText); color: Theme.textMuted; Layout.preferredWidth: 120; horizontalAlignment: Text.AlignRight }
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: modelData.dateText + " · " + modelData.detail
                                        color: Theme.textMuted
                                        font.pixelSize: Theme.fontSmall
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                            Label {
                                anchors.centerIn: parent
                                visible: ledger.count === 0
                                text: qsTr("Sin movimientos.")
                                color: Theme.textMuted
                            }
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: root.current() === null
                    text: qsTr("Elige un cliente para ver su cuenta.")
                    color: Theme.textMuted
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("F2 buscar · ↑↓ elegir · F5 abono · Enter/F3 editar · Ctrl+N nuevo · Al cobrar, F9 deja lo que falta a crédito")
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
            elide: Text.ElideRight
        }
    }

    CustomerDialog {
        id: customerDialog
        onSaved: { root.selectId(customerDialog.savedId); root.notify(qsTr("Cliente guardado."), false) }
        onClosed: list.forceActiveFocus()
    }

    CustomerPaymentDialog {
        id: paymentDialog
        onPaid: { root.selectId(paymentDialog.customer.id); root.notify(paymentDialog.resultText, false) }
        onClosed: list.forceActiveFocus()
    }
}
