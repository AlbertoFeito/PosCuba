import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Cambia la cantidad de una linea del carrito.
Dialog {
    id: dlg
    onAboutToShow: help.enter("venta")
    onAboutToHide: help.leave("venta")
    property int row: -1
    property string productName: ""
    property string unit: ""
    property string errorText: ""

    function openFor(r, name, u) {
        row = r
        productName = name
        unit = u
        qtyField.text = saleCtrl.qtyTextAt(r)
        errorText = ""
        open()
    }

    function apply() {
        var res = saleCtrl.setQty(row, qtyField.text)
        if (res.ok)
            close()
        else
            errorText = res.error
    }

    title: qsTr("Cantidad")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 400
    onOpened: {
        qtyField.forceActiveFocus()
        qtyField.selectAll()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing
        Label {
            Layout.fillWidth: true
            text: dlg.productName
            font.bold: true
            elide: Text.ElideRight
        }
        RowLayout {
            TextField {
                id: qtyField
                Layout.fillWidth: true
                font.pixelSize: Theme.fontSize + 6
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onAccepted: dlg.apply()
                HelpTip { text: qsTr("Cantidad nueva de la línea. Acepta decimales (0.5) para kg, lb, l o m.") }
            }
            Label { text: dlg.unit; color: Theme.textMuted; font.pixelSize: Theme.fontSize + 2 }
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
            PrimaryButton { text: qsTr("Aceptar (Enter)"); onClicked: dlg.apply() }
        }
    }
}
