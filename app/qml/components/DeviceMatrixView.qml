pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Configurator

Rectangle {
    id: root
    property int selectedKeyIndex: 0
    signal keySelected(int index)

    Layout.preferredWidth: 320
    Layout.preferredHeight: 340
    radius: Theme.radiusCard
    color: Theme.bgApp
    border.color: Theme.border
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        Rectangle {
            id: screenItem
            Layout.preferredWidth: 160
            Layout.preferredHeight: 60
            Layout.alignment: Qt.AlignCenter
            radius: 4

            readonly property bool isSelected: root.selectedKeyIndex === -1

            color: isSelected ? Theme.accentSelected : ((screenArea.containsMouse && Backend.connected) ? Theme.bgCardHover : "#0A0D12")
            border.color: isSelected ? Theme.borderActive : Theme.border
            border.width: 1

            Behavior on color { ColorAnimation { duration: 120 } }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 2

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "ST7789 TFT"
                    color: screenItem.isSelected ? "#FFFFFF" : Theme.accent
                    font.bold: true
                    font.pixelSize: 11
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "240 × 320 IPS"
                    color: screenItem.isSelected ? Theme.textPrimary : Theme.textSecondary
                    font.pixelSize: 9
                }
            }

            MouseArea {
                id: screenArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Backend.connected ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: root.keySelected(-1)
            }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 4
            rows: 4
            columnSpacing: 8
            rowSpacing: 8

            Repeater {
                model: Backend.keyModel

                delegate: Rectangle {
                    id: gridKey
                    required property int index
                    required property string shortLabel
                    required property string icon
                    required property bool isUnassigned

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 6

                    readonly property bool isSelected: root.selectedKeyIndex === gridKey.index

                    color: isSelected ? Theme.accentSelected : ((keyArea.containsMouse && Backend.connected) ? Theme.bgCardHover : Theme.bgCard)
                    border.color: isSelected ? Theme.borderActive : Theme.border
                    border.width: 1

                    Behavior on color { ColorAnimation { duration: 120 } }

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 3

                        Image {
                            visible: gridKey.icon !== ""
                            source: gridKey.icon
                            sourceSize.width: 18
                            sourceSize.height: 18
                            Layout.preferredWidth: visible ? 18 : 0
                            Layout.preferredHeight: visible ? 18 : 0
                            Layout.alignment: Qt.AlignHCenter
                            fillMode: Image.PreserveAspectFit
                            opacity: gridKey.isUnassigned ? 0.35 : 1.0
                        }

                        Text {
                            visible: gridKey.icon === ""
                            Layout.alignment: Qt.AlignHCenter
                            text: gridKey.shortLabel
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
                            color: gridKey.isSelected ? "#D0D7DE" : Theme.textSecondary
                            font.pixelSize: 9
                        }
                    }

                    MouseArea {
                        id: keyArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Backend.connected ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: root.keySelected(gridKey.index)
                    }
                }
            }
        }
    }
}