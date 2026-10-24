pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Configurator
import "components"

ApplicationWindow {
    id: root
    visible: true
    width: 760
    height: 580
    minimumWidth: 680
    minimumHeight: 520
    title: "Pico MacroDeck Studio"
    Material.theme: Material.Dark
    Material.accent: Material.Cyan
    color: Theme.bgApp

    property int selectedKeyIndex: 0

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        padding: 20

        ColumnLayout {
            width: parent.width
            spacing: 16

            ConnectionHeader {}

            RowLayout {
                Layout.fillWidth: true
                spacing: 20
                enabled: Backend.connected
                opacity: Backend.connected ? 1.0 : 0.35

                Behavior on opacity { NumberAnimation { duration: 200 } }

                DeviceMatrixView {
                    selectedKeyIndex: root.selectedKeyIndex
                    onKeySelected: function(idx) { root.selectedKeyIndex = idx; }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 340
                    radius: Theme.radiusCard
                    color: Theme.bgCard
                    border.color: Theme.border
                    border.width: 1

                    KeyConfigPanel {
                        anchors.fill: parent
                        anchors.margins: 16
                        keyIndex: root.selectedKeyIndex
                        visible: root.selectedKeyIndex !== -1
                    }

                    DisplayConfigPanel {
                        anchors.fill: parent
                        anchors.margins: 16
                        visible: root.selectedKeyIndex === -1
                    }
                }
            }
        }
    }
}