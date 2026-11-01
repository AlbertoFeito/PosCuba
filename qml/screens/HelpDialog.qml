import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Manual de usuario: indice, buscador y texto de la seccion.
Popup {
    id: dlg

    property string currentId: ""
    property string highlight: ""        // termino buscado a resaltar en la seccion
    property var history: []
    readonly property bool searching: searchField.text.trim() !== ""
    readonly property var results: searching ? help.search(searchField.text) : []
    readonly property int currentIndex: help.indexOf(currentId)

    function openAt(id) {
        searchField.text = ""
        history = []
        show(help.indexOf(id) >= 0 ? id : (help.sections.length > 0 ? help.sections[0].id : ""), "")
        open()
    }

    function show(id, term) {
        if (help.indexOf(id) < 0)
            return
        currentId = id
        highlight = term || ""
        body.text = help.html(id, Theme.accent, Theme.border, Theme.fontSize)
        body.deselect()
        flick.contentY = 0
        if (highlight !== "")
            Qt.callLater(dlg.markFirst)
    }

    function navigate(id, term) {
        if (id === currentId && (term || "") === "")
            return
        var h = history
        h.push(currentId)
        history = h
        show(id, term)
    }

    function back() {
        if (history.length === 0)
            return
        var h = history
        var id = h.pop()
        history = h
        show(id, "")
    }

    // Resalta la primera palabra buscada y la lleva a la vista.
    function markFirst() {
        var words = highlight.trim().split(/\s+/)
        if (words.length === 0 || words[0] === "")
            return
        var plain = body.getText(0, body.length)
        var at = help.find(plain, words[0], 0)
        if (at < 0)
            return
        body.select(at, at + words[0].length)
        var r = body.positionToRectangle(at)
        flick.contentY = Math.max(0, Math.min(r.y - 60, flick.contentHeight - flick.height))
    }

    function followLink(link) {
        if (link.indexOf("ayuda:") === 0)
            navigate(link.substring(6), "")
    }

    function openResult(i) {
        if (searching) {
            if (i >= 0 && i < results.length)
                navigate(results[i].id, searchField.text)
        } else if (i >= 0 && i < help.sections.length) {
            navigate(help.sections[i].id, "")
        }
    }

    function step(delta) {
        var i = currentIndex + delta
        if (i >= 0 && i < help.sections.length)
            navigate(help.sections[i].id, "")
    }

    function notify(message, isError) {
        var w = ApplicationWindow.window
        if (w && w.showToast)
            w.showToast(message, isError)
    }

    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    width: Math.min(1240, parent.width - 40)
    height: parent.height - 40
    modal: true
    focus: true
    padding: 0
    z: 100
    closePolicy: Popup.CloseOnEscape
    onOpened: searchField.forceActiveFocus()

    background: Rectangle {
        color: Theme.bg
        border.color: Theme.border
        radius: Theme.radius
    }

    Shortcut { sequence: "Alt+Left"; enabled: dlg.visible; onActivated: dlg.back() }
    Shortcut { sequence: "Ctrl+F"; enabled: dlg.visible; onActivated: { searchField.forceActiveFocus(); searchField.selectAll() } }
    Shortcut { sequence: "PgDown"; enabled: dlg.visible; onActivated: flick.contentY = Math.min(flick.contentY + flick.height * 0.9, Math.max(0, flick.contentHeight - flick.height)) }
    Shortcut { sequence: "PgUp"; enabled: dlg.visible; onActivated: flick.contentY = Math.max(0, flick.contentY - flick.height * 0.9) }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Label { text: qsTr("Manual de usuario"); font.pixelSize: Theme.fontLarge; font.bold: true }
            Label { text: qsTr("PosCuba %1").arg(app.appVersion); color: Theme.textMuted; Layout.fillWidth: true }
            CheckBox {
                text: qsTr("Mostrar ayudas al pasar el mouse")
                focusPolicy: Qt.NoFocus
                checked: help.showTips
                onToggled: help.showTips = checked
                // Este texto se ve aunque las ayudas esten apagadas: explica como recuperarlas.
                ToolTip.visible: hovered
                ToolTip.delay: 700
                ToolTip.text: qsTr("Al dejar el mouse quieto sobre un botón o campo aparece una explicación corta como esta.")
            }
            Button {
                text: qsTr("Guardar en PDF")
                focusPolicy: Qt.NoFocus
                onClicked: {
                    var r = help.exportPdf()
                    dlg.notify(r.ok ? qsTr("Manual guardado en %1").arg(r.path) : r.error, !r.ok)
                }
                HelpTip { text: qsTr("Guarda el manual completo en un PDF para imprimirlo o leerlo en otra PC.") }
            }
            Button { text: qsTr("Cerrar (Esc)"); focusPolicy: Qt.NoFocus; onClicked: dlg.close() }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            // ---------- Indice y busqueda ----------
            ColumnLayout {
                Layout.preferredWidth: 300
                Layout.maximumWidth: 300
                Layout.fillWidth: false
                Layout.fillHeight: true
                spacing: 8

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Buscar en el manual (Ctrl+F)")
                    selectByMouse: true
                    onTextChanged: list.currentIndex = dlg.searching && dlg.results.length > 0 ? 0 : -1
                    onAccepted: if (dlg.searching && dlg.results.length > 0) dlg.openResult(Math.max(0, list.currentIndex))
                    Keys.onDownPressed: { list.forceActiveFocus(); if (list.currentIndex < 0 && list.count > 0) list.currentIndex = 0 }
                    Keys.onEscapePressed: { if (text !== "") text = ""; else dlg.close() }
                }

                Label {
                    visible: dlg.searching
                    text: dlg.results.length === 0 ? qsTr("Sin resultados.") : qsTr("%n sección(es)", "", dlg.results.length)
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Theme.surface
                    border.color: Theme.border
                    radius: Theme.radius
                    clip: true

                    ListView {
                        id: list
                        anchors.fill: parent
                        anchors.margins: 1
                        clip: true
                        model: dlg.searching ? dlg.results : help.sections
                        highlightMoveDuration: 0
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { }
                        Keys.onReturnPressed: dlg.openResult(currentIndex)
                        Keys.onEnterPressed: dlg.openResult(currentIndex)
                        Keys.onUpPressed: { if (currentIndex <= 0) searchField.forceActiveFocus(); else decrementCurrentIndex() }
                        Keys.onEscapePressed: dlg.close()

                        delegate: Rectangle {
                            readonly property bool isOpen: modelData.id === dlg.currentId
                            width: list.width
                            height: col.implicitHeight + 16
                            color: ListView.isCurrentItem && list.activeFocus ? Theme.selection
                                   : isOpen ? Theme.surfaceAlt
                                   : (mouse.containsMouse ? Theme.surfaceAlt : "transparent")
                            Rectangle { width: 4; height: parent.height; color: Theme.accent; visible: parent.isOpen }
                            Column {
                                id: col
                                x: 14
                                width: parent.width - 24
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 2
                                Label {
                                    width: parent.width
                                    text: dlg.searching ? modelData.title : (index + 1) + ". " + modelData.title
                                    font.bold: parent.parent.isOpen
                                    wrapMode: Text.Wrap
                                }
                                Label {
                                    width: parent.width
                                    visible: dlg.searching
                                    text: modelData.snippet || ""
                                    color: Theme.textMuted
                                    font.pixelSize: Theme.fontSmall
                                    wrapMode: Text.Wrap
                                    maximumLineCount: 3
                                    elide: Text.ElideRight
                                }
                            }
                            MouseArea {
                                id: mouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: { list.currentIndex = index; dlg.openResult(index) }
                            }
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("↑ ↓ elegir · Enter abrir · Alt+← volver")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                    elide: Text.ElideRight
                }
            }

            // ---------- Texto ----------
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Theme.surface
                    border.color: Theme.border
                    radius: Theme.radius
                    clip: true

                    Flickable {
                        id: flick
                        anchors.fill: parent
                        anchors.margins: 1
                        contentWidth: width
                        contentHeight: body.height + 48
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        TextEdit {
                            id: body
                            x: Math.max(24, (flick.width - width) / 2)
                            y: 20
                            width: Math.min(860, flick.width - 48)
                            readOnly: true
                            selectByMouse: true
                            persistentSelection: true
                            textFormat: TextEdit.RichText
                            wrapMode: TextEdit.Wrap
                            color: Theme.text
                            selectionColor: Theme.dark ? "#7a5c00" : "#ffe08a"
                            selectedTextColor: Theme.text
                            font.pixelSize: Theme.fontSize
                            onLinkActivated: dlg.followLink(hoveredLink)

                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.NoButton
                                cursorShape: body.hoveredLink !== "" ? Qt.PointingHandCursor : Qt.IBeamCursor
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        text: qsTr("← Volver")
                        enabled: dlg.history.length > 0
                        focusPolicy: Qt.NoFocus
                        onClicked: dlg.back()
                        HelpTip { text: qsTr("Regresa a la sección que estabas leyendo antes de seguir un enlace (Alt+←).") }
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: dlg.currentIndex > 0 ? qsTr("‹ %1").arg(help.sections[dlg.currentIndex - 1].title) : qsTr("‹ Anterior")
                        enabled: dlg.currentIndex > 0
                        focusPolicy: Qt.NoFocus
                        onClicked: dlg.step(-1)
                    }
                    Button {
                        text: dlg.currentIndex >= 0 && dlg.currentIndex + 1 < help.sections.length
                              ? qsTr("%1 ›").arg(help.sections[dlg.currentIndex + 1].title) : qsTr("Siguiente ›")
                        enabled: dlg.currentIndex >= 0 && dlg.currentIndex + 1 < help.sections.length
                        focusPolicy: Qt.NoFocus
                        onClicked: dlg.step(1)
                    }
                }
            }
        }
    }
}
