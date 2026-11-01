import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

Dialog {
    id: dlg
    onAboutToShow: help.enter("caja")
    onAboutToHide: help.leave("caja")
    property string errorText: ""
    signal cashOpened()

    function openNew() {
        amountField.text = ""
        errorText = ""
        open()
    }

    function tryOpen() {
        var r = cashCtrl.open(amountField.text)
        if (r.ok) {
            cashOpened()
            close()
        } else {
            errorText = r.error
        }
    }

    title: qsTr("Abrir caja")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 420
    onOpened: amountField.forceActiveFocus()

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTr("Indica el efectivo con que empieza la caja (fondo para dar vuelto).")
            color: Theme.textMuted
        }
        RowLayout {
            Label { text: qsTr("Fondo inicial") }
            TextField {
                id: amountField
                Layout.fillWidth: true
                placeholderText: "0.00"
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                font.pixelSize: Theme.fontSize + 4
                onAccepted: dlg.tryOpen()
                HelpTip { text: qsTr("Efectivo en moneda base que hay en la gaveta al empezar, para dar vuelto. Puede ser 0.") }
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
            PrimaryButton { text: qsTr("Abrir caja (Enter)"); onClicked: dlg.tryOpen() }
        }
    }
}
