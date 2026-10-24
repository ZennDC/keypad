#include "actions.hpp"
#include <QVariantMap>

namespace Actions {

const std::vector<ActionInfo>& catalog() {
    static const std::vector<ActionInfo> actionCatalog = {
        {QStringLiteral("No Action (Disabled)"), QStringLiteral("None"),
         QStringLiteral("qrc:/qt/qml/Configurator/assets/icons/square-dashed-plus.svg"), 0x00, 0x0000},
        // Consumer / Media Controls
        {QStringLiteral("Mute Volume"), QStringLiteral("Mute"),
         QStringLiteral("qrc:/qt/qml/Configurator/assets/icons/volume-x.svg"), 0x02, 0x00E2},
        {QStringLiteral("Volume Up"), QStringLiteral("Vol +"),
         QStringLiteral("qrc:/qt/qml/Configurator/assets/icons/volume-2.svg"), 0x02, 0x00E9},
        {QStringLiteral("Volume Down"), QStringLiteral("Vol -"),
         QStringLiteral("qrc:/qt/qml/Configurator/assets/icons/volume-1.svg"), 0x02, 0x00EA},
        {QStringLiteral("Play / Pause"), QStringLiteral("Play/Pause"),
         QStringLiteral("qrc:/qt/qml/Configurator/assets/icons/play.svg"), 0x02, 0x00CD},
        {QStringLiteral("Next Track"), QStringLiteral("Next"),
         QStringLiteral("qrc:/qt/qml/Configurator/assets/icons/fast-forward.svg"), 0x02, 0x00B5},
        {QStringLiteral("Previous Track"), QStringLiteral("Prev"),
         QStringLiteral("qrc:/qt/qml/Configurator/assets/icons/rewind.svg"), 0x02, 0x00B6},
        // Keyboard Inputs
        {QStringLiteral("Key: Enter"), QStringLiteral("Enter"), QString(), 0x01, 0x0028},
        {QStringLiteral("Key: Escape"), QStringLiteral("Escape"), QString(), 0x01, 0x0029},
        {QStringLiteral("Key: Tab"), QStringLiteral("Tab"), QString(), 0x01, 0x002B},
        {QStringLiteral("Key: Space"), QStringLiteral("Space"), QString(), 0x01, 0x002C},
        {QStringLiteral("Key: C"), QStringLiteral("C"), QString(), 0x01, 0x0006},
        {QStringLiteral("Key: V"), QStringLiteral("V"), QString(), 0x01, 0x0019},
        {QStringLiteral("Key: Z"), QStringLiteral("Z"), QString(), 0x01, 0x001D},
        {QStringLiteral("Key: F13"), QStringLiteral("F13"), QString(), 0x01, 0x0068},
        {QStringLiteral("Key: F14"), QStringLiteral("F14"), QString(), 0x01, 0x0069},
        {QStringLiteral("Key: F15"), QStringLiteral("F15"), QString(), 0x01, 0x006A},
        {QStringLiteral("Key: F16"), QStringLiteral("F16"), QString(), 0x01, 0x006B}};
    return actionCatalog;
}

int findIndex(uint8_t reportType, uint16_t usageCode) {
    if (usageCode == 0x0000 || reportType == 0) {
        return 0;
    }
    const auto& cat = catalog();
    for (size_t i = 1; i < cat.size(); ++i) {
        if (cat[i].reportType == reportType && cat[i].usageCode == usageCode) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

ActionInfo resolve(uint8_t reportType, uint16_t usageCode) {
    int idx = findIndex(reportType, usageCode);
    if (idx >= 0) {
        return catalog()[static_cast<size_t>(idx)];
    }
    ActionInfo custom;
    custom.reportType = reportType;
    custom.usageCode = usageCode;
    custom.shortLabel = QStringLiteral("0x%1").arg(usageCode, 4, 16, QChar('0')).toUpper();
    custom.label = QStringLiteral("Custom (%1)").arg(custom.shortLabel);
    return custom;
}

QVariantList toVariantList() {
    const auto& cat = catalog();
    QVariantList list;
    list.reserve(static_cast<int>(cat.size()));
    for (const auto& a : cat) {
        QVariantMap map;
        map[QStringLiteral("label")] = a.label;
        map[QStringLiteral("shortLabel")] = a.shortLabel;
        map[QStringLiteral("icon")] = a.icon;
        map[QStringLiteral("type")] = a.reportType;
        map[QStringLiteral("code")] = a.usageCode;
        list.append(map);
    }
    return list;
}

const QStringList& displayModes() {
    static const QStringList modes = {QStringLiteral("Hardware Monitor"), QStringLiteral("Profile Overview"),
                                      QStringLiteral("Custom Image")};
    return modes;
}

const QStringList& displayOrientations() {
    static const QStringList orientations = {QStringLiteral("Portrait (0°)"), QStringLiteral("Landscape (90°)"),
                                             QStringLiteral("Portrait Inverted (180°)"),
                                             QStringLiteral("Landscape Inverted (270°)")};
    return orientations;
}

} // namespace Actions