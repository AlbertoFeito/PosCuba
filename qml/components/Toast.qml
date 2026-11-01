import QtQuick 2.12
import QtQuick.Controls 2.12
import "../theme"

// Aviso breve en la parte inferior. Uso: toast.show("Guardado", false)
Popup {
    id: toast
    property bool isError: false
    property alias text: label.text

    function show(message, error) {
        label.text = message
        isError = error === true
        open()
        timer.restart()
    }

    x: (parent.width - width) / 2
    y: parent.height - height - 32
    padding: 14
    closePolicy: Popup.CloseOnPressOutside
    focus: false

    background: Rectangle {
        radius: Theme.radius
        color: toast.isError ? Theme.danger : (Theme.dark ? "#3a3f4a" : "#323840")
    }
    contentItem: Label {
        id: label
        color: "#ffffff"
        font.pixelSize: Theme.fontSize
    }

    Timer {
        id: timer
        interval: toast.isError ? 4500 : 2500
        onTriggered: toast.close()
    }
}
