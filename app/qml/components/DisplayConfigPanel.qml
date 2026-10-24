pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Configurator

ColumnLayout {
    id: root
    spacing: 10

    function syncDisplay() {
        if (displayModeSelect.count > 0) {
            displayModeSelect.currentIndex = Math.min(Math.max(0, Backend.displayMode), displayModeSelect.count - 1);
        }
        if (orientationSelect.count > 0) {
            orientationSelect.currentIndex = Math.min(Math.max(0, Backend.displayOrientation), orientationSelect.count - 1);
        }
        brightnessSlider.value = Backend.displayBrightness;
    }

    Connections {
        target: Backend
        function onDisplayConfigLoaded() { root.syncDisplay(); }
    }

    Component.onCompleted: syncDisplay()

    Text {
        text: "ST7789 TFT Display Configuration"
        color: Theme.accent
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        text: "Display Mode:"
        color: Theme.textSecondary
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
                color: Theme.textSecondary
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
                color: Theme.textSecondary
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

    Item { Layout.fillHeight: true }

    Button {
        Layout.fillWidth: true
        text: "Apply Display Config"
        highlighted: true
        onClicked: {
            Backend.sendDisplayConfig(displayModeSelect.currentIndex, orientationSelect.currentIndex, Math.round(brightnessSlider.value));
        }
    }
}