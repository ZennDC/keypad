pragma Singleton
import QtQuick
import QtQml

QtObject {
    id: root

    // Surface & background layers
    readonly property color bgApp: "#0D1117"
    readonly property color bgCard: "#161B22"
    readonly property color bgCardHover: "#21262D"
    readonly property color bgCardActive: "#30363D"

    // Borders & dividers
    readonly property color border: "#30363D"
    readonly property color borderActive: "#58A6FF"
    readonly property color borderSubtle: "#21262D"

    // Accents
    readonly property color accent: "#58A6FF"
    readonly property color accentSelected: "#1F6FEB"
    readonly property color accentHover: "#388BFD"

    // Typography
    readonly property color textPrimary: "#E6EDF3"
    readonly property color textSecondary: "#8B949E"
    readonly property color textMuted: "#6E7681"

    // Hardware status
    readonly property color statusConnected: "#3FB950"
    readonly property color statusDisconnected: "#F85149"
    readonly property color statusWarning: "#D29922"

    // Metrics
    readonly property int radiusCard: 10
    readonly property int radiusButton: 6
    readonly property int defaultSpacing: 12
}