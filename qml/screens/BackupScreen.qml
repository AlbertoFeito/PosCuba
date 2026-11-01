import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

Item {
    id: root

    property string selectedPath: ""
    property var details: ({})

    function notify(message, isError) {
        var w = ApplicationWindow.window
        if (w && w.showToast)
            w.showToast(message, isError)
    }

    function select(i) {
        list.currentIndex = i
        if (i >= 0 && i < backupCtrl.backups.length) {
            selectedPath = backupCtrl.backups[i].path
            details = backupCtrl.inspect(selectedPath)
        } else {
            selectedPath = ""
            details = {}
        }
    }

    function backupNow() {
        var r = backupCtrl.backupNow()
        if (r.ok) {
            notify(qsTr("Respaldo guardado en %1").arg(r.path), false)
            select(0)
        } else {
            notify(r.error, true)
        }
    }

    onVisibleChanged: {
        if (visible) {
            backupCtrl.refresh()
            select(backupCtrl.backups.length > 0 ? 0 : -1)
            keepSpin.value = backupCtrl.keepCount
            autoCheck.checked = backupCtrl.autoEnabled
        }
    }

    Shortcut { sequence: "Ctrl+B"; enabled: root.visible && !confirmDialog.visible; onActivated: root.backupNow() }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacing

            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("Respaldos"); font.pixelSize: Theme.fontLarge; font.bold: true; Layout.fillWidth: true }
                HelpButton { topic: "respaldos" }
                PrimaryButton { text: qsTr("Respaldar ahora (Ctrl+B)"); focusPolicy: Qt.NoFocus; onClicked: root.backupNow(); HelpTip { text: qsTr("Copia completa de todos los datos en la carpeta elegida. Se puede hacer con la caja abierta.") } }
            }

            // ---- Ubicacion ----
            Label { text: qsTr("Ubicación"); font.bold: true }
            // GridLayout (no Flow): un Flow dentro de un ColumnLayout reordena en bucle en Qt 5.
            GridLayout {
                Layout.fillWidth: true
                columns: 3
                rowSpacing: 6
                columnSpacing: 8
                Button {
                    text: qsTr("Carpeta de respaldos")
                    highlighted: backupCtrl.usingDefault
                    focusPolicy: Qt.NoFocus
                    onClicked: { backupCtrl.useDefaultFolder(); root.select(backupCtrl.backups.length > 0 ? 0 : -1) }
                    HelpTip { text: qsTr("Documentos\\PosCuba\\Respaldos en esta PC. Ahí van también los respaldos automáticos.") }
                }
                Repeater {
                    model: backupCtrl.drives
                    delegate: Button {
                        visible: !modelData.system
                        text: modelData.label + "  ·  " + modelData.freeText
                        highlighted: backupCtrl.folder === modelData.folder || backupCtrl.folder.replace(/\\/g, "/") === modelData.folder.replace(/\\/g, "/")
                        focusPolicy: Qt.NoFocus
                        onClicked: { backupCtrl.useDrive(modelData.root); root.select(backupCtrl.backups.length > 0 ? 0 : -1) }
                        HelpTip { text: qsTr("Guarda en esta unidad (memoria USB o disco), en la carpeta PosCuba_Respaldos.") }
                    }
                }
                Button {
                    text: qsTr("Actualizar unidades")
                    focusPolicy: Qt.NoFocus
                    onClicked: backupCtrl.refresh()
                    HelpTip { text: qsTr("Vuelve a buscar memorias USB conectadas.") }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: folderField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Otra carpeta (ej.: E:\\Respaldos)")
                    onAccepted: useFolderButton.clicked()
                    HelpTip { text: qsTr("Cualquier otra carpeta, por ejemplo E:\\Respaldos.") }
                }
                Button {
                    id: useFolderButton
                    text: qsTr("Usar carpeta")
                    focusPolicy: Qt.NoFocus
                    onClicked: {
                        var r = backupCtrl.useFolder(folderField.text)
                        if (!r.ok) root.notify(r.error, true)
                        else root.select(backupCtrl.backups.length > 0 ? 0 : -1)
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Carpeta actual: %1").arg(backupCtrl.folder)
                    color: Theme.textMuted
                    elide: Text.ElideMiddle
                }
                Button { text: qsTr("Abrir carpeta"); focusPolicy: Qt.NoFocus; onClicked: backupCtrl.openFolder() }
            }

            // ---- Lista ----
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
                            Label { width: 170; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Fecha"); font.bold: true; color: Theme.textMuted }
                            Label { width: 170; height: parent.height; verticalAlignment: Text.AlignVCenter; text: qsTr("Tipo"); font.bold: true; color: Theme.textMuted }
                            Label { width: 100; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: qsTr("Tamaño"); font.bold: true; color: Theme.textMuted }
                        }
                    }
                    ListView {
                        id: list
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: backupCtrl.backups
                        highlightMoveDuration: 0
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { }
                        onCurrentIndexChanged: if (currentIndex >= 0 && backupCtrl.backups[currentIndex] && backupCtrl.backups[currentIndex].path !== root.selectedPath) root.select(currentIndex)

                        delegate: Rectangle {
                            width: list.width
                            height: Theme.rowHeight
                            color: ListView.isCurrentItem ? Theme.selection : (index % 2 ? Theme.surfaceAlt : Theme.surface)
                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                Label { width: 170; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.dateText }
                                Label {
                                    width: 170; height: parent.height; verticalAlignment: Text.AlignVCenter
                                    text: modelData.kindText
                                    color: modelData.kind === "manual" ? Theme.text : Theme.textMuted
                                }
                                Label { width: 100; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight; text: modelData.sizeText; color: Theme.textMuted }
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: { root.select(index); list.forceActiveFocus() }
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: list.count === 0
                            text: qsTr("No hay respaldos en esta carpeta.")
                            color: Theme.textMuted
                        }
                    }
                }
            }
        }

        // ---------------- Panel derecho ----------------
        ColumnLayout {
            Layout.preferredWidth: 360
            Layout.fillHeight: true
            spacing: Theme.spacing

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: detailCol.implicitHeight + 32
                color: Theme.surface
                border.color: Theme.border
                radius: Theme.radius

                ColumnLayout {
                    id: detailCol
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 8

                    Label { text: qsTr("Respaldo seleccionado"); font.bold: true }
                    Label {
                        visible: root.selectedPath === ""
                        text: qsTr("Ninguno.")
                        color: Theme.textMuted
                    }
                    GridLayout {
                        visible: root.selectedPath !== "" && root.details.valid === true
                        columns: 2
                        columnSpacing: 16
                        rowSpacing: 4
                        Label { text: qsTr("Fecha"); color: Theme.textMuted }
                        Label { text: root.details.dateText || "" }
                        Label { text: qsTr("Productos"); color: Theme.textMuted }
                        Label { text: root.details.products !== undefined ? String(root.details.products) : "" }
                        Label { text: qsTr("Ventas"); color: Theme.textMuted }
                        Label { text: root.details.sales !== undefined ? String(root.details.sales) : "" }
                        Label { text: qsTr("Última venta"); color: Theme.textMuted }
                        Label { text: root.details.lastSaleText || "" }
                        Label { text: qsTr("Tamaño"); color: Theme.textMuted }
                        Label { text: root.details.sizeText || "" }
                    }
                    Label {
                        Layout.preferredWidth: 320
                        visible: root.selectedPath !== "" && root.details.valid !== true
                        text: root.details.error || ""
                        color: Theme.danger
                        wrapMode: Text.Wrap
                    }
                    Button {
                        Layout.fillWidth: true
                        visible: root.selectedPath !== ""
                        enabled: root.details.valid === true
                        text: qsTr("Restaurar este respaldo…")
                        focusPolicy: Qt.NoFocus
                        onClicked: confirmDialog.openNew()
                        HelpTip { text: qsTr("Reemplaza TODOS los datos actuales por los de este respaldo. Antes guarda una copia de los actuales.") }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: autoCol.implicitHeight + 32
                color: Theme.surface
                border.color: Theme.border
                radius: Theme.radius

                ColumnLayout {
                    id: autoCol
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 8
                    Label { text: qsTr("Respaldo automático"); font.bold: true }
                    CheckBox {
                        id: autoCheck
                        text: qsTr("Al iniciar cada día y al cerrar caja")
                        checked: backupCtrl.autoEnabled
                        HelpTip { text: qsTr("Respaldo automático la primera vez que se abre el programa cada día y al cerrar caja.") }
                    }
                    RowLayout {
                        Label { text: qsTr("Conservar los últimos") }
                        SpinBox { id: keepSpin; from: 1; to: 365; value: backupCtrl.keepCount; editable: true; HelpTip { text: qsTr("Cuántos respaldos automáticos se guardan; los más viejos se borran. Los manuales nunca se borran.") } }
                    }
                    Label {
                        Layout.preferredWidth: 320
                        wrapMode: Text.Wrap
                        text: qsTr("Se guardan en la carpeta de respaldos. Último: %1").arg(backupCtrl.lastAutoText)
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall
                    }
                    Button {
                        text: qsTr("Guardar")
                        focusPolicy: Qt.NoFocus
                        onClicked: {
                            var r = backupCtrl.saveAutoSettings(autoCheck.checked, keepSpin.value)
                            root.notify(r.ok ? qsTr("Ajustes de respaldo guardados.") : r.error, !r.ok)
                        }
                    }
                }
            }

            Label {
                Layout.preferredWidth: 360
                wrapMode: Text.Wrap
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                text: qsTr("Consejo: guarda un respaldo en una memoria USB al menos una vez por semana y guárdala fuera del negocio.")
            }
            Item { Layout.fillHeight: true }
        }
    }

    Dialog {
        id: confirmDialog
        onAboutToShow: help.enter("respaldos")
        onAboutToHide: help.leave("respaldos")
        property string errorText: ""

        function openNew() {
            wordField.text = ""
            errorText = ""
            open()
        }

        function doRestore() {
            if (wordField.text.trim().toUpperCase() !== qsTr("RESTAURAR"))
                return
            var r = backupCtrl.restore(root.selectedPath)
            if (r.ok) {
                close()
                root.notify(qsTr("Datos restaurados. Copia de los datos anteriores: %1. Inicia sesión de nuevo.").arg(r.safetyPath), false)
            } else {
                errorText = r.error
            }
        }

        title: qsTr("Restaurar respaldo")
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 500
        onOpened: wordField.forceActiveFocus()

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.spacing
            Label {
                Layout.preferredWidth: 460
                wrapMode: Text.Wrap
                text: qsTr("Se reemplazarán TODOS los datos actuales (productos, ventas, caja, usuarios y ajustes) por los del respaldo del %1.")
                      .arg(root.details.dateText || "")
            }
            Label {
                Layout.preferredWidth: 460
                wrapMode: Text.Wrap
                color: Theme.textMuted
                text: qsTr("Antes se guardará automáticamente una copia de los datos actuales. Después tendrás que iniciar sesión con los usuarios y PIN del respaldo.")
            }
            Label { text: qsTr("Escribe RESTAURAR para confirmar:"); font.bold: true }
            TextField {
                id: wordField
                Layout.fillWidth: true
                onAccepted: confirmDialog.doRestore()
            }
            Label {
                Layout.preferredWidth: 460
                visible: confirmDialog.errorText !== ""
                text: confirmDialog.errorText
                color: Theme.danger
                wrapMode: Text.Wrap
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button { text: qsTr("Cancelar (Esc)"); onClicked: confirmDialog.close() }
                PrimaryButton {
                    text: qsTr("Restaurar")
                    enabled: wordField.text.trim().toUpperCase() === qsTr("RESTAURAR")
                    onClicked: confirmDialog.doRestore()
                }
            }
        }
    }
}
