import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Importar productos desde un CSV (Excel) y exportar el catalogo.
Dialog {
    id: dlg
    onAboutToShow: help.enter("importar")
    onAboutToHide: help.leave("importar")

    property var files: []
    property var plan: ({})
    property string checkedPath: ""
    property string resultText: ""
    signal imported()

    function notify(message, isError) {
        var w = ApplicationWindow.window
        if (w && w.showToast)
            w.showToast(message, isError)
    }

    function openNew() {
        tabs.currentIndex = 0
        reloadFiles()
        pathField.text = files.length > 0 ? files[0].path : ""
        fileList.currentIndex = files.length > 0 ? 0 : -1
        plan = {}
        checkedPath = ""
        stockCheck.checked = false
        open()
    }

    function reloadFiles() {
        files = catalogCtrl.importCandidates()
    }

    function check() {
        checkedPath = pathField.text
        plan = catalogCtrl.importPlan(pathField.text, stockCheck.checked)
    }

    function doImport() {
        var r = catalogCtrl.importApply(checkedPath, stockCheck.checked)
        if (!r.ok) {
            plan = { "ok": false, "error": r.error }
            return
        }
        var msg = qsTr("Importación lista: %1 nuevo(s), %2 actualizado(s).").arg(r.created).arg(r.updated)
        if (r.categories > 0)
            msg += " " + qsTr("%1 categoría(s) nueva(s).").arg(r.categories)
        resultText = msg
        close()
        imported()
    }

    readonly property bool planReady: plan.ok === true && checkedPath === pathField.text
    readonly property int importable: planReady ? plan.newCount + plan.updateCount : 0

    title: qsTr("Importar y exportar productos")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(1000, parent.width - 40)
    height: Math.min(680, parent.height - 40)
    closePolicy: Popup.CloseOnEscape

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            TabBar {
                id: tabs
                Layout.fillWidth: true
                TabButton { text: qsTr("Importar desde Excel (CSV)"); focusPolicy: Qt.NoFocus }
                TabButton { text: qsTr("Exportar"); focusPolicy: Qt.NoFocus }
            }
            HelpButton { topic: "importar" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            // ---------------- Importar ----------------
            ColumnLayout {
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    // Archivos encontrados
                    ColumnLayout {
                        Layout.preferredWidth: 360
                        Layout.minimumWidth: 360
                        Layout.maximumWidth: 360
                        Layout.fillHeight: true
                        spacing: 6
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: qsTr("Archivos CSV encontrados"); font.bold: true; Layout.fillWidth: true }
                            Button {
                                text: qsTr("Actualizar")
                                focusPolicy: Qt.NoFocus
                                onClicked: dlg.reloadFiles()
                                HelpTip { text: qsTr("Vuelve a buscar archivos .csv (por ejemplo, si acabas de conectar una memoria USB).") }
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 170
                            color: Theme.surface
                            border.color: Theme.border
                            radius: Theme.radius
                            clip: true
                            ListView {
                                id: fileList
                                anchors.fill: parent
                                anchors.margins: 1
                                clip: true
                                model: dlg.files
                                boundsBehavior: Flickable.StopAtBounds
                                ScrollBar.vertical: ScrollBar { }
                                delegate: Rectangle {
                                    width: fileList.width
                                    height: 44
                                    color: ListView.isCurrentItem ? Theme.selection : (index % 2 ? Theme.surfaceAlt : Theme.surface)
                                    Column {
                                        anchors.verticalCenter: parent.verticalCenter
                                        x: 8
                                        width: parent.width - 16
                                        Label { width: parent.width; text: modelData.name; elide: Text.ElideMiddle; font.bold: true }
                                        Label { width: parent.width; text: modelData.dateText + " · " + modelData.folder; elide: Text.ElideMiddle; color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: { fileList.currentIndex = index; pathField.text = modelData.path }
                                        onDoubleClicked: { fileList.currentIndex = index; pathField.text = modelData.path; dlg.check() }
                                    }
                                }
                                Label {
                                    anchors.centerIn: parent
                                    width: parent.width - 20
                                    horizontalAlignment: Text.AlignHCenter
                                    wrapMode: Text.Wrap
                                    visible: fileList.count === 0
                                    text: qsTr("No hay archivos .csv en Documentos, Descargas, Escritorio ni en memorias USB.")
                                    color: Theme.textMuted
                                }
                            }
                        }
                    }

                    // Archivo y opciones
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 8
                        Label { text: qsTr("Archivo"); font.bold: true }
                        TextField {
                            id: pathField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Ruta del archivo, p. ej. E:\\productos.csv")
                            selectByMouse: true
                            onAccepted: dlg.check()
                            HelpTip { text: qsTr("Elige un archivo de la lista o escribe su ruta completa. En Excel: Guardar como → «CSV (delimitado por comas)» o «CSV UTF-8».") }
                        }
                        CheckBox {
                            id: stockCheck
                            text: qsTr("Ajustar el stock de los productos existentes con la columna «Stock»")
                            onToggled: if (dlg.checkedPath !== "") dlg.check()
                            HelpTip { text: qsTr("Sin marcar, la columna Stock solo se usa para los productos nuevos (como entrada de mercancía). Marcada, también corrige el stock de los que ya existen (como ajuste).") }
                        }
                        RowLayout {
                            PrimaryButton {
                                text: qsTr("Revisar archivo")
                                enabled: pathField.text.trim() !== ""
                                onClicked: dlg.check()
                                HelpTip { text: qsTr("Lee el archivo y muestra qué se va a crear, cambiar o rechazar. Todavía no guarda nada.") }
                            }
                            Button {
                                text: qsTr("Guardar plantilla")
                                focusPolicy: Qt.NoFocus
                                onClicked: {
                                    var r = catalogCtrl.exportTemplate(formatCombo.currentIndex === 1 ? "std" : "es")
                                    if (r.ok) {
                                        dlg.notify(qsTr("Plantilla guardada en %1").arg(r.path), false)
                                        dlg.reloadFiles()
                                        catalogCtrl.openFolder()
                                    } else {
                                        dlg.notify(r.error, true)
                                    }
                                }
                                HelpTip { text: qsTr("Crea plantilla_productos.csv con las columnas correctas y dos ejemplos, para llenarla en Excel.") }
                            }
                        }
                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSmall
                            text: qsTr("Columnas: Código, Código de barras, Nombre, Categoría, Unidad, Costo, Precio, Stock mínimo, Stock, Activo. Solo hacen falta Nombre y Precio para los nuevos; un producto existente se reconoce por su código.")
                        }
                    }
                }

                // Resultado de la revision
                Label {
                    Layout.fillWidth: true
                    visible: dlg.plan.ok !== undefined
                    wrapMode: Text.Wrap
                    text: dlg.plan.ok
                          ? qsTr("Nuevos: %1 · Cambian: %2 · Iguales: %3 · Con error: %4").arg(dlg.plan.newCount).arg(dlg.plan.updateCount)
                                .arg(dlg.plan.unchangedCount).arg(dlg.plan.errorCount)
                            + (dlg.plan.ignored !== "" ? "   " + qsTr("(columnas ignoradas: %1)").arg(dlg.plan.ignored) : "")
                          : (dlg.plan.error || "")
                    color: dlg.plan.ok ? Theme.text : Theme.danger
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Theme.surface
                    border.color: Theme.border
                    radius: Theme.radius
                    clip: true
                    ListView {
                        id: planList
                        readonly property int nameWidth: Math.max(160, width * 0.32)
                        anchors.fill: parent
                        anchors.margins: 1
                        clip: true
                        model: dlg.plan.ok ? dlg.plan.rows : []
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { }
                        delegate: Rectangle {
                            width: planList.width
                            height: 32
                            color: index % 2 ? Theme.surfaceAlt : Theme.surface
                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                spacing: 8
                                Label { width: 50; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.line; color: Theme.textMuted }
                                Label {
                                    width: 70; height: parent.height; verticalAlignment: Text.AlignVCenter
                                    text: modelData.statusText
                                    font.bold: true
                                    color: modelData.status === "error" ? Theme.danger
                                           : modelData.status === "new" ? Theme.success
                                           : modelData.status === "update" ? Theme.accent : Theme.textMuted
                                }
                                Label { width: 80; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.code; elide: Text.ElideRight }
                                Label { width: planList.nameWidth; height: parent.height; verticalAlignment: Text.AlignVCenter; text: modelData.name; elide: Text.ElideRight }
                                Label {
                                    width: planList.width - 16 - 50 - 70 - 80 - planList.nameWidth - 32
                                    height: parent.height
                                    verticalAlignment: Text.AlignVCenter
                                    text: modelData.detail
                                    elide: Text.ElideRight
                                    color: modelData.status === "error" ? Theme.danger : Theme.textMuted
                                    font.pixelSize: Theme.fontSmall
                                }
                            }
                        }
                        Label {
                            anchors.centerIn: parent
                            visible: planList.count === 0
                            text: qsTr("Pulsa «Revisar archivo» para ver qué se importará.")
                            color: Theme.textMuted
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        visible: dlg.planReady && dlg.plan.errorCount > 0
                        text: qsTr("Las filas con error no se importan. Corrígelas en el archivo y revisa otra vez.")
                        color: Theme.warning
                        elide: Text.ElideRight
                    }
                    Item { Layout.fillWidth: true; visible: !(dlg.planReady && dlg.plan.errorCount > 0) }
                    Button { text: qsTr("Cerrar (Esc)"); onClicked: dlg.close() }
                    PrimaryButton {
                        text: qsTr("Importar %n producto(s)", "", dlg.importable)
                        enabled: dlg.importable > 0
                        onClicked: dlg.doImport()
                        HelpTip { text: qsTr("Guarda los productos nuevos y los cambios, todo junto. Si algo falla no se guarda nada.") }
                    }
                }
            }

            // ---------------- Exportar ----------------
            ColumnLayout {
                spacing: 12
                Label {
                    Layout.preferredWidth: 700
                    wrapMode: Text.Wrap
                    text: qsTr("Guarda todos los productos en un CSV que se abre con Excel: para revisar precios, imprimir una lista o pasar el catálogo a otra PC (se vuelve a cargar con «Importar»).")
                }
                GridLayout {
                    columns: 2
                    columnSpacing: Theme.spacing
                    rowSpacing: 8
                    Label { text: qsTr("Formato") }
                    ComboBox {
                        id: formatCombo
                        Layout.preferredWidth: 260
                        model: [qsTr("Excel en español (;)"), qsTr("Estándar (,)")]
                        HelpTip { text: qsTr("Excel en español: punto y coma y coma decimal. Estándar: coma y punto decimal. Si en Excel todo sale en una columna, usa el otro.") }
                    }
                    Item { width: 1; height: 1 }
                    CheckBox { id: exportInactive; text: qsTr("Incluir productos inactivos") }
                }
                RowLayout {
                    PrimaryButton {
                        text: qsTr("Exportar productos")
                        onClicked: {
                            var r = catalogCtrl.exportProducts(formatCombo.currentIndex === 1 ? "std" : "es", exportInactive.checked)
                            if (r.ok) {
                                dlg.notify(qsTr("%1 producto(s) guardados en %2").arg(r.count).arg(r.path), false)
                                catalogCtrl.openFolder()
                            } else {
                                dlg.notify(r.error, true)
                            }
                        }
                    }
                    Button { text: qsTr("Abrir carpeta"); onClicked: catalogCtrl.openFolder() }
                }
                Label {
                    text: qsTr("Se guardan en %1").arg(catalogCtrl.folder)
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                }
                Item { Layout.fillHeight: true }
                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    Button { text: qsTr("Cerrar (Esc)"); onClicked: dlg.close() }
                }
            }
        }
    }
}
