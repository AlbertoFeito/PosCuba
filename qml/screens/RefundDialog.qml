import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Devolucion parcial o anulacion de una venta.
Dialog {
    id: dlg
    onAboutToShow: help.enter("devoluciones")
    onAboutToHide: help.leave("devoluciones")

    property var sale: ({})
    property var qtys: ({})            // id de linea -> texto
    property var preview: ({})
    property var adminList: []
    property string errorText: ""
    // Resultado para quien abrio el dialogo
    property string doneText: ""
    signal done()

    function openFor(saleId) {
        sale = refundCtrl.load(saleId)
        qtys = {}
        allCheck.checked = false
        cashRadio.checked = true
        reasonField.text = ""
        restockCheck.checked = true
        adminPin.text = ""
        adminList = app.isAdmin ? [] : refundCtrl.admins()
        adminCombo.currentIndex = 0
        errorText = sale.ok ? "" : sale.error
        updatePreview()
        open()
    }

    function setQty(id, text) {
        var q = qtys
        q[String(id)] = text
        qtys = q
        updatePreview()
    }

    function updatePreview() {
        if (!sale.ok)
            return
        preview = refundCtrl.preview(sale.saleId, qtys, allCheck.checked)
    }

    function confirm() {
        if (!sale.ok)
            return
        var form = {
            "saleId": sale.saleId,
            "qtys": qtys,
            "all": allCheck.checked,
            "method": transferRadio.checked ? "transfer" : "cash",
            "reason": reasonField.text,
            "restock": restockCheck.checked,
            "adminId": adminList.length > 0 && adminCombo.currentIndex >= 0 ? adminList[adminCombo.currentIndex].id : 0,
            "adminPin": adminPin.text
        }
        var r = refundCtrl.refund(form)
        adminPin.text = ""
        if (!r.ok) {
            errorText = r.error
            return
        }
        doneText = r.voided
                ? qsTr("Venta #%1 anulada. Devolver %2 %3.").arg(sale.number).arg(r.totalText)
                      .arg(r.method === "transfer" ? qsTr("por transferencia") : qsTr("en efectivo"))
                : qsTr("Devolución #%1: devolver %2 %3.").arg(r.number).arg(r.totalText)
                      .arg(r.method === "transfer" ? qsTr("por transferencia") : qsTr("en efectivo"))
        close()
        done()
    }

    title: qsTr("Devolución o anulación")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(760, parent.width - 40)
    height: Math.min(640, parent.height - 40)
    closePolicy: Popup.CloseOnEscape
    onOpened: if (allCheck.enabled) firstFocus.forceActiveFocus()

    Shortcut { sequence: "Ctrl+S"; enabled: dlg.visible; onActivated: dlg.confirm() }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: dlg.sale.ok ? qsTr("Venta #%1 · %2 · Total %3").arg(dlg.sale.number).arg(dlg.sale.dateText).arg(dlg.sale.totalText) : ""
                font.bold: true
                elide: Text.ElideRight
            }
            HelpButton { topic: "devoluciones" }
        }
        Label {
            visible: dlg.sale.hasRefunds === true
            text: qsTr("Ya devuelto: %1 · Queda: %2").arg(dlg.sale.refundedText || "").arg(dlg.sale.remainingText || "")
            color: Theme.warning
        }
        Label {
            visible: dlg.sale.voided === true
            text: qsTr("Esta venta ya está anulada.")
            color: Theme.danger
            font.bold: true
        }

        CheckBox {
            id: allCheck
            text: qsTr("Devolver todo y anular la venta")
            enabled: dlg.sale.ok === true && dlg.sale.voided !== true
            onToggled: {
                dlg.qtys = {}
                for (var i = 0; i < lines.count; ++i) {
                    var row = lines.itemAtIndex(i)
                    if (row)
                        row.clearQty()
                }
                dlg.updatePreview()
            }
            HelpTip { text: qsTr("Devuelve todo lo que queda de la venta. La venta queda marcada como ANULADA y no cuenta en los reportes.") }
        }

        // ---- Lineas ----
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
                    height: 32
                    color: Theme.surfaceAlt
                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 8
                        Label { width: lines.nameWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Producto"); font.bold: true; color: Theme.textMuted }
                        Label { width: 90; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Vendido"); font.bold: true; color: Theme.textMuted }
                        Label { width: 90; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Ya devuelto"); font.bold: true; color: Theme.textMuted }
                        Label {
                            width: 130; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                            text: qsTr("A devolver"); font.bold: true; color: Theme.textMuted
                            HelpTip { text: qsTr("Cantidad que el cliente devuelve de cada producto. Vacío o 0 = no se devuelve. Acepta decimales en kg, lb, l y m.") }
                        }
                    }
                }

                ListView {
                    id: lines
                    readonly property int nameWidth: Math.max(140, width - 20 - 90 - 90 - 130 - 24)
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: dlg.sale.items || []
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { }

                    delegate: Rectangle {
                        function clearQty() { qtyField.text = "" }
                        width: lines.width
                        height: 44
                        color: index % 2 ? Theme.surfaceAlt : Theme.surface
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8
                            Label { width: lines.nameWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.name; elide: Text.ElideRight }
                            Label { width: 90; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: modelData.soldText + " " + modelData.unit }
                            Label {
                                width: 90; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                text: modelData.returnedText === "0" ? "—" : modelData.returnedText
                                color: Theme.textMuted
                            }
                            TextField {
                                id: qtyField
                                width: 130
                                anchors.verticalCenter: parent.verticalCenter
                                horizontalAlignment: Text.AlignRight
                                enabled: !allCheck.checked && modelData.available > 0 && dlg.sale.voided !== true
                                placeholderText: modelData.available <= 0 ? qsTr("nada")
                                                 : allCheck.checked ? qsTr("todo: %1").arg(modelData.availableText)
                                                                    : qsTr("máx. %1").arg(modelData.availableText)
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                onTextEdited: dlg.setQty(modelData.id, text)
                                Component.onCompleted: if (index === 0) firstFocus.target = qtyField
                            }
                        }
                    }
                }
            }
        }

        // ---- Forma, motivo, autorizacion ----
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Theme.spacing
            rowSpacing: 8

            Label { text: qsTr("Devolver") }
            RowLayout {
                RadioButton {
                    id: cashRadio
                    text: qsTr("En efectivo")
                    checked: true
                    HelpTip { text: qsTr("El dinero sale de la gaveta (moneda base). Se descuenta del efectivo esperado de la caja abierta.") }
                }
                RadioButton {
                    id: transferRadio
                    text: qsTr("Por transferencia")
                    HelpTip { text: qsTr("Se devuelve por Transfermóvil o EnZona; no toca el efectivo de la gaveta.") }
                }
            }

            Label { text: qsTr("Motivo *") }
            TextField {
                id: reasonField
                Layout.fillWidth: true
                maximumLength: 120
                placeholderText: qsTr("Ej.: producto defectuoso, error al cobrar, el cliente se arrepintió")
                onAccepted: dlg.confirm()
                HelpTip { text: qsTr("Obligatorio. Queda en el historial de inventario y en el ticket de la venta.") }
            }

            Item { width: 1; height: 1 }
            CheckBox {
                id: restockCheck
                text: qsTr("Reintegrar los productos al inventario")
                checked: true
                HelpTip { text: qsTr("Desmárcalo si lo devuelto no se puede volver a vender (roto, vencido). Entonces el stock no sube.") }
            }

            Label { text: qsTr("Autoriza"); visible: dlg.adminList.length > 0 || !app.isAdmin }
            RowLayout {
                visible: dlg.adminList.length > 0 || !app.isAdmin
                Layout.fillWidth: true
                ComboBox {
                    id: adminCombo
                    Layout.preferredWidth: 220
                    model: dlg.adminList
                    textRole: "name"
                    HelpTip { text: qsTr("Un cajero no puede devolver dinero solo: un administrador lo autoriza con su PIN.") }
                }
                TextField {
                    id: adminPin
                    Layout.preferredWidth: 140
                    echoMode: TextInput.Password
                    maximumLength: 8
                    inputMethodHints: Qt.ImhDigitsOnly
                    placeholderText: qsTr("PIN")
                    onAccepted: dlg.confirm()
                }
                Label {
                    visible: dlg.adminList.length === 0
                    text: qsTr("No hay administradores con PIN.")
                    color: Theme.danger
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: dlg.preview.ok ? (dlg.preview.completes ? qsTr("Se anulará la venta.") : "")
                                     : (dlg.preview.error || "")
                color: dlg.preview.ok ? Theme.warning : Theme.textMuted
                elide: Text.ElideRight
            }
            Label {
                text: qsTr("A devolver: %1").arg(dlg.preview.totalText || "0.00")
                font.pixelSize: 22
                font.bold: true
                color: dlg.preview.ok ? Theme.danger : Theme.textMuted
                HelpTip { text: qsTr("Importe para el cliente: la parte proporcional de cada producto, con su parte del descuento de la venta.") }
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
            PrimaryButton {
                text: allCheck.checked ? qsTr("Anular venta (Ctrl+S)") : qsTr("Confirmar devolución (Ctrl+S)")
                enabled: dlg.preview.ok === true && reasonField.text.trim() !== ""
                onClicked: dlg.confirm()
                HelpTip { text: qsTr("Registra la devolución: ajusta caja, inventario y reportes. No se puede deshacer.") }
            }
        }
    }

    // Foco inicial en la primera cantidad (se fija cuando la lista crea su primera fila).
    QtObject {
        id: firstFocus
        property var target: null
        function forceActiveFocus() {
            if (target)
                target.forceActiveFocus()
            else
                reasonField.forceActiveFocus()
        }
    }
}
