import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

Item {
    id: root

    readonly property bool interactive: visible && !openCashDialog.visible && !movementDialog.visible
                                        && !closeDialog.visible
    readonly property var s: cashCtrl.summary

    function notify(message, isError) {
        var w = ApplicationWindow.window
        if (w && w.showToast)
            w.showToast(message, isError)
    }

    onVisibleChanged: if (visible) cashCtrl.refresh()

    Shortcut { sequence: "F8"; enabled: root.interactive && !cashCtrl.isOpen; onActivated: openCashDialog.openNew() }
    Shortcut { sequence: "F9"; enabled: root.interactive && cashCtrl.isOpen; onActivated: closeDialog.openNew() }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Caja")
                font.pixelSize: Theme.fontLarge
                font.bold: true
                Layout.fillWidth: true
            }
            HelpButton { topic: "caja" }
            Button {
                visible: cashCtrl.isOpen
                text: qsTr("Ingreso")
                focusPolicy: Qt.NoFocus
                onClicked: movementDialog.openFor(false)
                HelpTip { text: qsTr("Registra dinero que entra a la gaveta sin ser una venta (por ejemplo, cambio adicional).") }
            }
            Button {
                visible: cashCtrl.isOpen
                text: qsTr("Retiro")
                focusPolicy: Qt.NoFocus
                onClicked: movementDialog.openFor(true)
                HelpTip { text: qsTr("Registra dinero que sale de la gaveta: pago a un proveedor, depósito, gasto. Pide el motivo.") }
            }
            PrimaryButton {
                visible: cashCtrl.isOpen
                text: qsTr("Cerrar caja (F9)")
                focusPolicy: Qt.NoFocus
                onClicked: closeDialog.openNew()
                HelpTip { text: qsTr("Arqueo: cuentas el efectivo de cada moneda y el programa calcula si sobra o falta. Hace un respaldo automático.") }
            }
            PrimaryButton {
                visible: !cashCtrl.isOpen
                text: qsTr("Abrir caja (F8)")
                focusPolicy: Qt.NoFocus
                onClicked: openCashDialog.openNew()
                HelpTip { text: qsTr("Empieza el turno con el efectivo inicial de la gaveta. Sin caja abierta no se puede vender.") }
            }
        }

        // ---- Caja abierta ----
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: openInfo.implicitHeight + 32
            color: Theme.surface
            border.color: Theme.border
            radius: Theme.radius

            ColumnLayout {
                id: openInfo
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Label {
                    visible: !cashCtrl.isOpen
                    text: qsTr("La caja está cerrada. Ábrela para empezar a vender.")
                    color: Theme.textMuted
                }

                Label {
                    visible: cashCtrl.isOpen
                    text: qsTr("Abierta el %1 por %2 · Fondo inicial %3").arg(root.s.openedAt || "")
                                                                         .arg(root.s.openedBy || "")
                                                                         .arg(root.s.openingText || "")
                    color: Theme.textMuted
                }

                GridLayout {
                    visible: cashCtrl.isOpen
                    Layout.fillWidth: true
                    columns: 4
                    columnSpacing: 12
                    rowSpacing: 12

                    Repeater {
                        model: [
                            { "label": qsTr("Ventas"), "value": String(root.s.salesCount || 0),
                              "tip": qsTr("Cantidad de ventas desde que se abrió la caja.") },
                            { "label": qsTr("Total vendido"), "value": root.s.salesTotalText || "",
                              "tip": qsTr("Suma de las ventas en la moneda base, con los descuentos ya restados.") },
                            { "label": qsTr("Ganancia estimada"), "value": root.s.profitText || "",
                              "tip": qsTr("Total vendido menos el costo de los productos vendidos. Sin costo cargado, toda la venta cuenta como ganancia.") },
                            { "label": qsTr("Transferencias"), "value": root.s.transferText || "",
                              "tip": qsTr("Cobrado por transferencia. No está en la gaveta.") },
                            { "label": qsTr("Efectivo por ventas"), "value": root.s.cashSalesText || "",
                              "tip": qsTr("Cobrado en efectivo (todas las monedas convertidas a la base), ya restado el vuelto.") },
                            { "label": qsTr("Ingresos manuales"), "value": root.s.movementsInText || "",
                              "tip": qsTr("Dinero agregado a la gaveta con el botón Ingreso.") },
                            { "label": qsTr("Retiros"), "value": root.s.movementsOutText || "",
                              "tip": qsTr("Dinero sacado de la gaveta con el botón Retiro.") }
                        ]
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 70
                            radius: Theme.radius
                            color: Theme.surfaceAlt
                            Column {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 4
                                Label { text: modelData.label; color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
                                Label { text: modelData.value; font.pixelSize: Theme.fontSize + 5; font.bold: true }
                            }
                            HelpTip { text: modelData.tip }
                        }
                    }
                }

                Label {
                    visible: cashCtrl.isOpen
                    text: qsTr("Efectivo esperado en la gaveta")
                    font.bold: true
                    HelpTip { text: qsTr("Lo que debería haber en la gaveta por moneda: fondo inicial + efectivo de ventas − vuelto + ingresos − retiros.") }
                }
                Flow {
                    visible: cashCtrl.isOpen
                    Layout.fillWidth: true
                    spacing: 12
                    Repeater {
                        model: root.s.expected || []
                        delegate: Rectangle {
                            width: 220
                            height: 64
                            radius: Theme.radius
                            color: Theme.selection
                            Column {
                                anchors.fill: parent
                                anchors.margins: 10
                                Label { text: modelData.currency; color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
                                Label { text: modelData.expectedText; font.pixelSize: Theme.fontSize + 7; font.bold: true }
                            }
                        }
                    }
                }
            }
        }

        // ---- Historial ----
        Label { text: qsTr("Cierres anteriores"); font.bold: true; font.pixelSize: Theme.fontSize + 1; HelpTip { text: qsTr("Historial de cierres. Diferencia en verde: cuadró; en rojo: faltó dinero; en amarillo: sobró.") } }

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
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        Label { width: 170; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Apertura"); font.bold: true; color: Theme.textMuted }
                        Label { width: 170; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Cierre"); font.bold: true; color: Theme.textMuted }
                        Label { width: historyList.userWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Usuario"); font.bold: true; color: Theme.textMuted }
                        Label { width: 130; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Esperado"); font.bold: true; color: Theme.textMuted }
                        Label { width: 130; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Contado"); font.bold: true; color: Theme.textMuted }
                        Label { width: 130; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Diferencia"); font.bold: true; color: Theme.textMuted }
                    }
                }

                ListView {
                    id: historyList
                    readonly property int userWidth: Math.max(100, width - 24 - 170 - 170 - 3 * 130)
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: cashCtrl.history
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { }

                    delegate: Rectangle {
                        width: historyList.width
                        height: Theme.rowHeight
                        color: index % 2 ? Theme.surfaceAlt : Theme.surface
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            Label { width: 170; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.openedAt }
                            Label { width: 170; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.closedAt }
                            Label { width: historyList.userWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.openedBy; elide: Text.ElideRight }
                            Label { width: 130; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: modelData.expectedText }
                            Label { width: 130; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: modelData.countedText }
                            Label {
                                width: 130; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                text: modelData.differenceText
                                font.bold: true
                                color: modelData.differenceCents === 0 ? Theme.success
                                       : (modelData.differenceCents < 0 ? Theme.danger : Theme.warning)
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: historyList.count === 0
                        text: qsTr("Todavía no hay cierres.")
                        color: Theme.textMuted
                    }
                }
            }
        }
    }

    OpenCashDialog {
        id: openCashDialog
        onCashOpened: root.notify(qsTr("Caja abierta."), false)
    }

    CashMovementDialog {
        id: movementDialog
        onDone: root.notify(movementDialog.withdrawal ? qsTr("Retiro registrado.") : qsTr("Ingreso registrado."), false)
    }

    CloseCashDialog {
        id: closeDialog
    }
}
