pragma Singleton
import QtQuick 2.12

QtObject {
    property bool dark: false

    readonly property color bg: dark ? "#16181d" : "#f3f4f6"
    readonly property color surface: dark ? "#1f2229" : "#ffffff"
    readonly property color surfaceAlt: dark ? "#262a32" : "#f6f7f9"
    readonly property color sidebar: dark ? "#111317" : "#e8ebf0"
    readonly property color text: dark ? "#e8eaed" : "#1f2328"
    readonly property color textMuted: dark ? "#9aa0a6" : "#5f6670"
    readonly property color border: dark ? "#353a44" : "#d5d9e0"
    readonly property color accent: dark ? "#4c8dff" : "#1d5fd1"
    readonly property color accentText: "#ffffff"
    readonly property color selection: dark ? "#24406e" : "#d6e4fb"
    readonly property color danger: dark ? "#ff6b6b" : "#c62828"
    readonly property color success: dark ? "#5ccf8a" : "#2e7d32"
    readonly property color warning: dark ? "#f0b429" : "#b45309"

    readonly property int fontSize: 15
    readonly property int fontSmall: 13
    readonly property int fontLarge: 22
    readonly property int rowHeight: 40
    readonly property int spacing: 12
    readonly property int radius: 6
}
