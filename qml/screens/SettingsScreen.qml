import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Configuracion: datos del negocio, impresora y ticket.
Item {
    id: root

    property bool dirty: false

    function notify(message, isError) {
        var w = ApplicationWindow.window
        if (w && w.showToast)
            w.showToast(message, isError)
    }

    function values() {
        return {
            "businessName": nameField.text,
            "businessAddress": addressField.text,
            "businessPhone": phoneField.text,
            "ticketFooter": footerField.text,
            "printerName": printerCombo.currentIndex <= 0 ? "" : printerCombo.currentText,
            "paperWidth": paper80.checked ? 80 : 58,
            "autoPrint": autoPrint.checked,
            "output": outputPrinter.checked ? "printer" : "pdf"
        }
    }

    function load() {
        var s = ticketCtrl.settings
        nameField.text = s.businessName || ""
        addressField.text = s.businessAddress || ""
        phoneField.text = s.businessPhone || ""
        footerField.text = s.ticketFooter || ""
        paper80.checked = s.paperWidth === 80
        paper58.checked = s.paperWidth !== 80
        autoPrint.checked = s.autoPrint === true
        outputPrinter.checked = s.output === "printer"
        outputPdf.checked = s.output !== "printer"
        var idx = printerModel().indexOf(s.printerName || "")
        printerCombo.currentIndex = (s.printerName && idx > 0) ? idx : 0
        dirty = false
    }

    function printerModel() {
        var list = [qsTr("Predeterminada del sistema") + (ticketCtrl.defaultPrinter !== "" ? " (" + ticketCtrl.defaultPrinter + ")" : "")]
        return list.concat(ticketCtrl.printers)
    }

    function save() {
        var r = ticketCtrl.saveSettings(values())
        if (r.ok) {
            dirty = false
            notify(qsTr("Configuración guardada."), false)
        } else {
            notify(r.error, true)
        }
        return r.ok
    }

    onVisibleChanged: if (visible) { ticketCtrl.refreshPrinters(); load() }

    Shortcut { sequence: "Ctrl+S"; enabled: root.visible && tabs.currentIndex === 0; onActivated: root.save() }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        TabBar {
            id: tabs
            Layout.fillWidth: true
            TabButton { text: qsTr("Negocio y ticket"); focusPolicy: Qt.NoFocus }
            TabButton { text: qsTr("Monedas y tasas"); focusPolicy: Qt.NoFocus }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            Item {
                RowLayout {
                    anchors.fill: parent
                    spacing: 20

                    Flickable {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        contentHeight: form.implicitHeight
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { }

                        ColumnLayout {
                            id: form
                            width: parent.width
                            spacing: 14

                            Label { text: qsTr("Configuración"); font.pixelSize: Theme.fontLarge; font.bold: true }

                            Label { text: qsTr("Negocio"); font.bold: true; font.pixelSize: Theme.fontSize + 1 }
                            GridLayout {
                                Layout.fillWidth: true
                                columns: 2
                                columnSpacing: Theme.spacing
                                rowSpacing: 10
                                Label { text: qsTr("Nombre *") }
                                TextField { id: nameField; Layout.fillWidth: true; maximumLength: 80; onTextEdited: root.dirty = true }
                                Label { text: qsTr("Dirección") }
                                TextField { id: addressField; Layout.fillWidth: true; maximumLength: 160; onTextEdited: root.dirty = true }
                                Label { text: qsTr("Teléfono") }
                                TextField { id: phoneField; Layout.fillWidth: true; maximumLength: 40; onTextEdited: root.dirty = true }
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                            Label { text: qsTr("Ticket e impresora"); font.bold: true; font.pixelSize: Theme.fontSize + 1 }
                            GridLayout {
                                Layout.fillWidth: true
                                columns: 2
                                columnSpacing: Theme.spacing
                                rowSpacing: 10

                                Label { text: qsTr("Salida del ticket") }
                                RowLayout {
                                    RadioButton { id: outputPdf; text: qsTr("PDF diario (una venta por hoja)"); checked: true; onToggled: root.dirty = true }
                                    RadioButton { id: outputPrinter; text: qsTr("Impresora"); onToggled: root.dirty = true }
                                }

                                Label { text: qsTr("Impresora"); enabled: outputPrinter.checked }
                                RowLayout {
                                    Layout.fillWidth: true
                                    enabled: outputPrinter.checked
                                    ComboBox {
                                        id: printerCombo
                                        Layout.fillWidth: true
                                        model: root.printerModel()
                                        onActivated: root.dirty = true
                                    }
                                    Button {
                                        text: qsTr("Actualizar")
                                        onClicked: {
                                            var current = printerCombo.currentText
                                            ticketCtrl.refreshPrinters()
                                            var idx = root.printerModel().indexOf(current)
                                            printerCombo.currentIndex = idx > 0 ? idx : 0
                                        }
                                    }
                                }

                                Label { text: qsTr("Ancho del papel") }
                                RowLayout {
                                    RadioButton { id: paper58; text: qsTr("58 mm (32 caracteres)"); checked: true; onToggled: root.dirty = true }
                                    RadioButton { id: paper80; text: qsTr("80 mm (48 caracteres)"); onToggled: root.dirty = true }
                                }

                                Label { text: qsTr("Al cobrar") }
                                CheckBox {
                                    id: autoPrint
                                    text: outputPrinter.checked ? qsTr("Imprimir el ticket automáticamente")
                                                                : qsTr("Agregar cada venta al PDF del día automáticamente")
                                    onToggled: root.dirty = true
                                }

                                Label { text: qsTr("Pie del ticket"); Layout.alignment: Qt.AlignTop }
                                TextArea {
                                    id: footerField
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 70
                                    placeholderText: qsTr("¡Gracias por su compra!")
                                    wrapMode: TextArea.Wrap
                                    onTextChanged: if (activeFocus) root.dirty = true
                                    background: Rectangle { color: Theme.surface; border.color: footerField.activeFocus ? Theme.accent : Theme.border; radius: 2 }
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                                color: Theme.textMuted
                                font.pixelSize: Theme.fontSmall
                                text: qsTr("Se usa el driver de Windows de la impresora térmica. Si el ticket sale cortado o con márgenes, "
                                           + "revisa en las propiedades de la impresora el tamaño de papel (rollo de 58 u 80 mm). "
                                           + "Los PDF se guardan en %1.").arg(ticketCtrl.pdfFolder)
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                PrimaryButton { text: qsTr("Guardar (Ctrl+S)"); onClicked: root.save() }
                                Button {
                                    text: qsTr("Imprimir prueba")
                                    visible: outputPrinter.checked
                                    onClicked: {
                                        if (root.dirty && !root.save())
                                            return
                                        var r = ticketCtrl.printTest()
                                        root.notify(r.ok ? qsTr("Ticket de prueba enviado.") : r.error, !r.ok)
                                    }
                                }
                                Button {
                                    text: qsTr("PDF de prueba")
                                    onClicked: {
                                        if (root.dirty && !root.save())
                                            return
                                        var r = ticketCtrl.exportTestPdf()
                                        if (r.ok) {
                                            root.notify(qsTr("PDF guardado en %1").arg(r.path), false)
                                            ticketCtrl.openFolder("")
                                        } else {
                                            root.notify(r.error, true)
                                        }
                                    }
                                }
                                Item { Layout.fillWidth: true }
                                Label { visible: root.dirty; text: qsTr("Cambios sin guardar"); color: Theme.warning }
                            }
                        }
                    }

                    // Vista previa en vivo
                    ColumnLayout {
                        Layout.fillHeight: true
                        Layout.preferredWidth: previewBox.width
                        spacing: 8
                        Label { text: qsTr("Vista previa"); font.bold: true }
                        Rectangle {
                            id: previewBox
                            width: preview.implicitWidth + 32
                            Layout.fillHeight: true
                            color: "#ffffff"
                            border.color: Theme.border
                            radius: Theme.radius
                            clip: true
                            Text {
                                id: preview
                                x: 16
                                y: 16
                                color: "#111111"
                                font.family: ticketCtrl.monoFamily
                                font.pixelSize: paper80.checked ? 11 : 13
                                text: ticketCtrl.previewSample({
                                    "businessName": nameField.text,
                                    "businessAddress": addressField.text,
                                    "businessPhone": phoneField.text,
                                    "ticketFooter": footerField.text,
                                    "paperWidth": paper80.checked ? 80 : 58
                                })
                            }
                        }
                    }
                }
            }

            CurrenciesPanel { }
        }
    }
}
