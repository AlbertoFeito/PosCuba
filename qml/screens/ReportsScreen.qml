import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

Item {
    id: root
    readonly property var s: reportCtrl.summary

    function notify(message, isError) {
        var w = ApplicationWindow.window
        if (w && w.showToast)
            w.showToast(message, isError)
    }

    function exported(r) {
        if (r.ok) {
            notify(qsTr("CSV guardado en %1").arg(r.path), false)
            openFolderButton.visible = true
        } else {
            notify(r.error, true)
        }
    }

    function applyRange() {
        var r = reportCtrl.setRange(fromField.text, toField.text)
        if (!r.ok)
            notify(r.error, true)
        syncFields()
    }

    onVisibleChanged: if (visible) reportCtrl.refresh()

    // Sin Connections: "function onX()" no existe en Qt 5.14 y la forma antigua avisa en Qt 6.
    function syncFields() {
        fromField.text = reportCtrl.fromText
        toField.text = reportCtrl.toText
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        // ---- Periodo ----
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Label { text: qsTr("Reportes"); font.pixelSize: Theme.fontLarge; font.bold: true }
            HelpButton { topic: "reportes"; Layout.rightMargin: 12 }
            Repeater {
                model: [
                    { "key": "today", "text": qsTr("Hoy") },
                    { "key": "yesterday", "text": qsTr("Ayer") },
                    { "key": "week", "text": qsTr("Esta semana") },
                    { "key": "month", "text": qsTr("Este mes") },
                    { "key": "lastMonth", "text": qsTr("Mes anterior") },
                    { "key": "last30", "text": qsTr("30 días") },
                    { "key": "year", "text": qsTr("Este año") }
                ]
                delegate: Button {
                    text: modelData.text
                    focusPolicy: Qt.NoFocus
                    highlighted: reportCtrl.preset === modelData.key
                    onClicked: { reportCtrl.setPreset(modelData.key); root.syncFields() }
                    HelpTip { text: qsTr("Período rápido. «Esta semana» empieza el lunes.") }
                }
            }
            Item { Layout.fillWidth: true }
            TextField {
                id: fromField
                Layout.preferredWidth: 110
                placeholderText: "dd/mm/aaaa"
                text: reportCtrl.fromText
                onAccepted: root.applyRange()
                HelpTip { text: qsTr("Fecha inicial del rango (dd/mm/aaaa). Enter aplica.") }
            }
            Label { text: "–" }
            TextField {
                id: toField
                Layout.preferredWidth: 110
                placeholderText: "dd/mm/aaaa"
                text: reportCtrl.toText
                onAccepted: root.applyRange()
                HelpTip { text: qsTr("Fecha final del rango, incluida (dd/mm/aaaa). Enter aplica.") }
            }
            Button { text: qsTr("Aplicar"); focusPolicy: Qt.NoFocus; onClicked: root.applyRange(); HelpTip { text: qsTr("Muestra el reporte del rango escrito a la izquierda.") } }
        }

        // ---- Resumen ----
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Repeater {
                model: [
                    { "label": qsTr("Ventas"), "value": String(root.s.salesCount || 0), "hint": "",
                      "tip": qsTr("Cantidad de ventas del período.") },
                    { "label": qsTr("Total vendido"), "value": root.s.totalText || "", "hint": root.s.discountText !== "0.00" ? qsTr("descuentos %1").arg(root.s.discountText) : "",
                      "tip": qsTr("Suma de los totales cobrados, ya restados los descuentos.") },
                    { "label": qsTr("Ganancia"), "value": root.s.profitText || "", "hint": qsTr("margen %1 %").arg(root.s.marginPercent || 0),
                      "tip": qsTr("Total vendido menos el costo de lo vendido (costo al momento de cada venta). El margen es la ganancia sobre el total.") },
                    { "label": qsTr("Ticket promedio"), "value": root.s.averageText || "", "hint": "",
                      "tip": qsTr("Lo que gasta en promedio cada cliente: total vendido ÷ ventas.") },
                    { "label": qsTr("Efectivo"), "value": root.s.cashText || "", "hint": "",
                      "tip": qsTr("Cobrado en efectivo, convertido a la moneda base y sin el vuelto.") },
                    { "label": qsTr("Transferencias"), "value": root.s.transferText || "", "hint": "",
                      "tip": qsTr("Cobrado por transferencia.") }
                ]
                delegate: Rectangle {
                    // ancho preferido distinto de 0: si no, RowLayout los apila en x = 0
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    implicitHeight: 74
                    radius: Theme.radius
                    color: Theme.surface
                    border.color: Theme.border
                    Column {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 2
                        Label { text: modelData.label; color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
                        Label { text: modelData.value; font.pixelSize: Theme.fontSize + 6; font.bold: true }
                        Label { text: modelData.hint; color: Theme.textMuted; font.pixelSize: Theme.fontSmall - 1; visible: text !== "" }
                    }
                    HelpTip { text: modelData.tip }
                }
            }
        }

        // ---- Grafico ----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 190
            color: Theme.surface
            border.color: Theme.border
            radius: Theme.radius

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("Ventas por período"); font.bold: true; Layout.fillWidth: true; HelpTip { text: qsTr("Barras con el total de cada día, semana o mes. Deja el mouse sobre una barra para ver su detalle.") } }
                    Label {
                        visible: reportCtrl.effectiveGroup !== reportCtrl.groupBy
                        text: qsTr("Agrupado por %1 (rango largo)").arg(reportCtrl.effectiveGroup === "week" ? qsTr("semana") : qsTr("mes"))
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall
                    }
                    Repeater {
                        model: [ { "key": "day", "text": qsTr("Día") }, { "key": "week", "text": qsTr("Semana") }, { "key": "month", "text": qsTr("Mes") } ]
                        delegate: Button {
                            text: modelData.text
                            focusPolicy: Qt.NoFocus
                            highlighted: reportCtrl.effectiveGroup === modelData.key
                            onClicked: reportCtrl.setGroupBy(modelData.key)
                            HelpTip { text: qsTr("Agrupa el gráfico y la tabla. En rangos largos se agrupa solo por semana o por mes.") }
                        }
                    }
                }

                Item {
                    id: chart
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    readonly property int n: Math.max(1, reportCtrl.periods.length)
                    readonly property real slot: width / n
                    readonly property bool showLabels: slot >= 34

                    Rectangle { anchors.bottom: parent.bottom; anchors.bottomMargin: 18; width: parent.width; height: 1; color: Theme.border }

                    Repeater {
                        model: reportCtrl.periods
                        delegate: Item {
                            x: index * chart.slot
                            width: chart.slot
                            height: chart.height

                            Rectangle {
                                id: bar
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: 19
                                width: Math.max(2, Math.min(42, chart.slot * 0.7))
                                height: reportCtrl.maxPeriodTotal > 0
                                        ? Math.max(modelData.total > 0 ? 2 : 0, (parent.height - 40) * modelData.total / reportCtrl.maxPeriodTotal)
                                        : 0
                                radius: 2
                                color: hover.containsMouse ? Qt.lighter(Theme.accent, 1.2) : Theme.accent
                            }
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: bar.top
                                anchors.bottomMargin: 2
                                visible: hover.containsMouse || (chart.slot >= 70 && modelData.total > 0)
                                text: modelData.totalText
                                font.pixelSize: Theme.fontSmall - 2
                                color: Theme.textMuted
                            }
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom
                                visible: chart.showLabels || index % Math.ceil(34 / Math.max(1, chart.slot)) === 0
                                text: modelData.label
                                font.pixelSize: Theme.fontSmall - 2
                                color: Theme.textMuted
                            }
                            MouseArea {
                                id: hover
                                anchors.fill: parent
                                hoverEnabled: true
                                ToolTip.visible: containsMouse
                                ToolTip.text: qsTr("%1: %2 · %3 venta(s) · ganancia %4").arg(modelData.label).arg(modelData.totalText)
                                                  .arg(modelData.salesCount).arg(modelData.profitText)
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: reportCtrl.maxPeriodTotal <= 0
                        text: qsTr("No hay ventas en este período.")
                        color: Theme.textMuted
                    }
                }
            }
        }

        // ---- Tablas ----
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // Periodos
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 4
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
                            Label { width: periodList.colLabel; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Período"); font.bold: true; color: Theme.textMuted }
                            Label { width: 60; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Ventas"); font.bold: true; color: Theme.textMuted }
                            Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Total"); font.bold: true; color: Theme.textMuted }
                            Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Ganancia"); font.bold: true; color: Theme.textMuted }
                        }
                    }
                    ListView {
                        id: periodList
                        readonly property int colLabel: Math.max(90, width - 20 - 60 - 220)
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: reportCtrl.periods
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { }
                        delegate: Rectangle {
                            width: periodList.width
                            height: 32
                            color: index % 2 ? Theme.surfaceAlt : Theme.surface
                            opacity: modelData.salesCount > 0 ? 1.0 : 0.55
                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                Label { width: periodList.colLabel; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.label }
                                Label { width: 60; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: modelData.salesCount }
                                Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: modelData.totalText; font.bold: true }
                                Label {
                                    width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                    text: modelData.profitText
                                    color: modelData.profitNegative ? Theme.danger : Theme.success
                                }
                            }
                        }
                    }
                }
            }

            // Productos mas vendidos
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 6
                color: Theme.surface
                border.color: Theme.border
                radius: Theme.radius
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 0
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.margins: 6
                        Label { text: qsTr("Productos más vendidos"); font.bold: true; Layout.leftMargin: 4; Layout.fillWidth: true; HelpTip { text: qsTr("Productos vendidos en el período. Importes antes del descuento general de cada venta.") } }
                        Label { text: qsTr("Ordenar:"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
                        Repeater {
                            model: [ { "key": "qty", "text": qsTr("Cantidad") }, { "key": "revenue", "text": qsTr("Importe") }, { "key": "profit", "text": qsTr("Ganancia") } ]
                            delegate: Button {
                                text: modelData.text
                                focusPolicy: Qt.NoFocus
                                highlighted: reportCtrl.topOrder === modelData.key
                                onClicked: reportCtrl.setTopOrder(modelData.key)
                                HelpTip { text: qsTr("Ordena la lista por cantidad vendida, por importe o por ganancia.") }
                            }
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        height: 30
                        color: Theme.surfaceAlt
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            Label { width: 30; height: parent.height; verticalAlignment: Text.AlignVCenter; text: "#"; font.bold: true; color: Theme.textMuted }
                            Label { width: productList.colName; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Producto"); font.bold: true; color: Theme.textMuted }
                            Label { width: 100; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Cantidad"); font.bold: true; color: Theme.textMuted }
                            Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Importe"); font.bold: true; color: Theme.textMuted }
                            Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Ganancia"); font.bold: true; color: Theme.textMuted }
                        }
                    }
                    ListView {
                        id: productList
                        readonly property int colName: Math.max(120, width - 20 - 30 - 320)
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: reportCtrl.products
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { }
                        delegate: Rectangle {
                            width: productList.width
                            height: 32
                            color: index % 2 ? Theme.surfaceAlt : Theme.surface
                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                Label { width: 30; height: parent.height; verticalAlignment: Text.AlignVCenter; text: index + 1; color: Theme.textMuted }
                                Label { width: productList.colName; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.name; elide: Text.ElideRight }
                                Label { width: 100; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: modelData.qtyText + " " + modelData.unit }
                                Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: modelData.revenueText; font.bold: true }
                                Label {
                                    width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                    text: modelData.profitText
                                    color: modelData.profitNegative ? Theme.danger : Theme.success
                                }
                            }
                        }
                        Label {
                            anchors.centerIn: parent
                            visible: productList.count === 0
                            text: qsTr("Sin ventas en este período.")
                            color: Theme.textMuted
                        }
                    }
                }
            }
        }

        // ---- Exportar ----
        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("Exportar CSV:"); color: Theme.textMuted }
            Button { text: qsTr("Ventas por período"); focusPolicy: Qt.NoFocus; onClicked: root.exported(reportCtrl.exportPeriods()); HelpTip { text: qsTr("Guarda en CSV la tabla de ventas por período (total, descuentos, costo y ganancia).") } }
            Button { text: qsTr("Productos"); focusPolicy: Qt.NoFocus; onClicked: root.exported(reportCtrl.exportProducts()); HelpTip { text: qsTr("Guarda en CSV los productos vendidos con cantidad, importe, costo y ganancia.") } }
            Button { text: qsTr("Detalle de ventas"); focusPolicy: Qt.NoFocus; onClicked: root.exported(reportCtrl.exportSales()); HelpTip { text: qsTr("Guarda en CSV una fila por venta: fecha, cajero, totales, costo, ganancia, efectivo y transferencia.") } }
            Button {
                id: openFolderButton
                visible: false
                text: qsTr("Abrir carpeta")
                focusPolicy: Qt.NoFocus
                onClicked: reportCtrl.openFolder()
            }
            Item { Layout.fillWidth: true }
            Label { text: qsTr("Formato:"); color: Theme.textMuted }
            ComboBox {
                Layout.preferredWidth: 230
                focusPolicy: Qt.NoFocus
                model: [qsTr("Excel en español (;)"), qsTr("Estándar (,)")]
                HelpTip {
                    text: qsTr("Excel en español: separador punto y coma y coma decimal. Estándar: coma y punto decimal (Excel en inglés, LibreOffice). Si el archivo se abre en una sola columna, usa el otro.")
                }
                currentIndex: reportCtrl.csvFormat === "std" ? 1 : 0
                onActivated: reportCtrl.setCsvFormat(currentIndex === 1 ? "std" : "es")
            }
        }
        Label {
            text: qsTr("Los importes por producto son antes de los descuentos aplicados a la venta completa.")
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall - 1
        }
    }
}
