import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

Item {
    id: root

    readonly property bool interactive: visible && !productDialog.visible && !confirmDialog.visible
    readonly property int colCode: 120
    readonly property int colCategory: 170
    readonly property int colMoney: 110
    readonly property int colStock: 120

    function notify(message, isError) {
        var w = ApplicationWindow.window
        if (w && w.showToast)
            w.showToast(message, isError)
    }

    function focusSearch() {
        searchField.forceActiveFocus()
        searchField.selectAll()
    }

    function currentId() {
        return productCtrl.products.idAt(list.currentIndex)
    }

    function editCurrent() {
        var id = currentId()
        if (id > 0)
            productDialog.openEdit(id)
    }

    function toggleActiveCurrent() {
        if (list.currentItem === null)
            return
        if (list.currentItem.isActive) {
            confirmDialog.productId = currentId()
            confirmDialog.productName = list.currentItem.productName
            confirmDialog.open()
        } else {
            var r = productCtrl.setActive(currentId(), true)
            notify(r.ok ? qsTr("Producto reactivado.") : r.error, !r.ok)
        }
    }

    function selectId(id) {
        var row = productCtrl.products.rowOfId(id)
        if (row >= 0) {
            list.currentIndex = row
            list.positionViewAtIndex(row, ListView.Contain)
        }
        list.forceActiveFocus()
    }

    onVisibleChanged: if (visible) focusSearch()

    Shortcut {
        sequence: "Ctrl+N"
        enabled: root.interactive
        onActivated: productDialog.openNew()
    }
    Shortcut {
        sequence: "F2"
        enabled: root.interactive
        onActivated: root.focusSearch()
    }
    Shortcut {
        sequence: "F3"
        enabled: root.interactive
        onActivated: root.editCurrent()
    }

    Timer {
        id: searchDebounce
        interval: 200
        onTriggered: productCtrl.searchText = searchField.text
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: Theme.spacing

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Productos")
                font.pixelSize: Theme.fontLarge
                font.bold: true
                Layout.fillWidth: true
            }
            PrimaryButton {
                text: qsTr("Nuevo producto (Ctrl+N)")
                focusPolicy: Qt.NoFocus
                onClicked: productDialog.openNew()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Buscar por nombre, código o código de barras (F2)")
                selectByMouse: true
                onTextChanged: searchDebounce.restart()
                Keys.onDownPressed: {
                    if (list.count > 0) {
                        list.currentIndex = 0
                        list.forceActiveFocus()
                    }
                }
                // Enter (o el lector de barras): aplica la búsqueda ya; si hay un solo resultado, lo abre.
                onAccepted: {
                    searchDebounce.stop()
                    productCtrl.searchText = searchField.text
                    if (list.count === 1) {
                        list.currentIndex = 0
                        root.editCurrent()
                    } else if (list.count > 1) {
                        list.currentIndex = 0
                        list.forceActiveFocus()
                    }
                }
                Keys.onEscapePressed: searchField.text = ""
            }

            ComboBox {
                id: categoryFilter
                Layout.preferredWidth: 230
                model: productCtrl.categoryFilterModel
                textRole: "name"
                focusPolicy: Qt.NoFocus
                onActivated: productCtrl.categoryFilter = productCtrl.categoryFilterModel.idAt(currentIndex)
            }

            CheckBox {
                text: qsTr("Mostrar inactivos")
                focusPolicy: Qt.NoFocus
                checked: productCtrl.showInactive
                onToggled: productCtrl.showInactive = checked
            }
        }

        // Tabla
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
                    height: 36
                    color: Theme.surfaceAlt

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        Label { width: root.colCode; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Código"); font.bold: true; color: Theme.textMuted }
                        Label { width: list.nameWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Nombre"); font.bold: true; color: Theme.textMuted }
                        Label { width: root.colCategory; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Categoría"); font.bold: true; color: Theme.textMuted }
                        Label { width: root.colMoney; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Costo"); font.bold: true; color: Theme.textMuted }
                        Label { width: root.colMoney; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Precio"); font.bold: true; color: Theme.textMuted }
                        Label { width: root.colStock; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Stock"); font.bold: true; color: Theme.textMuted }
                    }
                }

                ListView {
                    id: list
                    readonly property int nameWidth: Math.max(160, width - 24 - root.colCode - root.colCategory
                                                                  - 2 * root.colMoney - root.colStock)
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: productCtrl.products
                    highlightMoveDuration: 0
                    keyNavigationWraps: false
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { }

                    Keys.onReturnPressed: root.editCurrent()
                    Keys.onEnterPressed: root.editCurrent()
                    Keys.onDeletePressed: root.toggleActiveCurrent()
                    Keys.onEscapePressed: root.focusSearch()
                    Keys.onUpPressed: {
                        if (currentIndex <= 0)
                            root.focusSearch()
                        else
                            decrementCurrentIndex()
                    }

                    delegate: Rectangle {
                        id: row
                        readonly property bool isActive: model.active
                        readonly property string productName: model.name

                        width: list.width
                        height: Theme.rowHeight
                        color: ListView.isCurrentItem && list.activeFocus ? Theme.selection
                               : ListView.isCurrentItem ? Qt.darker(Theme.surfaceAlt, Theme.dark ? 0.8 : 1.04)
                               : (index % 2 ? Theme.surfaceAlt : Theme.surface)

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            opacity: row.isActive ? 1.0 : 0.5

                            Label { width: root.colCode; height: parent.height; verticalAlignment: Text.AlignVCenter; text: model.code; elide: Text.ElideRight }
                            Label {
                                width: list.nameWidth; height: parent.height; verticalAlignment: Text.AlignVCenter
                                text: model.name + (row.isActive ? "" : "  " + qsTr("(inactivo)"))
                                elide: Text.ElideRight
                            }
                            Label { width: root.colCategory; height: parent.height; verticalAlignment: Text.AlignVCenter; text: model.categoryName; color: Theme.textMuted; elide: Text.ElideRight }
                            Label { width: root.colMoney; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: model.costText; color: Theme.textMuted }
                            Label { width: root.colMoney; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: model.priceText; font.bold: true }
                            Label {
                                width: root.colStock; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                text: model.stockText + " " + model.unit
                                color: model.lowStock ? Theme.danger : Theme.text
                                font.bold: model.lowStock
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
                                root.editCurrent()
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: list.count === 0
                        text: productCtrl.searchText !== "" ? qsTr("Sin resultados para «%1».").arg(productCtrl.searchText)
                                                            : qsTr("No hay productos. Crea el primero con Ctrl+N.")
                        color: Theme.textMuted
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("%n producto(s)", "", list.count) + "   ·   "
                      + qsTr("Enter/F3: editar   Supr: desactivar/reactivar   Esc: volver a buscar")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Button {
                text: qsTr("Editar")
                enabled: list.currentIndex >= 0 && list.count > 0
                focusPolicy: Qt.NoFocus
                onClicked: root.editCurrent()
            }
            Button {
                text: list.currentItem && !list.currentItem.isActive ? qsTr("Reactivar") : qsTr("Desactivar")
                enabled: list.currentIndex >= 0 && list.count > 0
                focusPolicy: Qt.NoFocus
                onClicked: root.toggleActiveCurrent()
            }
        }
    }

    ProductDialog {
        id: productDialog
        onSaved: {
            root.notify(qsTr("Producto guardado."), false)
            root.selectId(productDialog.savedId)
        }
        onClosed: if (root.visible && !list.activeFocus) list.forceActiveFocus()
    }

    Dialog {
        id: confirmDialog
        property int productId: 0
        property string productName: ""

        title: qsTr("Desactivar producto")
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 460
        onOpened: confirmButton.forceActiveFocus()

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.spacing
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("¿Desactivar «%1»?\nNo se borra: deja de aparecer en búsquedas y ventas, y se puede reactivar.")
                      .arg(confirmDialog.productName)
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button { text: qsTr("Cancelar (Esc)"); onClicked: confirmDialog.reject() }
                PrimaryButton {
                    id: confirmButton
                    text: qsTr("Desactivar (Enter)")
                    onClicked: confirmDialog.accept()
                    Keys.onReturnPressed: confirmDialog.accept()
                    Keys.onEnterPressed: confirmDialog.accept()
                }
            }
        }

        onAccepted: {
            var r = productCtrl.setActive(productId, false)
            root.notify(r.ok ? qsTr("Producto desactivado.") : r.error, !r.ok)
        }
        onClosed: list.forceActiveFocus()
    }
}
