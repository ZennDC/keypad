pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Configurator

Rectangle {
    id: root

    Layout.fillWidth: true
    implicitHeight: 64
    radius: Theme.radiusCard
    color: Theme.bgCard
    border.color: Backend.connected ? "#1F6FEB44" : Theme.border
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
                color: Backend.connected ? Theme.statusConnected : Theme.statusDisconnected
                Behavior on color { ColorAnimation { duration: 200 } }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            Text {
                text: "Pico MacroDeck Studio (0xCafe:0x4000)"
                color: Theme.textPrimary
                font.pixelSize: 13
                font.bold: true
            }
            Text {
                text: Backend.statusText
                color: Theme.textSecondary
                font.pixelSize: 11
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }
        }

        Button {
            text: "Bootloader"
            enabled: Backend.connected
            onClicked: Backend.rebootBootloader()
        }

        Button {
            text: "Save to Flash"
            enabled: Backend.connected
            onClicked: Backend.saveToFlash()
        }

        Button {
            text: Backend.connected ? "Disconnect" : "Connect"
            highlighted: !Backend.connected
            onClicked: Backend.connected ? Backend.disconnectDevice() : Backend.connectDevice()
        }
    }
}