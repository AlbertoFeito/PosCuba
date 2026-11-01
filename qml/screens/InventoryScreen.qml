import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

Item {
    id: root

    readonly property bool interactive: visible && !purchaseDialog.visible && !adjustDialog.visible
    readonly property bool hasSelection: inventoryCtrl.selectedProduct.id !== undefined

    function notify(message, isError) {
        var w = ApplicationWindow.window
        if (w && w.showToast)
            w.showToast(message, isError)
    }

    function focusSearch() {
        searchField.forceActiveFocus()
        searchField.selectAll()
    }

    function syncSelection() {
        inventoryCtrl.selectedProductId = inventoryCtrl.products.idAt(list.currentIndex)
    }

    // Tras recargar el modelo la lista pierde la fila actual: se restaura por id.
    function restoreSelection(productId) {
        var row = inventoryCtrl.products.rowOfId(productId)
        list.currentIndex = row
        if (row >= 0)
            list.positionViewAtIndex(row, ListView.Contain)
        else
            root.syncSelection()
    }

    function openPurchase() {
        if (hasSelection)
            purchaseDialog.openFor(inventoryCtrl.selectedProduct)
    }

    function openAdjust() {
        if (hasSelection)
            adjustDialog.openFor(inventoryCtrl.selectedProduct)
    }

    onVisibleChanged: {
        if (visible) {
            var keep = inventoryCtrl.selectedProductId
            inventoryCtrl.reload()
            restoreSelection(keep)
            focusSearch()
        }
    }

    Shortcut { sequence: "F2"; enabled: root.interactive; onActivated: root.focusSearch() }
    Shortcut { sequence: "F5"; enabled: root.interactive; onActivated: root.openPurchase() }
    Shortcut { sequence: "F6"; enabled: root.interactive; onActivated: root.openAdjust() }

    Timer {
        id: searchDebounce
        interval: 200
        onTriggered: {
            inventoryCtrl.searchText = searchField.text
            list.currentIndex = list.count > 0 ? 0 : -1
            root.syncSelection()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // ---------- Lista de productos ----------
        ColumnLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: 5
            spacing: Theme.spacing

            Label {
                text: qsTr("Inventario")
                font.pixelSize: Theme.fontLarge
                font.bold: true
            }

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Buscar producto o escanear código (F2)")
                selectByMouse: true
                onTextChanged: searchDebounce.restart()
                Keys.onDownPressed: list.forceActiveFocus()
                onAccepted: {
                    searchDebounce.stop()
                    inventoryCtrl.searchText = searchField.text
                    list.currentIndex = list.count > 0 ? 0 : -1
                    root.syncSelection()
                    if (list.count > 0)
                        list.forceActiveFocus()
                }
                Keys.onEscapePressed: searchField.text = ""
            }

            CheckBox {
                text: inventoryCtrl.lowStockCount > 0 ? qsTr("Solo stock bajo (%1)").arg(inventoryCtrl.lowStockCount)
                                                      : qsTr("Solo stock bajo")
                focusPolicy: Qt.NoFocus
                checked: inventoryCtrl.lowStockOnly
                onToggled: {
                    inventoryCtrl.lowStockOnly = checked
                    list.currentIndex = list.count > 0 ? 0 : -1
                    root.syncSelection()
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
                            Label { width: 80; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Código"); font.bold: true; color: Theme.textMuted }
                            Label { width: list.nameWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Nombre"); font.bold: true; color: Theme.textMuted }
                            Label { width: 100; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Stock"); font.bold: true; color: Theme.textMuted }
                            Label { width: 80; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Mínimo"); font.bold: true; color: Theme.textMuted }
                        }
                    }

                    ListView {
                        id: list
                        readonly property int nameWidth: Math.max(120, width - 24 - 80 - 100 - 80)
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: inventoryCtrl.products
                        currentIndex: -1
                        highlightMoveDuration: 0
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { }

                        onCurrentIndexChanged: root.syncSelection()
                        Keys.onReturnPressed: root.openPurchase()
                        Keys.onEnterPressed: root.openPurchase()
                        Keys.onEscapePressed: root.focusSearch()
                        Keys.onUpPressed: {
                            if (currentIndex <= 0)
                                root.focusSearch()
                            else
                                decrementCurrentIndex()
                        }

                        delegate: Rectangle {
                            width: list.width
                            height: Theme.rowHeight
                            color: ListView.isCurrentItem ? Theme.selection
                                                          : (index % 2 ? Theme.surfaceAlt : Theme.surface)
                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                Label { width: 80; height: parent.height; verticalAlignment: Text.AlignVCenter; text: model.code; elide: Text.ElideRight }
                                Label { width: list.nameWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: model.name; elide: Text.ElideRight }
                                Label {
                                    width: 100; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                    text: model.stockText + " " + model.unit
                                    color: model.lowStock ? Theme.danger : Theme.text
                                    font.bold: true
                                }
                                Label {
                                    width: 80; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                    text: model.minStockText === "0" ? "—" : model.minStockText
                                    color: Theme.textMuted
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    list.currentIndex = index
                                    list.forceActiveFocus()
                                }
                                onDoubleClicked: {
                                    list.currentIndex = index
                                    root.openPurchase()
                                }
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: list.count === 0
                            text: inventoryCtrl.lowStockOnly ? qsTr("Ningún producto con stock bajo.")
                                                             : qsTr("Sin productos.")
                            color: Theme.textMuted
                        }
                    }
                }
            }

            Label {
                text: qsTr("Enter/F5: entrada   F6: ajuste   Esc: volver a buscar")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
            }
        }

        // ---------- Detalle e historial ----------
        ColumnLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: 6
            spacing: Theme.spacing

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: detail.implicitHeight + 32
                color: Theme.surface
                border.color: Theme.border
                radius: Theme.radius

                ColumnLayout {
                    id: detail
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 8

                    Label {
                        visible: !root.hasSelection
                        text: qsTr("Selecciona un producto para ver su stock y movimientos.")
                        color: Theme.textMuted
                    }

                    RowLayout {
                        visible: root.hasSelection
                        Layout.fillWidth: true
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Label {
                                Layout.fillWidth: true
                                text: inventoryCtrl.selectedProduct.name || ""
                                font.pixelSize: Theme.fontSize + 3
                                font.bold: true
                                elide: Text.ElideRight
                            }
                            Label {
                                text: qsTr("Código %1 · Costo %2 · Mínimo %3")
                                      .arg(inventoryCtrl.selectedProduct.code || "")
                                      .arg(inventoryCtrl.selectedProduct.costText || "")
                                      .arg(inventoryCtrl.selectedProduct.minStockText || "")
                                color: Theme.textMuted
                            }
                        }
                        ColumnLayout {
                            spacing: 0
                            Label {
                                Layout.alignment: Qt.AlignRight
                                text: (inventoryCtrl.selectedProduct.stockNegative ? "-" : "")
                                      + (inventoryCtrl.selectedProduct.stockText || "") + " "
                                      + (inventoryCtrl.selectedProduct.unit || "")
                                font.pixelSize: 28
                                font.bold: true
                                color: inventoryCtrl.selectedProduct.lowStock || inventoryCtrl.selectedProduct.stockNegative
                                       ? Theme.danger : Theme.text
                            }
                            Label {
                                Layout.alignment: Qt.AlignRight
                                text: inventoryCtrl.selectedProduct.stockNegative ? qsTr("stock negativo")
                                      : (inventoryCtrl.selectedProduct.lowStock ? qsTr("stock bajo") : qsTr("en stock"))
                                color: inventoryCtrl.selectedProduct.lowStock || inventoryCtrl.selectedProduct.stockNegative
                                       ? Theme.danger : Theme.textMuted
                                font.pixelSize: Theme.fontSmall
                            }
                        }
                    }

                    RowLayout {
                        visible: root.hasSelection
                        spacing: 8
                        PrimaryButton { text: qsTr("Entrada (F5)"); focusPolicy: Qt.NoFocus; onClicked: root.openPurchase() }
                        Button { text: qsTr("Ajuste (F6)"); focusPolicy: Qt.NoFocus; onClicked: root.openAdjust() }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: qsTr("Movimientos")
                    font.bold: true
                    font.pixelSize: Theme.fontSize + 1
                    Layout.fillWidth: true
                }
                CheckBox {
                    text: qsTr("Todos los productos")
                    focusPolicy: Qt.NoFocus
                    checked: inventoryCtrl.showAllMovements
                    onToggled: inventoryCtrl.showAllMovements = checked
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
                            spacing: 8
                            Label { width: 130; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Fecha"); font.bold: true; color: Theme.textMuted }
                            Label { width: 80; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Tipo"); font.bold: true; color: Theme.textMuted }
                            Label { width: 80; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Cantidad"); font.bold: true; color: Theme.textMuted }
                            Label { width: 80; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Saldo"); font.bold: true; color: Theme.textMuted }
                            Label { width: movList.detailWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Detalle"); font.bold: true; color: Theme.textMuted }
                        }
                    }

                    ListView {
                        id: movList
                        readonly property int detailWidth: Math.max(120, width - 24 - 130 - 80 - 80 - 80 - 32)
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: inventoryCtrl.movements
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { }

                        delegate: Rectangle {
                            width: movList.width
                            height: 44
                            color: index % 2 ? Theme.surfaceAlt : Theme.surface
                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 8
                                Label { width: 130; height: parent.height; verticalAlignment: Text.AlignVCenter; text: model.dateText; font.pixelSize: Theme.fontSmall }
                                Label { width: 80; height: parent.height; verticalAlignment: Text.AlignVCenter; text: model.typeText }
                                Label {
                                    width: 80; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                    text: model.qtyText
                                    color: model.isIncrease ? Theme.success : Theme.danger
                                    font.bold: true
                                }
                                Label { width: 80; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: model.stockAfterText }
                                Column {
                                    width: movList.detailWidth
                                    anchors.verticalCenter: parent.verticalCenter
                                    Label {
                                        width: parent.width
                                        elide: Text.ElideRight
                                        text: inventoryCtrl.showAllMovements ? model.productName
                                                                             : (model.note !== "" ? model.note : "—")
                                    }
                                    Label {
                                        width: parent.width
                                        elide: Text.ElideRight
                                        font.pixelSize: Theme.fontSmall
                                        color: Theme.textMuted
                                        text: {
                                            var parts = []
                                            if (inventoryCtrl.showAllMovements && model.note !== "")
                                                parts.push(model.note)
                                            if (model.costText !== "")
                                                parts.push(qsTr("costo %1").arg(model.costText))
                                            parts.push(model.userName)
                                            return parts.join(" · ")
                                        }
                                    }
                                }
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: movList.count === 0
                            text: qsTr("Sin movimientos.")
                            color: Theme.textMuted
                        }
                    }
                }
            }
        }
    }

    PurchaseDialog {
        id: purchaseDialog
        onDone: {
            root.restoreSelection(purchaseDialog.product.id)
            root.notify(qsTr("Entrada registrada."), false)
        }
        onClosed: list.forceActiveFocus()
    }

    AdjustDialog {
        id: adjustDialog
        onDone: {
            root.restoreSelection(adjustDialog.product.id)
            root.notify(qsTr("Ajuste aplicado."), false)
        }
        onClosed: list.forceActiveFocus()
    }
}
