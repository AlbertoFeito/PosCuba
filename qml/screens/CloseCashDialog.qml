import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Cierre con arqueo: efectivo contado por moneda frente al esperado.
Dialog {
    id: dlg
    property string errorText: ""
    property var counted: ({})        // moneda -> texto
    property var resultData: ({})
    property bool finished: false
    signal done()

    function openNew() {
        counted = {}
        errorText = ""
        finished = false
        resultData = {}
        open()
    }

    function setCount(currency, text) {
        var c = counted
        c[currency] = text
        counted = c
    }

    function tryClose() {
        var r = cashCtrl.close(counted)
        if (r.ok) {
            resultData = r
            finished = true
            done()
            closeButton.forceActiveFocus()
        } else {
            errorText = r.error
        }
    }

    title: finished ? qsTr("Caja cerrada") : qsTr("Cierre de caja (arqueo)")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(560, parent.width - 40)
    closePolicy: Popup.CloseOnEscape
    onOpened: if (countRepeater.count > 0) countRepeater.itemAt(0).focusField()

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        // ---- Conteo ----
        ColumnLayout {
            visible: !dlg.finished
            Layout.fillWidth: true
            spacing: 10

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Cuenta el efectivo de la gaveta y escribe el total por moneda.")
                color: Theme.textMuted
            }

            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("Moneda"); font.bold: true; color: Theme.textMuted; Layout.preferredWidth: 70 }
                Label { text: qsTr("Esperado"); font.bold: true; color: Theme.textMuted; Layout.preferredWidth: 120; horizontalAlignment: Text.AlignRight }
                Label { text: qsTr("Contado"); font.bold: true; color: Theme.textMuted; Layout.fillWidth: true }
                Label { text: qsTr("Diferencia"); font.bold: true; color: Theme.textMuted; Layout.preferredWidth: 110; horizontalAlignment: Text.AlignRight }
            }

            Repeater {
                id: countRepeater
                model: dlg.visible && !dlg.finished ? (cashCtrl.summary.expected || []) : []
                delegate: RowLayout {
                    property var diff: cashCtrl.previewDifference(modelData.currency, field.text)
                    function focusField() { field.forceActiveFocus() }
                    Layout.fillWidth: true
                    Label { text: modelData.currency; font.bold: true; Layout.preferredWidth: 70 }
                    Label { text: modelData.expectedText; Layout.preferredWidth: 120; horizontalAlignment: Text.AlignRight }
                    TextField {
                        id: field
                        Layout.fillWidth: true
                        font.pixelSize: Theme.fontSize + 2
                        placeholderText: "0.00"
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        onTextChanged: dlg.setCount(modelData.currency, text)
                        Component.onCompleted: dlg.setCount(modelData.currency, "")
                        onAccepted: {
                            if (index + 1 < countRepeater.count)
                                countRepeater.itemAt(index + 1).focusField()
                            else
                                dlg.tryClose()
                        }
                    }
                    Label {
                        Layout.preferredWidth: 110
                        horizontalAlignment: Text.AlignRight
                        text: field.text !== "" && diff.valid ? diff.differenceText : ""
                        color: !diff.valid || diff.differenceCents === 0 ? Theme.text
                               : (diff.differenceCents < 0 ? Theme.danger : Theme.warning)
                        font.bold: true
                    }
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
                PrimaryButton { text: qsTr("Cerrar caja"); onClicked: dlg.tryClose() }
            }
        }

        // ---- Resultado ----
        ColumnLayout {
            visible: dlg.finished
            Layout.fillWidth: true
            spacing: 8

            GridLayout {
                columns: 2
                columnSpacing: 24
                Label { text: qsTr("Esperado (en %1)").arg(saleCtrl.baseCurrency) }
                Label { text: dlg.resultData.expectedText || ""; font.bold: true }
                Label { text: qsTr("Contado (en %1)").arg(saleCtrl.baseCurrency) }
                Label { text: dlg.resultData.countedText || ""; font.bold: true }
                Label { text: qsTr("Diferencia") }
                Label {
                    text: dlg.resultData.differenceText || ""
                    font.bold: true
                    font.pixelSize: 22
                    color: dlg.resultData.differenceCents === 0 ? Theme.success
                           : (dlg.resultData.differenceCents < 0 ? Theme.danger : Theme.warning)
                }
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                color: Theme.textMuted
                text: dlg.resultData.differenceCents === 0 ? qsTr("La caja cuadra.")
                      : (dlg.resultData.differenceCents < 0 ? qsTr("Falta dinero respecto a lo esperado.")
                                                            : qsTr("Sobra dinero respecto a lo esperado."))
            }
            Button {
                id: closeButton
                Layout.alignment: Qt.AlignRight
                text: qsTr("Aceptar (Enter)")
                onClicked: dlg.close()
                Keys.onReturnPressed: dlg.close()
                Keys.onEnterPressed: dlg.close()
            }
        }
    }
}
