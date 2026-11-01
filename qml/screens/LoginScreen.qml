import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"
import "../components"

// Inicio de sesion con PIN. En el primer inicio pide crear el PIN del administrador.
Rectangle {
    id: root
    color: Theme.bg

    property int selectedIndex: 0
    property string errorText: ""
    readonly property var users: app.loginUsers
    readonly property var selectedUser: users.length > 0 ? users[Math.min(selectedIndex, users.length - 1)] : ({})

    signal loggedIn()

    function reset() {
        pinField.text = ""
        setupPin.text = ""
        setupConfirm.text = ""
        errorText = ""
        if (selectedIndex >= users.length)
            selectedIndex = 0
        focusInput()
    }

    function focusInput() {
        if (app.needsSetup)
            setupPin.forceActiveFocus()
        else
            pinField.forceActiveFocus()
    }

    function selectUser(i) {
        selectedIndex = i
        pinField.text = ""
        errorText = ""
        pinField.forceActiveFocus()
    }

    function tryLogin() {
        if (!selectedUser.id)
            return
        var r = app.login(selectedUser.id, pinField.text)
        pinField.text = ""
        if (r.ok) {
            errorText = ""
            loggedIn()
        } else {
            errorText = r.error
        }
    }

    function trySetup() {
        var r = app.setupAdmin(setupPin.text, setupConfirm.text)
        if (r.ok) {
            reset()
            loggedIn()
        } else {
            errorText = r.error
        }
    }

    function typeDigit(d) {
        if (pinField.text.length < 8)
            pinField.text += d
        pinField.forceActiveFocus()
    }

    // Diferido: al cerrar sesion se ocultan las pantallas despues y se llevarian el foco.
    onVisibleChanged: if (visible) { reset(); Qt.callLater(focusInput) }
    Component.onCompleted: reset()

    // Bloquea los clics hacia la ventana de abajo
    MouseArea { anchors.fill: parent }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(560, parent.width - 40)
        spacing: 18

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("PosCuba")
            font.pixelSize: 40
            font.bold: true
        }

        // ---------- Primer inicio ----------
        ColumnLayout {
            visible: app.needsSetup
            Layout.fillWidth: true
            spacing: 12

            // Ancho fijo: un Label con ajuste de linea y fillWidth reordena en bucle en Qt 5.
            Label {
                Layout.preferredWidth: 500
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: qsTr("Configuración inicial: crea el PIN de «%1». Lo usarás para entrar y para administrar productos, usuarios y reportes.")
                      .arg(app.setupUserName)
                color: Theme.textMuted
            }
            TextField {
                id: setupPin
                Layout.fillWidth: true
                font.pixelSize: 22
                horizontalAlignment: Text.AlignHCenter
                echoMode: TextInput.Password
                maximumLength: 8
                inputMethodHints: Qt.ImhDigitsOnly
                placeholderText: qsTr("PIN nuevo (4 a 8 dígitos)")
                onAccepted: setupConfirm.forceActiveFocus()
            }
            TextField {
                id: setupConfirm
                Layout.fillWidth: true
                font.pixelSize: 22
                horizontalAlignment: Text.AlignHCenter
                echoMode: TextInput.Password
                maximumLength: 8
                inputMethodHints: Qt.ImhDigitsOnly
                placeholderText: qsTr("Repite el PIN")
                onAccepted: root.trySetup()
            }
            PrimaryButton {
                Layout.fillWidth: true
                implicitHeight: 48
                text: qsTr("Guardar PIN y entrar")
                onClicked: root.trySetup()
            }
        }

        // ---------- Inicio de sesion ----------
        ColumnLayout {
            visible: !app.needsSetup
            Layout.alignment: Qt.AlignHCenter
            spacing: 14

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("¿Quién eres?")
                color: Theme.textMuted
            }

            // GridLayout (no Flow): un Flow dentro de un ColumnLayout reordena en bucle en Qt 5.
            GridLayout {
                Layout.alignment: Qt.AlignHCenter
                columns: Math.max(1, Math.min(3, root.users.length))
                rowSpacing: 10
                columnSpacing: 10
                Repeater {
                    model: root.users
                    delegate: Rectangle {
                        Layout.preferredWidth: 170
                        Layout.preferredHeight: 64
                        radius: Theme.radius
                        color: index === root.selectedIndex ? Theme.selection : Theme.surface
                        border.color: index === root.selectedIndex ? Theme.accent : Theme.border
                        border.width: index === root.selectedIndex ? 2 : 1
                        Column {
                            anchors.centerIn: parent
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: modelData.name
                                font.bold: true
                                font.pixelSize: Theme.fontSize + 1
                            }
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: modelData.hasPin ? modelData.roleText : qsTr("sin PIN")
                                color: modelData.hasPin ? Theme.textMuted : Theme.warning
                                font.pixelSize: Theme.fontSmall
                            }
                        }
                        MouseArea { anchors.fill: parent; onClicked: root.selectUser(index) }
                    }
                }
            }

            TextField {
                id: pinField
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 260
                font.pixelSize: 26
                horizontalAlignment: Text.AlignHCenter
                echoMode: TextInput.Password
                maximumLength: 8
                inputMethodHints: Qt.ImhDigitsOnly
                placeholderText: qsTr("PIN de %1").arg(root.selectedUser.name || "")
                onAccepted: root.tryLogin()
                // ← → cambian de usuario sin usar el raton
                Keys.onLeftPressed: root.selectUser((root.selectedIndex + root.users.length - 1) % Math.max(1, root.users.length))
                Keys.onRightPressed: root.selectUser((root.selectedIndex + 1) % Math.max(1, root.users.length))
            }

            // Teclado numerico en pantalla
            GridLayout {
                Layout.alignment: Qt.AlignHCenter
                columns: 3
                rowSpacing: 8
                columnSpacing: 8
                Repeater {
                    model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "⌫", "0", "OK"]
                    delegate: Button {
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 52
                        text: modelData
                        font.pixelSize: 20
                        focusPolicy: Qt.NoFocus
                        highlighted: modelData === "OK"
                        onClicked: {
                            if (modelData === "OK")
                                root.tryLogin()
                            else if (modelData === "⌫")
                                pinField.text = pinField.text.slice(0, -1)
                            else
                                root.typeDigit(modelData)
                        }
                    }
                }
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("← → cambiar de usuario · Enter entrar")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
            }
        }

        Label {
            Layout.preferredWidth: 500
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Text.AlignHCenter
            visible: root.errorText !== ""
            text: root.errorText
            color: Theme.danger
            wrapMode: Text.Wrap
            font.bold: true
        }
    }

    Label {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 12
        text: qsTr("Versión %1").arg(app.appVersion)
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall
    }
}
