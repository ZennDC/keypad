pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Configurator

ColumnLayout {
    id: root
    required property int keyIndex

    spacing: 12

    function syncIndex() {
        if (keyIndex < 0 || !Backend.keyModel || keyIndex >= Backend.keyModel.rowCount()) return;
        const actionIdx = Backend.keyModel.actionIndex(keyIndex);
        if (actionIdx !== undefined && actionIdx >= 0) {
            actionSelect.currentIndex = actionIdx;
        }
    }

    onKeyIndexChanged: syncIndex()

    Connections {
        target: Backend
        function onKeysChanged() { root.syncIndex(); }
    }

    Component.onCompleted: syncIndex()

    Text {
        text: "Configuring Switch #" + (root.keyIndex + 1)
        color: Theme.accent
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        text: "Assign Macro or Action:"
        color: Theme.textSecondary
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
                    color: Theme.textPrimary
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
                    color: itemDel.highlighted ? Theme.accent : Theme.textPrimary
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }
        }
    }

    Item { Layout.fillHeight: true }

    Button {
        Layout.fillWidth: true
        text: "Apply Switch " + (root.keyIndex + 1)
        highlighted: true
        onClicked: {
            const item = actionSelect.currentItem;
            if (item) {
                Backend.sendKeyConfig(root.keyIndex, item.type, item.code);
            }
        }
    }
}