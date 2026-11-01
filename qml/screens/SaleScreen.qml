import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

Item {
    id: root

    readonly property bool dialogOpen: paymentDialog.visible || quantityDialog.visible || discountDialog.visible
                                       || cancelDialog.visible || openCashDialog.visible || recentDialog.visible
    readonly property bool interactive: visible && !dialogOpen && saleCtrl.hasOpenSession
    property string lastSaleText: ""

    // El aviso de la ultima venta se borra al cambiar de sesion (cerrar sesion, restaurar
    // un respaldo) o al abrir/cerrar caja: si no, mostraria una venta que ya no aplica.
    readonly property bool sessionKey: app.loggedIn
    readonly property bool cashKey: saleCtrl.hasOpenSession
    onSessionKeyChanged: lastSaleText = ""
    onCashKeyChanged: lastSaleText = ""

    function notify(message, isError) {
        var w = ApplicationWindow.window
        if (w && w.showToast)
            w.showToast(message, isError)
    }

    function focusSearch() {
        searchField.forceActiveFocus()
    }

    function clearSearch() {
        searchDebounce.stop()
        searchField.text = ""
        saleCtrl.searchText = ""
    }

    function afterAdd(r) {
        if (r.ok) {
            lastSaleText = ""
            cartList.currentIndex = r.row
            cartList.positionViewAtIndex(r.row, ListView.Contain)
            clearSearch()
        } else {
            notify(r.error, true)
        }
        focusSearch()
    }

    function addFromSearch() {
        searchDebounce.stop()
        var text = searchField.text.trim()
        if (text === "")
            return
        var r = saleCtrl.addByCode(text)
        if (r.ok || !r.notFound) {
            afterAdd(r)
            return
        }
        // Sin coincidencia exacta: buscar por nombre.
        saleCtrl.searchText = text.indexOf("*") > 0 ? text.substring(text.indexOf("*") + 1) : text
        if (resultsList.count === 1) {
            afterAdd(saleCtrl.addProduct(saleCtrl.searchResults.idAt(0), quantityPrefix()))
        } else if (resultsList.count > 1) {
            resultsList.currentIndex = 0
            resultsList.forceActiveFocus()
        } else {
            notify(qsTr("No se encontró «%1».").arg(text), true)
        }
    }

    // "3*arroz" -> "3"
    function quantityPrefix() {
        var t = searchField.text
        var star = t.indexOf("*")
        return star > 0 ? t.substring(0, star).trim() : "1"
    }

    function addResult(row) {
        afterAdd(saleCtrl.addProduct(saleCtrl.searchResults.idAt(row), quantityPrefix()))
    }

    function editQty() {
        if (cartList.currentIndex < 0 || cartList.currentItem === null)
            return
        quantityDialog.openFor(cartList.currentIndex, cartList.currentItem.lineName, cartList.currentItem.lineUnit)
    }

    function removeCurrent() {
        if (cartList.currentIndex < 0)
            return
        var row = cartList.currentIndex
        saleCtrl.removeLine(row)
        cartList.currentIndex = Math.min(row, cartList.count - 1)
    }

    function pay() {
        if (saleCtrl.itemCount === 0) {
            notify(qsTr("Agrega productos antes de cobrar."), true)
            return
        }
        paymentDialog.openNew()
    }

    function cancelOrClear() {
        if (searchField.text !== "" || resultsList.activeFocus) {
            clearSearch()
            focusSearch()
        } else if (saleCtrl.itemCount > 0) {
            cancelDialog.open()
        }
    }

    onVisibleChanged: if (visible) focusSearch()

    Shortcut { sequence: "F2"; enabled: root.interactive; onActivated: root.focusSearch() }
    Shortcut { sequence: "F3"; enabled: root.interactive; onActivated: root.editQty() }
    Shortcut { sequence: "F4"; enabled: root.interactive; onActivated: root.pay() }
    Shortcut { sequence: "F7"; enabled: root.interactive && saleCtrl.itemCount > 0; onActivated: discountDialog.openNew() }
    Shortcut { sequence: "Esc"; enabled: root.interactive; onActivated: root.cancelOrClear() }
    Shortcut { sequence: "Ctrl+Delete"; enabled: root.interactive; onActivated: root.removeCurrent() }
    Shortcut { sequence: "Ctrl+Up"; enabled: root.interactive; onActivated: cartList.decrementCurrentIndex() }
    Shortcut { sequence: "Ctrl+Down"; enabled: root.interactive; onActivated: cartList.incrementCurrentIndex() }
    Shortcut { sequence: "F10"; enabled: root.interactive; onActivated: recentDialog.openNew() }
    Shortcut { sequence: "F8"; enabled: root.visible && !root.dialogOpen && !saleCtrl.hasOpenSession; onActivated: openCashDialog.openNew() }

    Timer {
        id: searchDebounce
        interval: 180
        onTriggered: {
            var t = searchField.text
            var star = t.indexOf("*")
            saleCtrl.searchText = star > 0 ? t.substring(star + 1) : t
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // ---------------- Carrito ----------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacing

            TextField {
                id: searchField
                Layout.fillWidth: true
                implicitHeight: 50
                font.pixelSize: Theme.fontSize + 4
                enabled: saleCtrl.hasOpenSession
                placeholderText: qsTr("Escanea o escribe código o nombre (F2) · 3*código para cantidad")
                selectByMouse: true
                // "+" o "-" escritos con el buscador vacio ajustan la cantidad de la linea seleccionada.
                // (Se detecta por el texto y no con Keys.onPressed para no depender del parametro
                // "event", desaconsejado en Qt 6 y sin sintaxis alternativa en Qt 5.14.)
                onTextChanged: {
                    if ((text === "+" || text === "-") && cartList.currentIndex >= 0) {
                        saleCtrl.changeQty(cartList.currentIndex, text === "+" ? 1 : -1)
                        text = ""
                    } else if (text !== "") {
                        searchDebounce.restart()
                    } else {
                        saleCtrl.searchText = ""
                    }
                }
                onAccepted: root.addFromSearch()
                Keys.onDeletePressed: {
                    if (text === "")
                        root.removeCurrent()
                    else if (selectedText !== "")
                        remove(selectionStart, selectionEnd)
                    else
                        remove(cursorPosition, cursorPosition + 1)
                }
                Keys.onDownPressed: {
                    if (resultsList.count > 0) {
                        resultsList.currentIndex = 0
                        resultsList.forceActiveFocus()
                    } else {
                        cartList.incrementCurrentIndex()
                    }
                }
                Keys.onUpPressed: cartList.decrementCurrentIndex()

                Popup {
                    id: resultsPopup
                    y: searchField.height + 2
                    width: searchField.width
                    height: Math.min(resultsList.contentHeight + 2, 360)
                    padding: 1
                    visible: searchField.text !== "" && resultsList.count > 0 && root.interactive
                    closePolicy: Popup.NoAutoClose
                    focus: false

                    background: Rectangle {
                        color: Theme.surface
                        border.color: Theme.accent
                        radius: Theme.radius
                    }

                    ListView {
                        id: resultsList
                        anchors.fill: parent
                        clip: true
                        model: saleCtrl.searchResults
                        highlightMoveDuration: 0
                        boundsBehavior: Flickable.StopAtBounds
                        Keys.onReturnPressed: root.addResult(currentIndex)
                        Keys.onEnterPressed: root.addResult(currentIndex)
                        Keys.onUpPressed: {
                            if (currentIndex <= 0) root.focusSearch()
                            else decrementCurrentIndex()
                        }

                        delegate: Rectangle {
                            width: resultsList.width
                            height: 42
                            color: ListView.isCurrentItem && resultsList.activeFocus ? Theme.selection
                                   : (mouse.containsMouse ? Theme.surfaceAlt : "transparent")
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                Label { text: model.code; color: Theme.textMuted; Layout.preferredWidth: 90; elide: Text.ElideRight }
                                Label { text: model.name; Layout.fillWidth: true; elide: Text.ElideRight }
                                Label {
                                    text: qsTr("stock %1").arg(model.stockText)
                                    color: model.lowStock ? Theme.danger : Theme.textMuted
                                    font.pixelSize: Theme.fontSmall
                                }
                                Label { text: model.priceText; font.bold: true; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight }
                            }
                            MouseArea {
                                id: mouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: root.addResult(index)
                            }
                        }
                    }
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
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            Label { width: 36; height: parent.height; verticalAlignment: Text.AlignVCenter; text: "#"; font.bold: true; color: Theme.textMuted }
                            Label { width: cartList.nameWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Producto"); font.bold: true; color: Theme.textMuted }
                            Label { width: 120; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Cantidad"); font.bold: true; color: Theme.textMuted }
                            Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Precio"); font.bold: true; color: Theme.textMuted }
                            Label { width: 130; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Importe"); font.bold: true; color: Theme.textMuted }
                        }
                    }

                    ListView {
                        id: cartList
                        readonly property int nameWidth: Math.max(140, width - 24 - 36 - 120 - 110 - 130)
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: saleCtrl.cart
                        highlightMoveDuration: 0
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { }
                        onCountChanged: if (count === 0) currentIndex = -1

                        delegate: Rectangle {
                            readonly property string lineName: model.name
                            readonly property string lineUnit: model.unit
                            width: cartList.width
                            height: 46
                            color: ListView.isCurrentItem ? Theme.selection : (index % 2 ? Theme.surfaceAlt : Theme.surface)
                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                Label { width: 36; height: parent.height; verticalAlignment: Text.AlignVCenter; text: index + 1; color: Theme.textMuted }
                                Column {
                                    width: cartList.nameWidth
                                    anchors.verticalCenter: parent.verticalCenter
                                    Label { width: parent.width; text: model.name; elide: Text.ElideRight; font.pixelSize: Theme.fontSize + 1 }
                                    Label {
                                        width: parent.width
                                        visible: model.stockShort
                                        text: qsTr("stock insuficiente")
                                        color: Theme.warning
                                        font.pixelSize: Theme.fontSmall - 1
                                    }
                                }
                                Label { width: 120; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: model.qtyText + " " + model.unit; font.pixelSize: Theme.fontSize + 1 }
                                Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: model.priceText; color: Theme.textMuted }
                                Label { width: 130; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: model.totalText; font.bold: true; font.pixelSize: Theme.fontSize + 1 }
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: { cartList.currentIndex = index; root.focusSearch() }
                                onDoubleClicked: { cartList.currentIndex = index; root.editQty() }
                            }
                        }

                        Column {
                            anchors.centerIn: parent
                            visible: cartList.count === 0
                            spacing: 8
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                visible: root.lastSaleText !== ""
                                text: root.lastSaleText
                                font.pixelSize: 26
                                font.bold: true
                                color: Theme.success
                            }
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: qsTr("Escanea un producto para empezar.")
                                color: Theme.textMuted
                            }
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("F3 cantidad · +/− ajustar · Ctrl+↑/↓ elegir línea · Supr quitar · F7 descuento · F4 cobrar · F10 reimprimir · Esc cancelar")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideRight
            }
        }

        // ---------------- Totales ----------------
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 340
            color: Theme.surface
            border.color: Theme.border
            radius: Theme.radius

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 10

                Label { text: qsTr("Venta"); font.pixelSize: Theme.fontLarge; font.bold: true }
                Label { text: qsTr("%n línea(s)", "", saleCtrl.itemCount); color: Theme.textMuted }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                RowLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("Subtotal"); Layout.fillWidth: true }
                    Label { text: saleCtrl.subtotalText }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: saleCtrl.discountLabel !== "" ? qsTr("Descuento (%1)").arg(saleCtrl.discountLabel) : qsTr("Descuento")
                        Layout.fillWidth: true
                    }
                    Label { text: saleCtrl.discountText === "0.00" ? "0.00" : "−" + saleCtrl.discountText; color: saleCtrl.discountText === "0.00" ? Theme.textMuted : Theme.success }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                Label { text: qsTr("TOTAL"); color: Theme.textMuted; font.bold: true }
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    text: saleCtrl.totalText
                    font.pixelSize: 44
                    font.bold: true
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: 24
                }
                Label {
                    Layout.alignment: Qt.AlignRight
                    text: saleCtrl.baseCurrency
                    color: Theme.textMuted
                }

                Item { Layout.fillHeight: true }

                PrimaryButton {
                    Layout.fillWidth: true
                    implicitHeight: 56
                    font.pixelSize: Theme.fontSize + 4
                    text: qsTr("Cobrar (F4)")
                    enabled: saleCtrl.itemCount > 0 && saleCtrl.hasOpenSession
                    focusPolicy: Qt.NoFocus
                    onClicked: root.pay()
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Cantidad (F3)")
                        enabled: cartList.currentIndex >= 0
                        focusPolicy: Qt.NoFocus
                        onClicked: root.editQty()
                    }
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Descuento (F7)")
                        enabled: saleCtrl.itemCount > 0
                        focusPolicy: Qt.NoFocus
                        onClicked: discountDialog.openNew()
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Quitar línea")
                        enabled: cartList.currentIndex >= 0
                        focusPolicy: Qt.NoFocus
                        onClicked: root.removeCurrent()
                    }
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Cancelar venta")
                        enabled: saleCtrl.itemCount > 0
                        focusPolicy: Qt.NoFocus
                        onClicked: cancelDialog.open()
                    }
                }
            }
        }
    }

    // Caja cerrada: bloquea la venta
    Rectangle {
        anchors.fill: parent
        visible: app.dbReady && !saleCtrl.hasOpenSession
        color: Qt.rgba(Theme.bg.r, Theme.bg.g, Theme.bg.b, 0.92)

        MouseArea { anchors.fill: parent }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 16
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("La caja está cerrada")
                font.pixelSize: Theme.fontLarge + 4
                font.bold: true
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Abre la caja con el fondo inicial para empezar a vender.")
                color: Theme.textMuted
            }
            PrimaryButton {
                Layout.alignment: Qt.AlignHCenter
                implicitHeight: 48
                text: qsTr("Abrir caja (F8)")
                onClicked: openCashDialog.openNew()
            }
        }
    }

    PaymentDialog {
        id: paymentDialog
        onCompleted: {
            root.lastSaleText = paymentDialog.lastHasChange
                    ? qsTr("Venta #%1 · Vuelto: %2").arg(paymentDialog.lastNumber).arg(paymentDialog.lastChangeText)
                    : qsTr("Venta #%1 registrada").arg(paymentDialog.lastNumber)
            if (paymentDialog.printRequested) {
                var pr = ticketCtrl.emitSale(paymentDialog.lastSaleId)
                if (!pr.ok) {
                    root.notify(qsTr("Venta guardada, pero no se generó el ticket: %1").arg(pr.error), true)
                    return
                }
            }
            root.notify(root.lastSaleText, false)
        }
        onClosed: root.focusSearch()
    }

    RecentSalesDialog {
        id: recentDialog
        onClosed: root.focusSearch()
    }

    QuantityDialog {
        id: quantityDialog
        onClosed: root.focusSearch()
    }

    DiscountDialog {
        id: discountDialog
        onClosed: root.focusSearch()
    }

    OpenCashDialog {
        id: openCashDialog
        onCashOpened: root.notify(qsTr("Caja abierta."), false)
        onClosed: root.focusSearch()
    }

    Dialog {
        id: cancelDialog
        title: qsTr("Cancelar venta")
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 400
        onOpened: cancelYes.forceActiveFocus()
        onClosed: root.focusSearch()

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.spacing
            Label { text: qsTr("¿Vaciar el carrito y cancelar esta venta?") }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button { text: qsTr("No (Esc)"); onClicked: cancelDialog.close() }
                PrimaryButton {
                    id: cancelYes
                    text: qsTr("Sí, cancelar (Enter)")
                    onClicked: { saleCtrl.clearCart(); cancelDialog.close() }
                    Keys.onReturnPressed: { saleCtrl.clearCart(); cancelDialog.close() }
                    Keys.onEnterPressed: { saleCtrl.clearCart(); cancelDialog.close() }
                }
            }
        }
    }
}
