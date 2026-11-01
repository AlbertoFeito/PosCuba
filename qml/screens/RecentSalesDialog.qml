import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Ultimas ventas: vista previa del ticket, reimpresion y PDF.
Dialog {
    id: dlg
    property var sales: []
    property string preview: ""

    function notify(message, isError) {
        var w = ApplicationWindow.window
        if (w && w.showToast)
            w.showToast(message, isError)
    }

    function selectedId() {
        return list.currentIndex >= 0 && list.currentIndex < sales.length ? sales[list.currentIndex].id : 0
    }

    function openNew() {
        sales = ticketCtrl.recentSales(50)
        list.currentIndex = sales.length > 0 ? 0 : -1
        updatePreview()
        open()
    }

    function updatePreview() {
        var id = selectedId()
        preview = id > 0 ? ticketCtrl.previewText(id) : ""
    }

    function printSelected() {
        var id = selectedId()
        if (id <= 0)
            return
        var r = ticketCtrl.printSale(id)
        notify(r.ok ? qsTr("Ticket enviado a la impresora.") : r.error, !r.ok)
    }

    function pdfSelected() {
        var id = selectedId()
        if (id <= 0)
            return
        var r = ticketCtrl.exportSalePdf(id)
        if (r.ok) {
            notify(qsTr("PDF guardado en %1").arg(r.path), false)
            lastPdf = r.path
        } else {
            notify(r.error, true)
        }
    }

    property string lastPdf: ""

    title: qsTr("Ventas recientes")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(900, parent.width - 40)
    height: Math.min(620, parent.height - 40)
    onOpened: list.forceActiveFocus()

    RowLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        ListView {
            id: list
            Layout.preferredWidth: 360
            Layout.fillHeight: true
            clip: true
            model: dlg.sales
            highlightMoveDuration: 0
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { }
            onCurrentIndexChanged: dlg.updatePreview()
            Keys.onReturnPressed: dlg.printSelected()
            Keys.onEnterPressed: dlg.printSelected()

            delegate: Rectangle {
                width: list.width
                height: 48
                color: ListView.isCurrentItem ? Theme.selection : (index % 2 ? Theme.surfaceAlt : Theme.surface)
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    Column {
                        Layout.fillWidth: true
                        Label { text: qsTr("#%1 · %2").arg(modelData.number).arg(modelData.dateText); font.bold: true }
                        Label {
                            text: qsTr("%1 línea(s) · %2").arg(modelData.itemCount).arg(modelData.userName)
                                  + (modelData.voided ? " · " + qsTr("ANULADA") : "")
                            color: modelData.voided ? Theme.danger : Theme.textMuted
                            font.pixelSize: Theme.fontSmall
                        }
                    }
                    Label { text: modelData.totalText; font.bold: true }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: { list.currentIndex = index; list.forceActiveFocus() }
                    onDoubleClicked: { list.currentIndex = index; dlg.printSelected() }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: list.count === 0
                text: qsTr("Todavía no hay ventas.")
                color: Theme.textMuted
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacing

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#ffffff"
                border.color: Theme.border
                radius: Theme.radius
                clip: true

                Flickable {
                    id: flick
                    anchors.fill: parent
                    anchors.margins: 12
                    contentWidth: ticketText.implicitWidth
                    contentHeight: ticketText.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { }

                    Text {
                        id: ticketText
                        text: dlg.preview
                        color: "#111111"
                        font.family: ticketCtrl.monoFamily
                        font.pixelSize: 13
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Button {
                    text: qsTr("Abrir carpeta")
                    visible: dlg.lastPdf !== ""
                    focusPolicy: Qt.NoFocus
                    onClicked: ticketCtrl.openFolder("")
                }
                Item { Layout.fillWidth: true }
                Button { text: qsTr("Cerrar (Esc)"); focusPolicy: Qt.NoFocus; onClicked: dlg.close() }
                Button {
                    text: qsTr("PDF del día")
                    enabled: list.count > 0
                    focusPolicy: Qt.NoFocus
                    onClicked: dlg.pdfSelected()
                }
                PrimaryButton {
                    text: qsTr("Imprimir (Enter)")
                    enabled: list.count > 0
                    focusPolicy: Qt.NoFocus
                    onClicked: dlg.printSelected()
                }
            }
        }
    }
}
