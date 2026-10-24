#include "controller.hpp"
#include <hidapi/hidapi.h>
#include <QDebug>
#include <array>

using namespace zdc::protocol;

namespace {
constexpr uint8_t ConfigReportId = static_cast<uint8_t>(ReportId::ConfigFeature);
}

DeviceController::DeviceController(QObject *parent) : QObject(parent) {
    hid_init();
    m_keyLabels.reserve(NumMacroKeys);
    for (size_t i = 0; i < NumMacroKeys; ++i) {
        m_keyLabels.append(QStringLiteral("None"));
    }

    // Check device presence every 1000ms
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
    disconnectDevice();
    updateStatus(QStringLiteral("Device disconnected"));
}

void DeviceController::pollConnection() {
    if (!m_device) {
        // Automatically attempt to reconnect if not connected
        connectDevice();
        return;
    }

    // Verify if the device still enumerates on the host bus
    struct hid_device_info *devs = hid_enumerate(VID, PID);
    if (!devs) {
        handleDeviceError(QStringLiteral("device enumeration check"));
        return;
    }
    hid_free_enumeration(devs);
}

QString DeviceController::lookupActionLabel(ReportType type, uint16_t code) {
    if (code == 0x0000 || type == static_cast<ReportType>(0)) {
        return QStringLiteral("None");
    }

    if (type == ReportType::Consumer) {
        switch (code) {
            case 0x00E2: return QStringLiteral("Mute");
            case 0x00E9: return QStringLiteral("Vol +");
            case 0x00EA: return QStringLiteral("Vol -");
            case 0x00CD: return QStringLiteral("Play/Pause");
            case 0x00B5: return QStringLiteral("Next");
            case 0x00B6: return QStringLiteral("Prev");
            default: break;
        }
    } else if (type == ReportType::Keyboard) {
        switch (code) {
            case 0x28: return QStringLiteral("Enter");
            case 0x29: return QStringLiteral("Escape");
            case 0x2B: return QStringLiteral("Tab");
            case 0x2C: return QStringLiteral("Space");
            case 0x06: return QStringLiteral("C");
            case 0x19: return QStringLiteral("V");
            case 0x1D: return QStringLiteral("Z");
            case 0x68: return QStringLiteral("F13");
            case 0x69: return QStringLiteral("F14");
            case 0x6A: return QStringLiteral("F15");
            case 0x6B: return QStringLiteral("F16");
            default: break;
        }
    }
    return QStringLiteral("0x%1").arg(code, 4, 16, QChar('0')).toUpper();
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

    // 1. Parse 16 Keys (3 bytes: type, usage_low, usage_high)
    m_keyLabels.clear();
    size_t offset = 0;
    for (size_t i = 0; i < NumMacroKeys; ++i) {
        auto type = static_cast<ReportType>(data[offset]);
        uint16_t usage = static_cast<uint16_t>(data[offset + 1]) | 
                         (static_cast<uint16_t>(data[offset + 2]) << 8);
        offset += 3;
        m_keyLabels.append(lookupActionLabel(type, usage));
    }
    emit keyLabelsChanged();

    // 2. Parse Display Settings
    m_displayMode = data[offset];
    m_displayOrientation = data[offset + 1];
    m_displayBrightness = data[offset + 2];
    emit displayConfigLoaded();

    updateStatus(QStringLiteral("Connected to Keypad"));
    return true;
}

bool DeviceController::connectDevice() {
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

    if (keyIndex < m_keyLabels.size()) {
        m_keyLabels[keyIndex] = lookupActionLabel(static_cast<ReportType>(reportType), static_cast<uint16_t>(usageCode));
        emit keyLabelsChanged();
    }
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