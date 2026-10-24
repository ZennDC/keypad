pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Configurator

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

    // Theme palette to avoid color duplications across components
    QtObject {
        id: theme
        readonly property color bgApp: "#0D1117"
        readonly property color bgCard: "#161B22"
        readonly property color bgCardHover: "#21262D"
        readonly property color border: "#30363D"
        readonly property color borderActive: "#58A6FF"
        readonly property color accent: "#58A6FF"
        readonly property color accentSelected: "#1F6FEB"
        readonly property color textPrimary: "#E6EDF3"
        readonly property color textSecondary: "#8B949E"
        readonly property color textMuted: "#6E7681"
        readonly property color statusConnected: "#3FB950"
        readonly property color statusDisconnected: "#F85149"
    }

    // -1 represents the screen display; 0-15 represent physical switches
    property int selectedKeyIndex: 0

    // Synchronizes the actionSelect ComboBox with the currently selected switch's action
    function syncActionSelect() {
        if (!actionSelect || selectedKeyIndex < 0 || !Backend.keys || selectedKeyIndex >= Backend.keys.length) {
            return;
        }
        const key = Backend.keys[selectedKeyIndex];
        if (key && key.actionIndex !== undefined) {
            actionSelect.currentIndex = Math.max(0, key.actionIndex);
        }
    }

    // Synchronizes the display controls with values loaded from the hardware
    function syncDisplayConfig() {
        if (displayModeSelect.count > 0) {
            displayModeSelect.currentIndex = Math.min(Math.max(0, Backend.displayMode), displayModeSelect.count - 1);
        }
        if (orientationSelect.count > 0) {
            orientationSelect.currentIndex = Math.min(Math.max(0, Backend.displayOrientation), orientationSelect.count - 1);
        }
        brightnessSlider.value = Backend.displayBrightness;
    }

    onSelectedKeyIndexChanged: {
        syncActionSelect();
    }

    Connections {
        target: Backend
        function onKeysChanged() {
            root.syncActionSelect();
        }
        function onDisplayConfigLoaded() {
            root.syncDisplayConfig();
        }
    }

    Component.onCompleted: {
        syncActionSelect();
        syncDisplayConfig();
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        padding: 20

        ColumnLayout {
            width: parent.width
            spacing: 16

            // -----------------------------------------------------------------
            // Top Bar: Connection Status & Hardware Controls
            // -----------------------------------------------------------------
            Rectangle {
                id: connectionCard
                Layout.fillWidth: true
                implicitHeight: 64
                radius: 10
                color: theme.bgCard
                border.color: Backend.connected ? "#1F6FEB44" : theme.border
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 14

                    // Animated connection status dot
                    Item {
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 20
                        Layout.alignment: Qt.AlignVCenter

                        Rectangle {
                            anchors.centerIn: parent
                            width: 10
                            height: 10
                            radius: 5
                            color: Backend.connected ? "#238636" : "#DA3633"
                            opacity: 0.6
                            visible: Backend.connected

                            SequentialAnimation on scale {
                                running: Backend.connected
                                loops: Animation.Infinite
                                NumberAnimation { from: 1.0; to: 2.2; duration: 1400; easing.type: Easing.OutQuad }
                                NumberAnimation { from: 2.2; to: 2.2; duration: 200 }
                            }
                            SequentialAnimation on opacity {
                                running: Backend.connected
                                loops: Animation.Infinite
                                NumberAnimation { from: 0.7; to: 0.0; duration: 1400; easing.type: Easing.OutQuad }
                                NumberAnimation { from: 0.0; to: 0.0; duration: 200 }
                            }
                        }

                        Rectangle {
                            anchors.centerIn: parent
                            width: 10
                            height: 10
                            radius: 5
                            color: Backend.connected ? theme.statusConnected : theme.statusDisconnected
                            Behavior on color {
                                ColorAnimation { duration: 200 }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 2

                        Text {
                            text: "Pico MacroDeck Studio (0xCafe:0x4000)"
                            color: theme.textPrimary
                            font.pixelSize: 13
                            font.bold: true
                        }
                        Text {
                            text: Backend.statusText
                            color: theme.textSecondary
                            font.pixelSize: 11
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        }
                    }

                    Button {
                        Layout.alignment: Qt.AlignVCenter
                        text: "Bootloader"
                        enabled: Backend.connected
                        onClicked: Backend.rebootBootloader()
                    }

                    Button {
                        Layout.alignment: Qt.AlignVCenter
                        text: "Save to Flash"
                        enabled: Backend.connected
                        onClicked: Backend.saveToFlash()
                    }

                    Button {
                        Layout.alignment: Qt.AlignVCenter
                        text: Backend.connected ? "Disconnect" : "Connect"
                        highlighted: !Backend.connected
                        onClicked: {
                            if (Backend.connected) {
                                Backend.disconnectDevice();
                            } else {
                                Backend.connectDevice();
                            }
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            // Main Editor Workspace
            // -----------------------------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                spacing: 20

                enabled: Backend.connected
                opacity: Backend.connected ? 1.0 : 0.35

                Behavior on opacity {
                    NumberAnimation { duration: 200 }
                }

                // -------------------------------------------------------------
                // Left Panel: 4x4 Interactive Matrix & ST7789 TFT Preview
                // -------------------------------------------------------------
                Rectangle {
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 340
                    radius: 10
                    color: theme.bgApp
                    border.color: theme.border
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 8

                        // Interactive TFT Display Header
                        Rectangle {
                            id: screenItem
                            Layout.preferredWidth: 160
                            Layout.preferredHeight: 60
                            Layout.alignment: Qt.AlignCenter
                            radius: 4

                            readonly property bool isSelected: root.selectedKeyIndex === -1

                            color: isSelected ? theme.accentSelected : ((screenArea.containsMouse && Backend.connected) ? theme.bgCardHover : "#0A0D12")
                            border.color: isSelected ? theme.borderActive : theme.border
                            border.width: 1

                            Behavior on color {
                                ColorAnimation { duration: 120 }
                            }

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 2

                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: "ST7789 TFT"
                                    color: screenItem.isSelected ? "#FFFFFF" : theme.accent
                                    font.bold: true
                                    font.pixelSize: 11
                                }
                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: "240 × 320 IPS"
                                    color: screenItem.isSelected ? theme.textPrimary : theme.textSecondary
                                    font.pixelSize: 9
                                }
                            }

                            MouseArea {
                                id: screenArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Backend.connected ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: {
                                    root.selectedKeyIndex = -1;
                                }
                            }
                        }

                        // 4x4 Macro Key Matrix
                        GridLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            columns: 4
                            rows: 4
                            columnSpacing: 8
                            rowSpacing: 8

                            Repeater {
                                model: 16

                                Rectangle {
                                    id: gridKey
                                    required property int index
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 6

                                    readonly property bool isSelected: root.selectedKeyIndex === gridKey.index
                                    readonly property var keyData: (Backend.keys && gridKey.index < Backend.keys.length) ? Backend.keys[gridKey.index] : null
                                    readonly property string actionShortLabel: keyData?.shortLabel || "..."
                                    readonly property string actionIcon: keyData?.icon || ""
                                    readonly property bool isUnassigned: keyData?.isUnassigned ?? true

                                    color: isSelected ? theme.accentSelected : ((keyArea.containsMouse && Backend.connected) ? theme.bgCardHover : theme.bgCard)
                                    border.color: isSelected ? theme.borderActive : theme.border
                                    border.width: 1

                                    Behavior on color {
                                        ColorAnimation { duration: 120 }
                                    }

                                    ColumnLayout {
                                        anchors.centerIn: parent
                                        spacing: 3

                                        Image {
                                            visible: gridKey.actionIcon !== ""
                                            source: gridKey.actionIcon
                                            sourceSize.width: 18
                                            sourceSize.height: 18
                                            Layout.preferredWidth: visible ? 18 : 0
                                            Layout.preferredHeight: visible ? 18 : 0
                                            Layout.alignment: Qt.AlignHCenter
                                            fillMode: Image.PreserveAspectFit
                                            opacity: gridKey.isUnassigned ? 0.35 : 1.0
                                        }

                                        Text {
                                            visible: gridKey.actionIcon === ""
                                            Layout.alignment: Qt.AlignHCenter
                                            text: gridKey.actionShortLabel
                                            color: "#FFFFFF"
                                            font.bold: true
                                            font.pixelSize: 11
                                            elide: Text.ElideRight
                                            Layout.maximumWidth: gridKey.width - 8
                                            horizontalAlignment: Text.AlignHCenter
                                        }

                                        Text {
                                            Layout.alignment: Qt.AlignHCenter
                                            text: "SW " + (gridKey.index + 1)
                                            color: gridKey.isSelected ? "#D0D7DE" : theme.textSecondary
                                            font.pixelSize: 9
                                        }
                                    }

                                    MouseArea {
                                        id: keyArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Backend.connected ? Qt.PointingHandCursor : Qt.ArrowCursor
                                        onClicked: {
                                            root.selectedKeyIndex = gridKey.index;
                                            root.syncActionSelect();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // -------------------------------------------------------------
                // Right Panel: Switch / Display Configuration Form
                // -------------------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 340
                    radius: 10
                    color: theme.bgCard
                    border.color: theme.border
                    border.width: 1

                    // Mode 1: Physical Switch Configuration
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        visible: root.selectedKeyIndex !== -1

                        Text {
                            text: "Configuring Switch #" + (root.selectedKeyIndex + 1)
                            color: theme.accent
                            font.pixelSize: 15
                            font.bold: true
                        }

                        Label {
                            text: "Assign Macro or Action:"
                            color: theme.textSecondary
                            font.pixelSize: 11
                        }

                        ComboBox {
                            id: actionSelect
                            Layout.fillWidth: true
                            model: Backend.availableActions
                            textRole: "label"
                            valueRole: "code"

                            readonly property var currentItem: (currentIndex >= 0 && currentIndex < model.length) ? model[currentIndex] : null
                            displayText: currentItem?.label || ""

                            leftPadding: 16
                            rightPadding: 40

                            contentItem: Item {
                                implicitHeight: 24

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 10

                                    Image {
                                        readonly property string iconSrc: actionSelect.currentItem?.icon || ""
                                        visible: iconSrc !== ""
                                        source: iconSrc
                                        sourceSize.width: 16
                                        sourceSize.height: 16
                                        Layout.preferredWidth: visible ? 16 : 0
                                        Layout.preferredHeight: visible ? 16 : 0
                                        Layout.alignment: Qt.AlignVCenter
                                        fillMode: Image.PreserveAspectFit
                                    }

                                    Text {
                                        text: actionSelect.displayText
                                        color: theme.textPrimary
                                        font.pixelSize: 13
                                        verticalAlignment: Text.AlignVCenter
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            delegate: ItemDelegate {
                                id: itemDel
                                required property var modelData
                                required property int index
                                width: actionSelect.width

                                contentItem: RowLayout {
                                    spacing: 10

                                    Image {
                                        readonly property string iconPath: itemDel.modelData.icon || ""
                                        visible: iconPath !== ""
                                        source: iconPath
                                        sourceSize.width: 16
                                        sourceSize.height: 16
                                        Layout.preferredWidth: visible ? 16 : 0
                                        Layout.preferredHeight: visible ? 16 : 0
                                        fillMode: Image.PreserveAspectFit
                                    }

                                    Text {
                                        text: itemDel.modelData.label
                                        color: itemDel.highlighted ? theme.accent : theme.textPrimary
                                        font.pixelSize: 13
                                        verticalAlignment: Text.AlignVCenter
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }

                        Item {
                            Layout.fillHeight: true
                        }

                        Button {
                            Layout.fillWidth: true
                            text: "Apply Switch " + (root.selectedKeyIndex + 1)
                            highlighted: true
                            onClicked: {
                                const item = actionSelect.currentItem;
                                if (item) {
                                    Backend.sendKeyConfig(root.selectedKeyIndex, item.type, item.code);
                                }
                            }
                        }
                    }

                    // Mode 2: ST7789 TFT Screen Configuration
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 10
                        visible: root.selectedKeyIndex === -1

                        Text {
                            text: "ST7789 TFT Display Configuration"
                            color: theme.accent
                            font.pixelSize: 15
                            font.bold: true
                        }

                        Label {
                            text: "Display Mode:"
                            color: theme.textSecondary
                            font.pixelSize: 11
                        }

                        ComboBox {
                            id: displayModeSelect
                            Layout.fillWidth: true
                            model: Backend.displayModes
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Label {
                                    text: "Orientation:"
                                    color: theme.textSecondary
                                    font.pixelSize: 11
                                }

                                ComboBox {
                                    id: orientationSelect
                                    Layout.fillWidth: true
                                    model: Backend.displayOrientations
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Label {
                                    text: "Backlight: " + Math.round(brightnessSlider.value) + "%"
                                    color: theme.textSecondary
                                    font.pixelSize: 11
                                }

                                Slider {
                                    id: brightnessSlider
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 100
                                    value: 80
                                    stepSize: 5
                                }
                            }
                        }

                        Item {
                            Layout.fillHeight: true
                        }

                        Button {
                            Layout.fillWidth: true
                            text: "Apply Display Config"
                            highlighted: true
                            onClicked: {
                                Backend.sendDisplayConfig(displayModeSelect.currentIndex, orientationSelect.currentIndex, Math.round(brightnessSlider.value));
                            }
                        }
                    }
                }
            }
        }
    }
}
