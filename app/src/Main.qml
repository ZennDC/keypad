pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Configurator

ApplicationWindow {
    id: root
    visible: true
    width: 720
    height: 560
    minimumWidth: 640
    minimumHeight: 520
    title: "Pico MacroDeck Studio"
    Material.theme: Material.Dark
    Material.accent: Material.Cyan

    // -1 represents the screen display; 0-15 represent physical switches
    property int selectedKeyIndex: 0

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        padding: 20

        ColumnLayout {
            width: parent.width
            spacing: 16

            // Status & Connection Header
            Rectangle {
                id: connectionCard
                Layout.fillWidth: true
                implicitHeight: 64
                radius: 10
                color: "#161B22"
                border.color: Backend.connected ? "#1F6FEB44" : "#30363D"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 14

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
                                NumberAnimation {
                                    from: 1.0
                                    to: 2.2
                                    duration: 1400
                                    easing.type: Easing.OutQuad
                                }
                                NumberAnimation {
                                    from: 2.2
                                    to: 2.2
                                    duration: 200
                                }
                            }
                            SequentialAnimation on opacity {
                                running: Backend.connected
                                loops: Animation.Infinite
                                NumberAnimation {
                                    from: 0.7
                                    to: 0.0
                                    duration: 1400
                                    easing.type: Easing.OutQuad
                                }
                                NumberAnimation {
                                    from: 0.0
                                    to: 0.0
                                    duration: 200
                                }
                            }
                        }

                        Rectangle {
                            anchors.centerIn: parent
                            width: 10
                            height: 10
                            radius: 5
                            color: Backend.connected ? "#3FB950" : "#F85149"
                            Behavior on color {
                                ColorAnimation {
                                    duration: 200
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 2

                        Text {
                            text: "Pico MacroDeck Studio (0xCafe:0x4000)"
                            color: "#E6EDF3"
                            font.pixelSize: 13
                            font.bold: true
                        }
                        Text {
                            text: Backend.statusText
                            color: "#8B949E"
                            font.pixelSize: 11
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        }
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

            // Main Editor Workspace
            RowLayout {
                Layout.fillWidth: true
                spacing: 20

                // Cascades input disabling and visually dims when disconnected
                enabled: Backend.connected
                opacity: Backend.connected ? 1.0 : 0.35

                Behavior on opacity {
                    NumberAnimation {
                        duration: 200
                    }
                }

                // 4x4 Interactive Matrix & Display
                Rectangle {
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 340
                    radius: 10
                    color: "#0D1117"
                    border.color: "#30363D"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 8

                        // Interactive Display Target
                        Rectangle {
                            id: screenItem
                            Layout.preferredWidth: 160
                            Layout.preferredHeight: 60
                            Layout.alignment: Qt.AlignCenter
                            radius: 4

                            readonly property bool isSelected: root.selectedKeyIndex === -1

                            color: isSelected ? "#1F6FEB" : ((screenArea.containsMouse && Backend.connected) ? "#21262D" : "#0A0D12")
                            border.color: isSelected ? "#58A6FF" : "#30363D"
                            border.width: 1

                            Behavior on color {
                                ColorAnimation {
                                    duration: 120
                                }
                            }

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 2

                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: "ST7789 TFT"
                                    color: screenItem.isSelected ? "#FFFFFF" : "#58A6FF"
                                    font.bold: true
                                    font.pixelSize: 11
                                }
                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: "240 × 320 IPS"
                                    color: screenItem.isSelected ? "#E6EDF3" : "#8B949E"
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

                        // Key Matrix Grid
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

                                    color: isSelected ? "#1F6FEB" : ((keyArea.containsMouse && Backend.connected) ? "#21262D" : "#161B22")
                                    border.color: isSelected ? "#58A6FF" : "#30363D"
                                    border.width: 1

                                    Behavior on color {
                                        ColorAnimation {
                                            duration: 120
                                        }
                                    }

                                    ColumnLayout {
                                        anchors.centerIn: parent
                                        spacing: 2

                                        Text {
                                            Layout.alignment: Qt.AlignHCenter
                                            text: "SW " + (gridKey.index + 1)
                                            color: gridKey.isSelected ? "#FFFFFF" : "#C9D1D9"
                                            font.bold: true
                                            font.pixelSize: 12
                                        }
                                    }

                                    MouseArea {
                                        id: keyArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Backend.connected ? Qt.PointingHandCursor : Qt.ArrowCursor
                                        onClicked: {
                                            root.selectedKeyIndex = gridKey.index;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Configuration Panel
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 340
                    radius: 10
                    color: "#161B22"
                    border.color: "#30363D"
                    border.width: 1

                    // Mode 1: Switch Configuration
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        visible: root.selectedKeyIndex !== -1

                        Text {
                            text: "Configuring Switch #" + (root.selectedKeyIndex + 1)
                            color: "#58A6FF"
                            font.pixelSize: 15
                            font.bold: true
                        }

                        Label {
                            text: "Assign Macro or Action:"
                            color: "#8B949E"
                            font.pixelSize: 11
                        }

                        ComboBox {
                            id: actionSelect
                            Layout.fillWidth: true
                            textRole: "label"
                            valueRole: "code"

                            leftPadding: 16
                            rightPadding: 40

                            model: [
                                {
                                    label: "Mute Volume",
                                    icon: "qrc:/qt/qml/Configurator/assets/icons/volume-x.svg",
                                    type: 0x02,
                                    code: 0x00E2
                                },
                                {
                                    label: "Volume Up",
                                    icon: "qrc:/qt/qml/Configurator/assets/icons/volume-2.svg",
                                    type: 0x02,
                                    code: 0x00E9
                                },
                                {
                                    label: "Volume Down",
                                    icon: "qrc:/qt/qml/Configurator/assets/icons/volume-1.svg",
                                    type: 0x02,
                                    code: 0x00EA
                                },
                                {
                                    label: "Play / Pause",
                                    icon: "qrc:/qt/qml/Configurator/assets/icons/play.svg",
                                    type: 0x02,
                                    code: 0x00CD
                                },
                                {
                                    label: "Next Track",
                                    icon: "qrc:/qt/qml/Configurator/assets/icons/fast-forward.svg",
                                    type: 0x02,
                                    code: 0x00B5
                                },
                                {
                                    label: "Previous Track",
                                    icon: "qrc:/qt/qml/Configurator/assets/icons/rewind.svg",
                                    type: 0x02,
                                    code: 0x00B6
                                },
                                {
                                    label: "Key: Enter",
                                    type: 0x01,
                                    code: 0x28
                                },
                                {
                                    label: "Key: Escape",
                                    type: 0x01,
                                    code: 0x29
                                },
                                {
                                    label: "Key: Tab",
                                    type: 0x01,
                                    code: 0x2B
                                },
                                {
                                    label: "Key: Space",
                                    type: 0x01,
                                    code: 0x2C
                                },
                                {
                                    label: "Key: C",
                                    type: 0x01,
                                    code: 0x06
                                },
                                {
                                    label: "Key: V",
                                    type: 0x01,
                                    code: 0x19
                                },
                                {
                                    label: "Key: Z",
                                    type: 0x01,
                                    code: 0x1D
                                },
                                {
                                    label: "Key: F13",
                                    type: 0x01,
                                    code: 0x68
                                },
                                {
                                    label: "Key: F14",
                                    type: 0x01,
                                    code: 0x69
                                },
                                {
                                    label: "Key: F15",
                                    type: 0x01,
                                    code: 0x6A
                                },
                                {
                                    label: "Key: F16",
                                    type: 0x01,
                                    code: 0x6B
                                }
                            ]

                            // Selected item layout
                            contentItem: Item {
                                implicitHeight: 24

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 10

                                    Image {
                                        readonly property string currentIcon: actionSelect.model[actionSelect.currentIndex]?.icon || ""
                                        visible: currentIcon !== ""
                                        source: currentIcon
                                        sourceSize.width: 16
                                        sourceSize.height: 16
                                        Layout.preferredWidth: visible ? 16 : 0
                                        Layout.preferredHeight: visible ? 16 : 0
                                        Layout.alignment: Qt.AlignVCenter
                                        fillMode: Image.PreserveAspectFit
                                    }

                                    Text {
                                        text: actionSelect.displayText
                                        color: "#E6EDF3"
                                        font.pixelSize: 13
                                        verticalAlignment: Text.AlignVCenter
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            // Dropdown item layout
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
                                        color: itemDel.highlighted ? "#58A6FF" : "#E6EDF3"
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
                                var item = actionSelect.model[actionSelect.currentIndex];
                                Backend.sendKeyConfig(root.selectedKeyIndex, item.type, item.code);
                            }
                        }
                    }

                    // Mode 2: Screen Configuration
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 10
                        visible: root.selectedKeyIndex === -1

                        Text {
                            text: "ST7789 TFT Display Configuration"
                            color: "#58A6FF"
                            font.pixelSize: 15
                            font.bold: true
                        }

                        Label {
                            text: "Display Mode:"
                            color: "#8B949E"
                            font.pixelSize: 11
                        }

                        ComboBox {
                            id: displayModeSelect
                            Layout.fillWidth: true
                            model: ["Hardware Monitor", "Active Profile Overview", "Custom RGB565 Image", "Audio Visualizer", "Color Pattern"]
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Label {
                                    text: "Orientation:"
                                    color: "#8B949E"
                                    font.pixelSize: 11
                                }

                                ComboBox {
                                    id: orientationSelect
                                    Layout.fillWidth: true
                                    model: ["Portrait", "Landscape", "Portrait Inverted", "Landscape Inverted"]
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Label {
                                    text: "Backlight: " + Math.round(brightnessSlider.value) + "%"
                                    color: "#8B949E"
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
