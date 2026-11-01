import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import "theme"
import "components"
import "screens"

ApplicationWindow {
    id: window

    // Indices de pagina (orden del StackLayout)
    readonly property int pageSale: 0
    readonly property int pageProducts: 1
    readonly property int pageInventory: 2
    readonly property int pageCash: 3
    readonly property int pageReports: 4
    readonly property int pageSettings: 5
    readonly property int pageUsers: 6
    readonly property int pageBackups: 7
    readonly property int pageStatus: 8
    property int currentPage: app.dbReady ? pageSale : pageStatus

    readonly property bool working: app.dbReady && app.loggedIn
    readonly property bool admin: working && app.isAdmin
    // Un cajero nunca ve paginas de administrador aunque currentPage apunte a una.
    readonly property int shownPage: !app.dbReady ? pageStatus
                                     : (!app.isAdmin && currentPage !== pageSale && currentPage !== pageCash)
                                       ? pageSale : currentPage

    function showToast(message, isError) {
        toast.show(message, isError)
    }

    function go(page) {
        currentPage = page
    }

    function logout() {
        saleCtrl.clearCart()
        app.logout()
        currentPage = pageSale
    }

    visible: true
    width: 1366
    height: 768
    minimumWidth: 1024
    minimumHeight: 640
    title: app.loggedIn ? qsTr("PosCuba %1 · %2").arg(app.appVersion).arg(app.userName)
                        : qsTr("PosCuba %1").arg(app.appVersion)
    color: Theme.bg
    font.pixelSize: Theme.fontSize

    palette.window: Theme.bg
    palette.windowText: Theme.text
    palette.base: Theme.surface
    palette.alternateBase: Theme.surfaceAlt
    palette.text: Theme.text
    palette.button: Theme.surfaceAlt
    palette.buttonText: Theme.text
    palette.brightText: Theme.accentText
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentText
    palette.light: Theme.surface
    palette.midlight: Theme.surfaceAlt
    palette.mid: Theme.border
    palette.dark: Theme.border
    palette.shadow: Theme.dark ? "#000000" : "#9aa0a6"
    palette.toolTipBase: Theme.surface
    palette.toolTipText: Theme.text

    Component.onCompleted: {
        Theme.dark = Qt.binding(function() { return app.darkTheme })
        // placeholderText solo existe en el palette de Qt 6; en Qt 5 Fusion lo deriva del color del texto.
        if (palette.placeholderText !== undefined)
            palette.placeholderText = Qt.binding(function() { return Theme.textMuted })
    }

    Shortcut { sequence: "Ctrl+Q"; onActivated: Qt.quit() }
    Shortcut { sequence: "Ctrl+1"; enabled: window.working; onActivated: window.go(window.pageSale) }
    Shortcut { sequence: "Ctrl+2"; enabled: window.admin; onActivated: window.go(window.pageProducts) }
    Shortcut { sequence: "Ctrl+3"; enabled: window.admin; onActivated: window.go(window.pageInventory) }
    Shortcut { sequence: "Ctrl+4"; enabled: window.working; onActivated: window.go(window.pageCash) }
    Shortcut { sequence: "Ctrl+5"; enabled: window.admin; onActivated: window.go(window.pageReports) }
    Shortcut { sequence: "Ctrl+6"; enabled: window.admin; onActivated: window.go(window.pageSettings) }
    Shortcut { sequence: "Ctrl+7"; enabled: window.admin; onActivated: window.go(window.pageUsers) }
    Shortcut { sequence: "Ctrl+8"; enabled: window.admin; onActivated: window.go(window.pageBackups) }
    Shortcut { sequence: "Ctrl+9"; enabled: window.admin || !app.dbReady; onActivated: window.go(window.pageStatus) }
    Shortcut { sequence: "Ctrl+L"; enabled: window.working; onActivated: window.logout() }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 210
            color: Theme.sidebar

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 16
                anchors.bottomMargin: 12
                spacing: 2

                Label {
                    text: qsTr("PosCuba")
                    font.pixelSize: Theme.fontLarge
                    font.bold: true
                    Layout.leftMargin: 16
                    Layout.bottomMargin: 16
                }

                SidebarButton {
                    Layout.fillWidth: true
                    visible: window.working
                    text: qsTr("Venta")
                    shortcutText: "Ctrl+1"
                    selected: window.currentPage === window.pageSale
                    onClicked: window.go(window.pageSale)
                }
                SidebarButton {
                    Layout.fillWidth: true
                    visible: window.admin
                    text: qsTr("Productos")
                    shortcutText: "Ctrl+2"
                    selected: window.currentPage === window.pageProducts
                    onClicked: window.go(window.pageProducts)
                }
                SidebarButton {
                    Layout.fillWidth: true
                    visible: window.admin
                    text: inventoryCtrl.lowStockCount > 0 ? qsTr("Inventario (%1)").arg(inventoryCtrl.lowStockCount)
                                                          : qsTr("Inventario")
                    alert: inventoryCtrl.lowStockCount > 0
                    shortcutText: "Ctrl+3"
                    selected: window.currentPage === window.pageInventory
                    onClicked: window.go(window.pageInventory)
                }
                SidebarButton {
                    Layout.fillWidth: true
                    visible: window.working
                    text: cashCtrl.isOpen ? qsTr("Caja (abierta)") : qsTr("Caja (cerrada)")
                    shortcutText: "Ctrl+4"
                    selected: window.currentPage === window.pageCash
                    onClicked: window.go(window.pageCash)
                }
                SidebarButton {
                    Layout.fillWidth: true
                    visible: window.admin
                    text: qsTr("Reportes")
                    shortcutText: "Ctrl+5"
                    selected: window.currentPage === window.pageReports
                    onClicked: window.go(window.pageReports)
                }

                Item { Layout.fillHeight: true }

                SidebarButton {
                    Layout.fillWidth: true
                    visible: window.admin
                    text: qsTr("Configuración")
                    shortcutText: "Ctrl+6"
                    selected: window.currentPage === window.pageSettings
                    onClicked: window.go(window.pageSettings)
                }
                SidebarButton {
                    Layout.fillWidth: true
                    visible: window.admin
                    text: qsTr("Usuarios")
                    shortcutText: "Ctrl+7"
                    selected: window.currentPage === window.pageUsers
                    onClicked: window.go(window.pageUsers)
                }
                SidebarButton {
                    Layout.fillWidth: true
                    visible: window.admin
                    text: qsTr("Respaldos")
                    shortcutText: "Ctrl+8"
                    selected: window.currentPage === window.pageBackups
                    onClicked: window.go(window.pageBackups)
                }
                SidebarButton {
                    Layout.fillWidth: true
                    visible: window.admin || !app.dbReady
                    text: qsTr("Estado")
                    shortcutText: "Ctrl+9"
                    selected: window.currentPage === window.pageStatus
                    onClicked: window.go(window.pageStatus)
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: 6
                    Layout.bottomMargin: 6
                    height: 1
                    color: Theme.border
                    visible: window.working
                }
                ColumnLayout {
                    visible: window.working
                    Layout.leftMargin: 16
                    Layout.rightMargin: 8
                    spacing: 0
                    Label { text: app.userName; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
                    Label {
                        text: app.isAdmin ? qsTr("Administrador") : qsTr("Cajero")
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall
                    }
                }
                SidebarButton {
                    Layout.fillWidth: true
                    visible: window.working
                    text: qsTr("Cambiar mi PIN")
                    onClicked: changePinDialog.openNew()
                }
                SidebarButton {
                    Layout.fillWidth: true
                    visible: window.working
                    text: qsTr("Cerrar sesión")
                    shortcutText: "Ctrl+L"
                    onClicked: window.logout()
                }
                Switch {
                    Layout.leftMargin: 8
                    text: qsTr("Tema oscuro")
                    focusPolicy: Qt.NoFocus
                    checked: app.darkTheme
                    onToggled: app.darkTheme = checked
                }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            color: Theme.border
        }

        // Contenedor: ocultarlo sin sesion evita que las pantallas respondan a sus atajos detras del login.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: window.working || !app.dbReady

        StackLayout {
            anchors.fill: parent
            currentIndex: window.shownPage

            SaleScreen { }
            ProductsScreen { }
            InventoryScreen { }
            CashScreen { }
            ReportsScreen { }
            SettingsScreen { }
            UsersScreen { }
            BackupScreen { }
            StatusScreen { }
        }
        }
    }

    LoginScreen {
        anchors.fill: parent
        visible: app.dbReady && !app.loggedIn
        onLoggedIn: {
            window.currentPage = window.pageSale
            window.showToast(qsTr("Hola, %1.").arg(app.userName), false)
        }
    }

    ChangePinDialog {
        id: changePinDialog
        onDone: window.showToast(qsTr("PIN cambiado."), false)
    }

    Toast { id: toast }
}
