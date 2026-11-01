import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Monedas y tasas de cambio (pestana de Configuracion).
Item {
    id: root

    property bool editing: false      // false = moneda nueva
    property bool editingBase: false
    property string errorText: ""

    function notify(message, isError) {
        var w = ApplicationWindow.window
        if (w && w.showToast)
            w.showToast(message, isError)
    }

    function newCurrency(code, name, symbol) {
        editing = false
        editingBase = false
        list.currentIndex = -1
        codeField.text = code || ""
        nameField.text = name || ""
        symbolField.text = symbol || ""
        rateField.text = ""
        activeCheck.checked = true
        errorText = ""
        if (code)
            rateField.forceActiveFocus()
        else
            codeField.forceActiveFocus()
    }

    function edit(i) {
        var c = currencyCtrl.currencies[i]
        if (!c)
            return
        list.currentIndex = i
        editing = true
        editingBase = c.isBase
        codeField.text = c.code
        nameField.text = c.name
        symbolField.text = c.symbol
        rateField.text = c.isBase ? "1" : c.rateText
        activeCheck.checked = c.active
        errorText = ""
        if (!c.isBase) {
            rateField.forceActiveFocus()
            rateField.selectAll()
        }
    }

    function save() {
        var form = {
            "code": codeField.text,
            "name": nameField.text,
            "symbol": symbolField.text,
            "rate": editingBase ? "" : rateField.text,
            "active": activeCheck.checked
        }
        var r = editing ? currencyCtrl.update(form) : currencyCtrl.add(form)
        if (r.ok) {
            errorText = ""
            notify(qsTr("Moneda %1 guardada.").arg(codeField.text.toUpperCase()), false)
            for (var i = 0; i < currencyCtrl.currencies.length; ++i) {
                if (currencyCtrl.currencies[i].code === codeField.text.trim().toUpperCase()) {
                    edit(i)
                    break
                }
            }
        } else {
            errorText = r.error
        }
    }

    onVisibleChanged: {
        if (visible) {
            currencyCtrl.refresh()
            if (currencyCtrl.currencies.length > 1)
                edit(1)
            else
                newCurrency("USD", qsTr("Dólar estadounidense"), "US$")
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 20

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacing

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                color: Theme.textMuted
                text: qsTr("Los precios y reportes están en %1 (moneda base). Las demás monedas se aceptan en efectivo al cobrar con la tasa indicada. Cada pago guarda la tasa usada, así que cambiarla no altera ventas anteriores.")
                      .arg(currencyCtrl.baseCode)
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
                            Label { width: list.colName; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Nombre"); font.bold: true; color: Theme.textMuted }
                            Label { width: 190; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Tasa"); font.bold: true; color: Theme.textMuted }
                            Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Estado"); font.bold: true; color: Theme.textMuted }
                        }
                    }
                    ListView {
                        id: list
                        readonly property int colName: Math.max(120, width - 24 - 80 - 190 - 110)
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: currencyCtrl.currencies
                        currentIndex: -1
                        boundsBehavior: Flickable.StopAtBounds
                        delegate: Rectangle {
                            width: list.width
                            height: Theme.rowHeight
                            color: ListView.isCurrentItem ? Theme.selection : (index % 2 ? Theme.surfaceAlt : Theme.surface)
                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                opacity: modelData.active ? 1.0 : 0.5
                                Label { width: 80; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.code; font.bold: true }
                                Label { width: list.colName; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.name + " (" + modelData.symbol + ")"; elide: Text.ElideRight }
                                Label {
                                    width: 190; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                    text: modelData.isBase ? "—" : qsTr("1 %1 = %2 %3").arg(modelData.code).arg(modelData.rateText).arg(currencyCtrl.baseCode)
                                }
                                Label {
                                    width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                    text: modelData.isBase ? qsTr("Base") : (modelData.active ? qsTr("Activa") : qsTr("Inactiva"))
                                    color: modelData.isBase ? Theme.accent : (modelData.active ? Theme.success : Theme.textMuted)
                                }
                            }
                            MouseArea { anchors.fill: parent; onClicked: root.edit(index) }
                        }
                    }
                }
            }

            RowLayout {
                Label { text: qsTr("Agregar:"); color: Theme.textMuted }
                Button { text: "USD"; focusPolicy: Qt.NoFocus; onClicked: root.newCurrency("USD", qsTr("Dólar estadounidense"), "US$") }
                Button { text: "MLC"; focusPolicy: Qt.NoFocus; onClicked: root.newCurrency("MLC", qsTr("Moneda libremente convertible"), "MLC") }
                Button { text: "EUR"; focusPolicy: Qt.NoFocus; onClicked: root.newCurrency("EUR", qsTr("Euro"), "€") }
                Button { text: qsTr("Otra"); focusPolicy: Qt.NoFocus; onClicked: root.newCurrency("", "", "") }
            }
        }

        // ---- Formulario ----
        Rectangle {
            Layout.preferredWidth: 380
            Layout.alignment: Qt.AlignTop
            implicitHeight: form.implicitHeight + 32
            color: Theme.surface
            border.color: Theme.border
            radius: Theme.radius

            ColumnLayout {
                id: form
                anchors.fill: parent
                anchors.margins: 16
                spacing: 10

                Label {
                    text: root.editing ? qsTr("Editar %1").arg(codeField.text) : qsTr("Nueva moneda")
                    font.bold: true
                    font.pixelSize: Theme.fontSize + 2
                }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: Theme.spacing
                    rowSpacing: 8
                    Label { text: qsTr("Código") }
                    TextField {
                        id: codeField
                        Layout.fillWidth: true
                        maximumLength: 3
                        readOnly: root.editing
                        font.capitalization: Font.AllUppercase
                        onAccepted: nameField.forceActiveFocus()
                    }
                    Label { text: qsTr("Nombre") }
                    TextField { id: nameField; Layout.fillWidth: true; maximumLength: 40; onAccepted: symbolField.forceActiveFocus() }
                    Label { text: qsTr("Símbolo") }
                    TextField { id: symbolField; Layout.fillWidth: true; maximumLength: 5; onAccepted: rateField.forceActiveFocus() }
                    Label { text: qsTr("Tasa") }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("1 %1 =").arg(codeField.text.toUpperCase() || "…") }
                        TextField {
                            id: rateField
                            Layout.fillWidth: true
                            enabled: !root.editingBase
                            font.pixelSize: Theme.fontSize + 3
                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                            placeholderText: "400"
                            onAccepted: root.save()
                        }
                        Label { text: currencyCtrl.baseCode }
                    }
                    Item { width: 1; height: 1 }
                    CheckBox {
                        id: activeCheck
                        text: qsTr("Aceptar al cobrar")
                        enabled: !root.editingBase
                    }
                }

                Label {
                    Layout.preferredWidth: 340
                    visible: root.errorText !== ""
                    text: root.errorText
                    color: Theme.danger
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    PrimaryButton { text: qsTr("Guardar (Enter)"); onClicked: root.save() }
                    Item { Layout.fillWidth: true }
                    Button {
                        visible: root.editing && !root.editingBase && currencyCtrl.canChangeBase
                        text: qsTr("Usar como base")
                        onClicked: {
                            var r = currencyCtrl.setBase(codeField.text)
                            root.notify(r.ok ? qsTr("Moneda base cambiada. Revisa las tasas de las demás monedas.") : r.error, !r.ok)
                            if (r.ok) root.edit(0)
                        }
                    }
                }

                Label {
                    Layout.preferredWidth: 340
                    visible: root.editingBase
                    wrapMode: Text.Wrap
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                    text: currencyCtrl.canChangeBase
                          ? qsTr("Es la moneda base. Puedes elegir otra mientras no haya ventas registradas.")
                          : qsTr("Es la moneda base. Ya hay ventas registradas, por eso no se puede cambiar.")
                }
            }
        }
    }
}
