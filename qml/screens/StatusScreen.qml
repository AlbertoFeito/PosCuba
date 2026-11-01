import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../theme"

Item {
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 640)
        spacing: 16

        Label {
            text: qsTr("Estado del sistema")
            font.pixelSize: Theme.fontLarge
            font.bold: true
        }

        Frame {
            Layout.fillWidth: true
            background: Rectangle {
                color: Theme.surface
                border.color: Theme.border
                radius: Theme.radius
            }

            GridLayout {
                anchors.fill: parent
                columns: 2
                columnSpacing: 16
                rowSpacing: 8

                Label { text: qsTr("Base de datos:"); font.bold: true }
                Label {
                    text: app.dbReady ? qsTr("Lista") : qsTr("Error")
                    color: app.dbReady ? Theme.success : Theme.danger
                    font.bold: true
                }
                Label { text: qsTr("Archivo:"); font.bold: true }
                Label { text: app.dbPath; elide: Text.ElideMiddle; Layout.fillWidth: true }
                Label { text: qsTr("Versión del esquema:"); font.bold: true }
                Label { text: app.schemaVersion }
                Label { text: qsTr("SQLite:"); font.bold: true }
                Label { text: app.sqliteVersion }
                Label { text: qsTr("Qt:"); font.bold: true }
                Label { text: app.qtVersion }
                Label { text: qsTr("Versión:"); font.bold: true }
                Label { text: app.appVersion }
            }
        }

        Label {
            visible: !app.dbReady
            text: app.errorMessage
            color: Theme.danger
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
    }
}
