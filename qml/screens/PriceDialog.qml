import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Cambio de precios en lote: un % sobre el precio actual o un margen sobre el costo, con redondeo.
Dialog {
    id: dlg
    onAboutToShow: help.enter("precios")
    onAboutToHide: help.leave("precios")

    readonly property var steps: [0, 100, 500, 1000, 5000, 10000]
    property var preview: ({})
    property int appliedCount: 0
    signal applied()

    function form() {
        return {
            "categoryId": productCtrl.categoryFilterModel.idAt(Math.max(0, categoryCombo.currentIndex)),
            "includeInactive": inactiveCheck.checked,
            "mode": marginRadio.checked ? "margin" : "percent",
            "percent": percentField.text,
            "step": steps[Math.max(0, stepCombo.currentIndex)],
            "rounding": roundCombo.currentIndex === 1 ? "nearest" : "up"
        }
    }

    function refresh() {
        preview = percentField.text.trim() === "" ? ({}) : catalogCtrl.pricePreview(form())
    }

    function openNew() {
        categoryCombo.currentIndex = 0
        inactiveCheck.checked = false
        percentRadio.checked = true
        percentField.text = ""
        stepCombo.currentIndex = 0
        roundCombo.currentIndex = 0
        preview = {}
        open()
    }

    function apply() {
        var r = catalogCtrl.applyPrices(form())
        if (!r.ok) {
            preview = { "ok": false, "error": r.error }
            return
        }
        appliedCount = r.count
        close()
        applied()
    }

    title: qsTr("Cambiar precios")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(860, parent.width - 40)
    height: Math.min(660, parent.height - 40)
    closePolicy: Popup.CloseOnEscape
    onOpened: percentField.forceActiveFocus()

    Timer { id: debounce; interval: 250; onTriggered: dlg.refresh() }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        GridLayout {
            Layout.fillWidth: true
            columns: 3
            columnSpacing: Theme.spacing
            rowSpacing: 8

            Label { text: qsTr("Productos") }
            ComboBox {
                id: categoryCombo
                Layout.preferredWidth: 260
                model: productCtrl.categoryFilterModel
                textRole: "name"
                onActivated: debounce.restart()
                HelpTip { text: qsTr("A qué productos se aplica: todos o solo los de una categoría.") }
            }
            RowLayout {
                CheckBox {
                    id: inactiveCheck
                    text: qsTr("Incluir inactivos")
                    onToggled: debounce.restart()
                    HelpTip { text: qsTr("También cambia el precio de los productos desactivados.") }
                }
                Item { Layout.fillWidth: true }
                HelpButton { topic: "precios" }
            }

            Label { text: qsTr("Cambio") }
            RadioButton {
                id: percentRadio
                text: qsTr("% sobre el precio actual")
                checked: true
                onToggled: debounce.restart()
                HelpTip { text: qsTr("Precio nuevo = precio actual + el porcentaje. Para bajar, escribe un número negativo (-10).") }
            }
            RadioButton {
                id: marginRadio
                text: qsTr("Costo + % de ganancia")
                onToggled: debounce.restart()
                HelpTip { text: qsTr("Precio nuevo = costo del producto + el porcentaje (50 = el precio es el costo más la mitad). Los productos sin costo no se tocan.") }
            }

            Label { text: marginRadio.checked ? qsTr("Ganancia %") : qsTr("Porcentaje") }
            TextField {
                id: percentField
                Layout.preferredWidth: 160
                font.pixelSize: Theme.fontSize + 3
                placeholderText: marginRadio.checked ? qsTr("Ej.: 40") : qsTr("Ej.: 10 o -5")
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onTextChanged: debounce.restart()
                onAccepted: dlg.refresh()
                HelpTip { text: qsTr("Porcentaje con hasta dos decimales: 10, 12,5 o -5.") }
            }
            Item { width: 1; height: 1 }

            Label { text: qsTr("Redondeo") }
            ComboBox {
                id: stepCombo
                Layout.preferredWidth: 160
                model: [qsTr("Sin redondeo"), qsTr("a 1"), qsTr("a 5"), qsTr("a 10"), qsTr("a 50"), qsTr("a 100")]
                onActivated: debounce.restart()
                HelpTip { text: qsTr("Deja precios fáciles de cobrar: con «a 5», 132 pasa a 135 (o a 130 si redondeas al más cercano).") }
            }
            ComboBox {
                id: roundCombo
                Layout.preferredWidth: 200
                enabled: stepCombo.currentIndex > 0
                model: [qsTr("hacia arriba"), qsTr("al más cercano")]
                onActivated: debounce.restart()
            }
        }

        Label {
            Layout.fillWidth: true
            text: !dlg.preview.ok ? (dlg.preview.error || qsTr("Escribe el porcentaje para ver los precios nuevos."))
                  : qsTr("%1 producto(s) cambian · %2 quedan igual").arg(dlg.preview.count).arg(dlg.preview.unchanged)
                    + (dlg.preview.skipped > 0 ? " · " + qsTr("%1 sin costo (no se cambian)").arg(dlg.preview.skipped) : "")
            color: dlg.preview.ok === false && dlg.preview.error ? Theme.danger : Theme.textMuted
            elide: Text.ElideRight
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
                    height: 32
                    color: Theme.surfaceAlt
                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        Label { width: 90; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Código"); font.bold: true; color: Theme.textMuted }
                        Label { width: rows.nameWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Producto"); font.bold: true; color: Theme.textMuted }
                        Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Actual"); font.bold: true; color: Theme.textMuted }
                        Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Nuevo"); font.bold: true; color: Theme.textMuted }
                        Label { width: 100; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Diferencia"); font.bold: true; color: Theme.textMuted }
                    }
                }
                ListView {
                    id: rows
                    readonly property int nameWidth: Math.max(140, width - 20 - 90 - 110 - 110 - 100)
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: dlg.preview.ok ? dlg.preview.rows : []
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { }
                    delegate: Rectangle {
                        width: rows.width
                        height: 34
                        color: index % 2 ? Theme.surfaceAlt : Theme.surface
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            Label { width: 90; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.code; elide: Text.ElideRight; color: Theme.textMuted }
                            Label { width: rows.nameWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.name; elide: Text.ElideRight }
                            Label { width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: modelData.oldText; color: Theme.textMuted }
                            Label {
                                width: 110; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                text: modelData.newText
                                font.bold: true
                                color: modelData.belowCost ? Theme.danger : Theme.text
                            }
                            Label {
                                width: 100; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                                text: modelData.diffText
                                color: modelData.up ? Theme.success : Theme.warning
                            }
                        }
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("En rojo: el precio nuevo queda por debajo del costo. Las ventas ya hechas conservan su precio.")
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            Button { text: qsTr("Cancelar (Esc)"); onClicked: dlg.close() }
            PrimaryButton {
                text: dlg.preview.ok ? qsTr("Aplicar a %n producto(s)", "", dlg.preview.count) : qsTr("Aplicar")
                enabled: dlg.preview.ok === true && dlg.preview.count > 0
                onClicked: confirm.open()
                HelpTip { text: qsTr("Cambia los precios de la lista de una vez. Para volver atrás habría que aplicar el cambio contrario.") }
            }
        }
    }

    Dialog {
        id: confirm
        title: qsTr("Confirmar cambio de precios")
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 440
        onOpened: confirmYes.forceActiveFocus()
        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.spacing
            Label {
                Layout.preferredWidth: 400
                wrapMode: Text.Wrap
                text: qsTr("Se cambiará el precio de %n producto(s).", "", dlg.preview.count || 0)
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button { text: qsTr("No (Esc)"); onClicked: confirm.close() }
                PrimaryButton {
                    id: confirmYes
                    text: qsTr("Sí, cambiar (Enter)")
                    onClicked: { confirm.close(); dlg.apply() }
                    Keys.onReturnPressed: { confirm.close(); dlg.apply() }
                    Keys.onEnterPressed: { confirm.close(); dlg.apply() }
                }
            }
        }
    }
}
