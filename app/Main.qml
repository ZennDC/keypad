pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Configurator

ApplicationWindow {
    id: root
    visible: true
    width: 680
    height: 540
    minimumWidth: 620
    minimumHeight: 500
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
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
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
                            wrapMode: Text.WordWrap
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
                        text: Backend.connected ? "Disconnect" : "Connect"
                        highlighted: !Backend.connected
                        onClicked: {
                            if (Backend.connected)
                                Backend.disconnectDevice();
                            else
                                Backend.connectDevice();
                        }
                    }
                }
            }

            // Main Editor Workspace
            RowLayout {
                Layout.fillWidth: true
                spacing: 20

                // 4x4 Interactive Matrix & Display
                Rectangle {
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 320
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
                            Layout.preferredHeight: 65
                            Layout.alignment: Qt.AlignCenter
                            radius: 4

                            readonly property bool isSelected: root.selectedKeyIndex === -1

                            color: isSelected ? "#1F6FEB" : (screenArea.containsMouse ? "#21262D" : "#0A0D12")
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
                                cursorShape: Qt.PointingHandCursor
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

                                    color: isSelected ? "#1F6FEB" : (keyArea.containsMouse ? "#21262D" : "#161B22")
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
                                        cursorShape: Qt.PointingHandCursor
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
                    Layout.preferredHeight: 320
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
                            model: [
                                {
                                    label: "Mute Volume",
                                    code: 0xE0E2
                                },
                                {
                                    label: "Volume Up",
                                    code: 0xE0E9
                                },
                                {
                                    label: "Volume Down",
                                    code: 0xE0EA
                                },
                                {
                                    label: "Play / Pause",
                                    code: 0xE0CD
                                },
                                {
                                    label: "Next Track",
                                    code: 0xE0B5
                                },
                                {
                                    label: "Prev Track",
                                    code: 0xE0B6
                                },
                                {
                                    label: "Key: Enter",
                                    code: 0x28
                                },
                                {
                                    label: "Key: Escape",
                                    code: 0x29
                                },
                                {
                                    label: "Key: Tab",
                                    code: 0x2B
                                },
                                {
                                    label: "Key: Space",
                                    code: 0x2C
                                },
                                {
                                    label: "Key: C",
                                    code: 0x06
                                },
                                {
                                    label: "Key: V",
                                    code: 0x19
                                },
                                {
                                    label: "Key: Z",
                                    code: 0x1D
                                },
                                {
                                    label: "Key: F13",
                                    code: 0x68
                                },
                                {
                                    label: "Key: F14",
                                    code: 0x69
                                }
                            ]
                        }

                        Item {
                            Layout.fillHeight: true
                        }

                        Button {
                            Layout.fillWidth: true
                            text: "Flash Switch " + (root.selectedKeyIndex + 1)
                            enabled: Backend.connected
                            highlighted: true
                            onClicked: {
                                var selectedCode = Number(actionSelect.currentValue);
                                Backend.sendConfig(root.selectedKeyIndex, selectedCode);
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
                            text: "Display Content / Widget:"
                            color: "#8B949E"
                            font.pixelSize: 11
                        }

                        ComboBox {
                            id: displayModeSelect
                            Layout.fillWidth: true
                            model: ["Hardware Monitor (CPU / RAM / GPU)", "Active Profile & Layer Overview", "Custom RGB565 Image / Logo", "Audio Visualizer (FFT)", "Static Color Fill / Test Pattern"]
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
                                    model: ["Portrait (240x320)", "Landscape (320x240)", "Portrait Flipped", "Landscape Flipped"]
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
                            text: "Upload ST7789 Config"
                            enabled: Backend.connected
                            highlighted: true
                            onClicked: {
                                // Sends the display mode, rotation (0-3), and backlight duty cycle (0-100)
                                if (Backend.sendDisplayConfig) {
                                    Backend.sendDisplayConfig(displayModeSelect.currentIndex, orientationSelect.currentIndex, Math.round(brightnessSlider.value));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
