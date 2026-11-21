import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Cobro: efectivo (cualquier moneda activa), transferencia o mixto.
Dialog {
    id: dlg

    onAboutToShow: help.enter("cobro")
    onAboutToHide: help.leave("cobro")
    property string errorText: ""
    property var preview: ({})
    // Resultado de la ultima venta para la pantalla de venta
    property int lastNumber: 0
    property int lastSaleId: 0
    property bool printRequested: false
    property string lastChangeText: ""
    property bool lastHasChange: false
    signal completed()

    property var customers: []
    readonly property bool multiCurrency: saleCtrl.currencies.length > 1
    readonly property var selectedCustomer: creditCheck.checked && customerCombo.currentIndex >= 0
                                            && customerCombo.currentIndex < customers.length
                                            ? customers[customerCombo.currentIndex] : null
    readonly property var selectedCurrency: saleCtrl.currencies.length > 0
                                            ? saleCtrl.currencies[Math.max(0, currencyCombo.currentIndex)] : ({})

    function form(allowNegative) {
        return {
            "cash": cashField.text,
            "cashCurrency": selectedCurrency.code || "",
            "transfer": transferField.text,
            "reference": referenceField.text,
            "credit": creditCheck.checked,
            "customerId": dlg.selectedCustomer ? dlg.selectedCustomer.id : 0,
            "allowNegative": allowNegative === true
        }
    }

    function updatePreview() {
        preview = saleCtrl.previewPayment(form(false))
    }

    function openNew() {
        cashField.text = ""
        transferField.text = ""
        referenceField.text = ""
        currencyCombo.currentIndex = 0
        creditCheck.checked = false
        customers = customerCtrl.pick("")
        customerCombo.currentIndex = -1
        printTicket.checked = ticketCtrl.autoPrint
        errorText = ""
        updatePreview()
        open()
    }

    function setCash(cents) {
        currencyCombo.currentIndex = 0
        cashField.text = fmt.money(cents)
        cashField.forceActiveFocus()
        cashField.selectAll()
    }

    function quickAmounts() {
        var t = saleCtrl.totalCents
        var list = [t]
        var steps = [10000, 50000, 100000]   // 100, 500, 1000
        for (var i = 0; i < steps.length; ++i) {
            var v = Math.ceil(t / steps[i]) * steps[i]
            if (v > t && list.indexOf(v) < 0)
                list.push(v)
        }
        return list
    }

    function tryCheckout(allowNegative) {
        var r = saleCtrl.checkout(form(allowNegative))
        if (r.ok) {
            lastNumber = r.number
            lastSaleId = r.saleId
            printRequested = printTicket.checked
            lastChangeText = r.changeText
            lastHasChange = r.hasChange
            close()
            completed()
        } else if (r.stockIssues && r.stockIssues.length > 0) {
            stockConfirm.issues = r.stockIssues
            stockConfirm.open()
        } else {
            errorText = r.error
        }
    }

    function reloadCustomers(selectId) {
        customers = customerCtrl.pick("")
        for (var i = 0; i < customers.length; ++i) {
            if (customers[i].id === selectId)
                customerCombo.currentIndex = i
        }
        updatePreview()
    }

    function confirm() {
        updatePreview()
        if (creditCheck.checked && !dlg.selectedCustomer) {
            errorText = qsTr("Elige el cliente que se lleva la mercancía a crédito.")
            customerCombo.forceActiveFocus()
            return
        }
        if (!preview.valid) {
            errorText = preview.error
            return
        }
        tryCheckout(false)
    }

    title: qsTr("Cobrar")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(620, parent.width - 40)
    closePolicy: Popup.CloseOnEscape
    onOpened: cashField.forceActiveFocus()

    // F6: siguiente moneda para el efectivo, sin salir del campo de importe.
    Shortcut {
        sequence: "F6"
        enabled: dlg.visible && dlg.multiCurrency && !stockConfirm.visible
        onActivated: {
            currencyCombo.currentIndex = (currencyCombo.currentIndex + 1) % currencyCombo.count
            cashField.forceActiveFocus()
            cashField.selectAll()
        }
    }

    // F9: dejar lo que falta a credito (fiado)
    Shortcut {
        sequence: "F9"
        enabled: dlg.visible && !stockConfirm.visible && !customerDialog.visible
        onActivated: {
            creditCheck.checked = !creditCheck.checked
            dlg.updatePreview()
            if (creditCheck.checked)
                customerCombo.forceActiveFocus()
            else
                cashField.forceActiveFocus()
        }
    }

    Shortcut {
        sequence: "Ctrl+S"
        enabled: dlg.visible && !stockConfirm.visible && !customerDialog.visible
        onActivated: dlg.confirm()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("Total a cobrar"); color: Theme.textMuted; Layout.fillWidth: true }
            HelpButton { topic: "cobro" }
            Label {
                text: saleCtrl.totalText + " " + saleCtrl.baseCurrency
                font.pixelSize: 32
                font.bold: true
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Theme.spacing
            rowSpacing: 10

            Label { text: qsTr("Efectivo"); font.bold: true }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: cashField
                    Layout.fillWidth: true
                    font.pixelSize: Theme.fontSize + 4
                    placeholderText: "0.00"
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    onTextChanged: { dlg.errorText = ""; dlg.updatePreview() }
                    onAccepted: dlg.confirm()
                    HelpTip { text: qsTr("Lo que entrega el cliente en efectivo, en la moneda elegida a la derecha. Enter confirma la venta.") }
                }
                ComboBox {
                    id: currencyCombo
                    visible: dlg.multiCurrency
                    Layout.preferredWidth: 100
                    model: saleCtrl.currencies
                    textRole: "code"
                    onCurrentIndexChanged: dlg.updatePreview()
                    HelpTip { text: qsTr("Moneda del efectivo recibido. F6 pasa a la siguiente. El vuelto siempre se da en la moneda base.") }
                }
            }

            Item { width: 1; height: 1 }
            Flow {
                Layout.fillWidth: true
                spacing: 6
                Repeater {
                    model: dlg.visible ? dlg.quickAmounts() : []
                    delegate: Button {
                        text: index === 0 ? qsTr("Exacto %1").arg(fmt.money(modelData)) : fmt.money(modelData)
                        focusPolicy: Qt.NoFocus
                        onClicked: dlg.setCash(modelData)
                        HelpTip { text: qsTr("Pone este importe en el campo de efectivo (moneda base).") }
                    }
                }
            }

            Label {
                visible: dlg.multiCurrency
                text: dlg.selectedCurrency.isBase ? "" : qsTr("Tasa")
                color: Theme.textMuted
            }
            Label {
                visible: dlg.multiCurrency
                text: dlg.selectedCurrency.isBase
                      ? qsTr("F6: pagar en otra moneda")
                      : qsTr("1 %1 = %2 %3 · F6: cambiar moneda").arg(dlg.selectedCurrency.code || "")
                                                               .arg(dlg.selectedCurrency.rateText || "")
                                                               .arg(saleCtrl.baseCurrency)
                color: Theme.textMuted
            }

            Label { text: qsTr("Transferencia"); font.bold: true }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: transferField
                    Layout.preferredWidth: 180
                    font.pixelSize: Theme.fontSize + 4
                    placeholderText: "0.00"
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    onTextChanged: { dlg.errorText = ""; dlg.updatePreview() }
                    onAccepted: dlg.confirm()
                    HelpTip { text: qsTr("Importe pagado por transferencia (Transfermóvil, EnZona…). No puede superar el total.") }
                }
                TextField {
                    id: referenceField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Nº de transacción (opcional)")
                    maximumLength: 60
                    onAccepted: dlg.confirm()
                    HelpTip { text: qsTr("Número de la transacción, para identificarla luego. Sale en el ticket. Opcional.") }
                }
            }

            Label { text: qsTr("A crédito"); font.bold: true }
            RowLayout {
                Layout.fillWidth: true
                CheckBox {
                    id: creditCheck
                    text: qsTr("Lo que falta (F9)")
                    focusPolicy: Qt.NoFocus
                    onToggled: {
                        dlg.errorText = ""
                        dlg.updatePreview()
                        if (checked)
                            customerCombo.forceActiveFocus()
                    }
                    HelpTip { text: qsTr("Fiado: lo que no se paga ahora queda como deuda del cliente elegido. Se puede combinar con efectivo o transferencia.") }
                }
                ComboBox {
                    id: customerCombo
                    Layout.fillWidth: true
                    enabled: creditCheck.checked
                    model: dlg.customers
                    textRole: "name"
                    displayText: currentIndex >= 0 ? currentText : qsTr("Elegir cliente…")
                    onActivated: { dlg.errorText = ""; dlg.updatePreview() }
                    // Enter con la lista cerrada confirma la venta, igual que en los importes.
                    Keys.onReturnPressed: { if (popup.visible) event.accepted = false; else dlg.confirm() }
                    Keys.onEnterPressed: { if (popup.visible) event.accepted = false; else dlg.confirm() }
                    HelpTip { text: qsTr("Cliente que queda debiendo. Escribe las primeras letras para buscarlo en la lista.") }
                }
                Button {
                    text: "+"
                    enabled: creditCheck.checked
                    focusPolicy: Qt.NoFocus
                    implicitWidth: 36
                    onClicked: customerDialog.openNew()
                    HelpTip { text: qsTr("Crear un cliente nuevo sin salir del cobro.") }
                }
            }

            Item { width: 1; height: 1; visible: dlg.selectedCustomer !== null }
            Label {
                visible: dlg.selectedCustomer !== null
                text: dlg.selectedCustomer
                      ? (dlg.selectedCustomer.hasLimit ? qsTr("Debe ahora %1 · límite %2").arg(dlg.selectedCustomer.balanceText).arg(dlg.selectedCustomer.limitText)
                                                         : qsTr("Debe ahora %1 · sin límite").arg(dlg.selectedCustomer.balanceText))
                      : ""
                color: dlg.selectedCustomer && dlg.selectedCustomer.overLimit ? Theme.danger : Theme.textMuted
                font.pixelSize: Theme.fontSmall
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("Pagado: %1").arg(dlg.preview.paidText || "0.00"); color: Theme.textMuted; Layout.fillWidth: true }
            Label {
                visible: dlg.preview.hasCredit === true
                text: qsTr("A crédito: %1").arg(dlg.preview.creditText || "")
                color: Theme.warning
                font.pixelSize: 24
                font.bold: true
                HelpTip { text: qsTr("Queda como deuda del cliente. Lo verá en Clientes y lo podrá pagar con abonos.") }
            }
            Label {
                visible: dlg.preview.hasRemaining === true
                text: qsTr("Falta: %1").arg(dlg.preview.remainingText || "")
                color: Theme.danger
                font.pixelSize: 24
                font.bold: true
            }
            Label {
                visible: dlg.preview.hasRemaining !== true && dlg.preview.hasCredit !== true
                text: qsTr("Vuelto: %1").arg(dlg.preview.changeText || "0.00")
                color: Theme.success
                font.pixelSize: 24
                font.bold: true
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
            Layout.fillWidth: true
            CheckBox {
                id: printTicket
                text: ticketCtrl.usesPdf ? qsTr("Agregar ticket al PDF del día") : qsTr("Imprimir ticket")
                focusPolicy: Qt.NoFocus
                Layout.fillWidth: true
                HelpTip { text: qsTr("Genera el ticket de esta venta. El valor inicial se cambia en Configuración → Al cobrar.") }
            }
            Button { text: qsTr("Volver (Esc)"); onClicked: dlg.close(); HelpTip { text: qsTr("Regresa al carrito sin cobrar.") } }
            PrimaryButton {
                text: qsTr("Confirmar venta (Enter)")
                enabled: dlg.preview.valid === true && (!creditCheck.checked || dlg.selectedCustomer !== null)
                onClicked: dlg.confirm()
                HelpTip { text: qsTr("Registra la venta, los pagos y descuenta el stock. Se activa cuando no falta dinero.") }
            }
        }
    }

    CustomerDialog {
        id: customerDialog
        onSaved: dlg.reloadCustomers(customerDialog.savedId)
        onClosed: customerCombo.forceActiveFocus()
    }

    Dialog {
        id: stockConfirm
        onAboutToShow: help.enter("cobro")
        onAboutToHide: help.leave("cobro")
        property var issues: []

        title: qsTr("Stock insuficiente")
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 480
        onOpened: sellAnyway.forceActiveFocus()
        onClosed: cashField.forceActiveFocus()

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.spacing
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Estos productos quedarán con stock negativo:")
            }
            Repeater {
                model: stockConfirm.issues
                delegate: Label {
                    Layout.fillWidth: true
                    text: "• " + modelData
                    color: Theme.danger
                    wrapMode: Text.Wrap
                }
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("¿Vender igual? Corrige el stock luego en Inventario.")
                color: Theme.textMuted
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button { text: qsTr("No vender (Esc)"); onClicked: stockConfirm.close() }
                PrimaryButton {
                    id: sellAnyway
                    text: qsTr("Vender igual (Enter)")
                    onClicked: { stockConfirm.close(); dlg.tryCheckout(true) }
                    Keys.onReturnPressed: { stockConfirm.close(); dlg.tryCheckout(true) }
                    Keys.onEnterPressed: { stockConfirm.close(); dlg.tryCheckout(true) }
                    HelpTip { text: qsTr("Registra la venta y el stock queda negativo. Corrígelo después en Inventario.") }
                }
            }
        }
    }
}
