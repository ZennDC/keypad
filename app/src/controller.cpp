#include "controller.hpp"
#include <hidapi/hidapi.h>
#include <QDebug>
#include <array>

using namespace zdc::protocol;

namespace {
constexpr uint8_t ConfigReportId = static_cast<uint8_t>(ReportId::ConfigFeature);
}

const std::vector<ActionInfo>& DeviceController::actionCatalog() {
    static const std::vector<ActionInfo> catalog = {
        {
            QStringLiteral("No Action (Disabled)"),
            QStringLiteral("None"),
            QStringLiteral("qrc:/qt/qml/Configurator/assets/icons/square-dashed-plus.svg"),
            0x00,
            0x0000
        },
        // Consumer / Media Controls
        {
            QStringLiteral("Mute Volume"),
            QStringLiteral("Mute"),
            QStringLiteral("qrc:/qt/qml/Configurator/assets/icons/volume-x.svg"),
            0x02,
            0x00E2
        },
        {
            QStringLiteral("Volume Up"),
            QStringLiteral("Vol +"),
            QStringLiteral("qrc:/qt/qml/Configurator/assets/icons/volume-2.svg"),
            0x02,
            0x00E9
        },
        {
            QStringLiteral("Volume Down"),
            QStringLiteral("Vol -"),
            QStringLiteral("qrc:/qt/qml/Configurator/assets/icons/volume-1.svg"),
            0x02,
            0x00EA
        },
        {
            QStringLiteral("Play / Pause"),
            QStringLiteral("Play/Pause"),
            QStringLiteral("qrc:/qt/qml/Configurator/assets/icons/play.svg"),
            0x02,
            0x00CD
        },
        {
            QStringLiteral("Next Track"),
            QStringLiteral("Next"),
            QStringLiteral("qrc:/qt/qml/Configurator/assets/icons/fast-forward.svg"),
            0x02,
            0x00B5
        },
        {
            QStringLiteral("Previous Track"),
            QStringLiteral("Prev"),
            QStringLiteral("qrc:/qt/qml/Configurator/assets/icons/rewind.svg"),
            0x02,
            0x00B6
        },
        // Keyboard Inputs
        {
            QStringLiteral("Key: Enter"),
            QStringLiteral("Enter"),
            QString(),
            0x01,
            0x28
        },
        {
            QStringLiteral("Key: Escape"),
            QStringLiteral("Escape"),
            QString(),
            0x01,
            0x29
        },
        {
            QStringLiteral("Key: Tab"),
            QStringLiteral("Tab"),
            QString(),
            0x01,
            0x2B
        },
        {
            QStringLiteral("Key: Space"),
            QStringLiteral("Space"),
            QString(),
            0x01,
            0x2C
        },
        {
            QStringLiteral("Key: C"),
            QStringLiteral("C"),
            QString(),
            0x01,
            0x06
        },
        {
            QStringLiteral("Key: V"),
            QStringLiteral("V"),
            QString(),
            0x01,
            0x19
        },
        {
            QStringLiteral("Key: Z"),
            QStringLiteral("Z"),
            QString(),
            0x01,
            0x1D
        },
        {
            QStringLiteral("Key: F13"),
            QStringLiteral("F13"),
            QString(),
            0x01,
            0x68
        },
        {
            QStringLiteral("Key: F14"),
            QStringLiteral("F14"),
            QString(),
            0x01,
            0x69
        },
        {
            QStringLiteral("Key: F15"),
            QStringLiteral("F15"),
            QString(),
            0x01,
            0x6A
        },
        {
            QStringLiteral("Key: F16"),
            QStringLiteral("F16"),
            QString(),
            0x01,
            0x6B
        }
    };
    return catalog;
}

int DeviceController::findActionIndex(uint8_t type, uint16_t code) {
    if (code == 0x0000 || type == 0) {
        return 0;
    }
    const auto& catalog = actionCatalog();
    for (size_t i = 1; i < catalog.size(); ++i) {
        if (catalog[i].reportType == type && catalog[i].usageCode == code) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

ActionInfo DeviceController::resolveAction(uint8_t type, uint16_t code) {
    int idx = findActionIndex(type, code);
    if (idx >= 0) {
        return actionCatalog()[static_cast<size_t>(idx)];
    }
    ActionInfo custom;
    custom.reportType = type;
    custom.usageCode = code;
    custom.shortLabel = QStringLiteral("0x%1").arg(code, 4, 16, QChar('0')).toUpper();
    custom.label = QStringLiteral("Custom (%1)").arg(custom.shortLabel);
    custom.icon = QString();
    return custom;
}

DeviceController::DeviceController(QObject *parent) : QObject(parent) {
    hid_init();

    // Setup periodic polling to auto-detect hardware
    connect(&m_pollTimer, &QTimer::timeout, this, &DeviceController::pollConnection);
    m_pollTimer.start(1000);
}

DeviceController::~DeviceController() {
    m_pollTimer.stop();
    disconnectDevice();
    hid_exit();
}

void DeviceController::updateStatus(const QString &text) {
    if (m_statusText != text) {
        m_statusText = text;
        emit statusChanged();
    }
}

void DeviceController::handleDeviceError(const QString &context) {
    qWarning() << "HID Device Error during:" << context;
    if (m_device) {
        hid_close(m_device);
        m_device = nullptr;
        emit connectionChanged();
    }
    m_userDisconnected = false; // Allow reconnection once the device re-attaches
    updateStatus(QStringLiteral("Device disconnected"));
}

void DeviceController::pollConnection() {
    if (!m_device) {
        if (!m_userDisconnected) {
            connectDevice();
        }
        return;
    }

    struct hid_device_info *devs = hid_enumerate(VID, PID);
    if (!devs) {
        handleDeviceError(QStringLiteral("device enumeration check"));
        return;
    }
    hid_free_enumeration(devs);
}

QVariantList DeviceController::availableActions() const {
    const auto& catalog = actionCatalog();
    QVariantList list;
    list.reserve(static_cast<int>(catalog.size()));

    for (const auto& a : catalog) {
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

QVariantList DeviceController::keys() const {
    QVariantList list;
    list.reserve(static_cast<int>(NumMacroKeys));

    for (size_t i = 0; i < NumMacroKeys; ++i) {
        const auto& k = m_keys[i];
        ActionInfo act = resolveAction(k.reportType, k.usageCode);
        int actionIdx = findActionIndex(k.reportType, k.usageCode);

        QVariantMap map;
        map[QStringLiteral("index")] = static_cast<int>(i);
        map[QStringLiteral("type")] = k.reportType;
        map[QStringLiteral("code")] = k.usageCode;
        map[QStringLiteral("label")] = act.label;
        map[QStringLiteral("shortLabel")] = act.shortLabel;
        map[QStringLiteral("icon")] = act.icon;
        map[QStringLiteral("isUnassigned")] = (k.usageCode == 0 || k.reportType == 0);
        map[QStringLiteral("actionIndex")] = (actionIdx >= 0 ? actionIdx : 0);
        list.append(map);
    }
    return list;
}

QStringList DeviceController::keyLabels() const {
    QStringList labels;
    labels.reserve(static_cast<int>(NumMacroKeys));
    for (size_t i = 0; i < NumMacroKeys; ++i) {
        labels.append(resolveAction(m_keys[i].reportType, m_keys[i].usageCode).shortLabel);
    }
    return labels;
}

QStringList DeviceController::displayModes() const {
    return {
        QStringLiteral("Hardware Monitor"),
        QStringLiteral("Profile Overview"),
        QStringLiteral("Custom Image")
    };
}

QStringList DeviceController::displayOrientations() const {
    return {
        QStringLiteral("Portrait (0°)"),
        QStringLiteral("Landscape (90°)"),
        QStringLiteral("Portrait Inverted (180°)"),
        QStringLiteral("Landscape Inverted (270°)")
    };
}

bool DeviceController::fetchDeviceConfig() {
    if (!m_device) return false;

    std::array<uint8_t, ConfigReportSize> raw{};
    raw[0] = ConfigReportId;

    int res = hid_get_feature_report(m_device, raw.data(), raw.size());
    if (res < 0) {
        handleDeviceError(QStringLiteral("fetchDeviceConfig"));
        return false;
    }

    const uint8_t *data = (raw[0] == ConfigReportId) ? &raw[1] : &raw[0];

    // 1. Parse 16 Keys (3 bytes each: type, usage_low, usage_high)
    size_t offset = 0;
    for (size_t i = 0; i < NumMacroKeys; ++i) {
        m_keys[i].reportType = data[offset];
        m_keys[i].usageCode = static_cast<uint16_t>(data[offset + 1]) | 
                              (static_cast<uint16_t>(data[offset + 2]) << 8);
        offset += 3;
    }
    emit keysChanged();

    // 2. Parse Display Settings
    m_displayMode = data[offset];
    m_displayOrientation = data[offset + 1];
    m_displayBrightness = data[offset + 2];
    emit displayConfigLoaded();

    updateStatus(QStringLiteral("Connected to Keypad"));
    return true;
}

bool DeviceController::connectDevice() {
    m_userDisconnected = false;
    if (m_device) return true;

    m_device = hid_open(VID, PID, nullptr);
    if (!m_device) {
        updateStatus(QStringLiteral("Disconnected"));
        return false;
    }

    // Set non-blocking mode so read calls never freeze Qt UI
    hid_set_nonblocking(m_device, 1);

    emit connectionChanged();
    fetchDeviceConfig();
    return true;
}

void DeviceController::disconnectDevice() {
    m_userDisconnected = true;
    if (m_device) {
        hid_close(m_device);
        m_device = nullptr;
        emit connectionChanged();
        updateStatus(QStringLiteral("Disconnected"));
    }
}

bool DeviceController::sendKeyConfig(int keyIndex, int reportType, int usageCode) {
    if (!m_device || keyIndex < 0 || static_cast<size_t>(keyIndex) >= NumMacroKeys) {
        return false;
    }

    std::array<uint8_t, ConfigReportSize> reportBuf{};
    reportBuf[0] = ConfigReportId;
    reportBuf[1] = static_cast<uint8_t>(keyIndex);
    reportBuf[2] = static_cast<uint8_t>(reportType);
    reportBuf[3] = static_cast<uint8_t>(usageCode & 0xFF);
    reportBuf[4] = static_cast<uint8_t>((usageCode >> 8) & 0xFF);

    if (hid_send_feature_report(m_device, reportBuf.data(), reportBuf.size()) < 0) {
        handleDeviceError(QStringLiteral("sendKeyConfig"));
        return false;
    }

    m_keys[static_cast<size_t>(keyIndex)].reportType = static_cast<uint8_t>(reportType);
    m_keys[static_cast<size_t>(keyIndex)].usageCode = static_cast<uint16_t>(usageCode);
    emit keysChanged();
    return true;
}

bool DeviceController::sendDisplayConfig(int mode, int orientation, int brightness) {
    if (!m_device) return false;

    std::array<uint8_t, ConfigReportSize> reportBuf{};
    reportBuf[0] = ConfigReportId;
    reportBuf[1] = static_cast<uint8_t>(Command::ConfigTargetDisplay);
    reportBuf[2] = static_cast<uint8_t>(mode);
    reportBuf[3] = static_cast<uint8_t>(orientation);
    reportBuf[4] = static_cast<uint8_t>(brightness);

    if (hid_send_feature_report(m_device, reportBuf.data(), reportBuf.size()) < 0) {
        handleDeviceError(QStringLiteral("sendDisplayConfig"));
        return false;
    }

    m_displayMode = mode;
    m_displayOrientation = orientation;
    m_displayBrightness = brightness;
    emit displayConfigLoaded();
    return true;
}

bool DeviceController::saveToFlash() {
    if (!m_device) return false;

    std::array<uint8_t, ConfigReportSize> reportBuf{};
    reportBuf[0] = ConfigReportId;
    reportBuf[1] = static_cast<uint8_t>(Command::SaveToFlash);

    if (hid_send_feature_report(m_device, reportBuf.data(), reportBuf.size()) < 0) {
        handleDeviceError(QStringLiteral("saveToFlash"));
        return false;
    }
    updateStatus(QStringLiteral("Configuration saved to flash"));
    return true;
}

bool DeviceController::rebootBootloader() {
    if (!m_device) return false;

    std::array<uint8_t, ConfigReportSize> reportBuf{};
    reportBuf[0] = ConfigReportId;
    reportBuf[1] = static_cast<uint8_t>(Command::Bootloader);

    hid_send_feature_report(m_device, reportBuf.data(), reportBuf.size());
    disconnectDevice();
    updateStatus(QStringLiteral("Rebooted to BOOTSEL"));
    return true;
}
