#include "controller.h"
#include "protocol.h"
#include <hidapi/hidapi.h>
#include <QDebug>
#include <array>

static constexpr uint16_t VID = 0xCafe;
static constexpr uint16_t PID = 0x4000;
static constexpr uint8_t  CONFIG_REPORT_ID = REPORT_ID_CONFIG_FEATURE; // 3

DeviceController::DeviceController(QObject *parent) : QObject(parent) {
    hid_init();
    for (int i = 0; i < NUM_MACRO_KEYS; ++i) {
        m_keyLabels.append(QStringLiteral("None"));
    }
}

DeviceController::~DeviceController() {
    disconnectDevice();
    hid_exit();
}

void DeviceController::updateStatus(const QString &text) {
    m_statusText = text;
    emit statusChanged();
}

QString DeviceController::lookupActionLabel(uint8_t type, uint16_t code) {
    if (type == 0x00) return QStringLiteral("Disabled");

    if (type == 0x02) { // Consumer
        switch (code) {
            case 0x00E2: return QStringLiteral("Mute");
            case 0x00E9: return QStringLiteral("Vol +");
            case 0x00EA: return QStringLiteral("Vol -");
            case 0x00CD: return QStringLiteral("Play/Pause");
            case 0x00B5: return QStringLiteral("Next");
            case 0x00B6: return QStringLiteral("Prev");
            default: break;
        }
    } else if (type == 0x01) { // Keyboard
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
    return QString("0x%1").arg(code, 4, 16, QChar('0')).toUpper();
}

bool DeviceController::fetchDeviceConfig() {
    if (!m_device) return false;

    // Buffer: Report ID prefix + 63 bytes payload
    std::array<uint8_t, CONFIG_REPORT_SIZE> raw{};
    raw[0] = CONFIG_REPORT_ID;

    int res = hid_get_feature_report(m_device, raw.data(), raw.size());
    if (res < 0) {
        const wchar_t *err = hid_error(m_device);
        qWarning() << "HID fetch error:" << QString::fromWCharArray(err ? err : L"unknown");
        return false;
    }

    // Determine payload start depending on whether hidapi returned report_id at raw[0]
    const uint8_t *data = (raw[0] == CONFIG_REPORT_ID) ? &raw[1] : &raw[0];

    // 1. Parse 16 Keys (Bytes 0 - 47)
    m_keyLabels.clear();
    int offset = 0;
    for (int i = 0; i < NUM_MACRO_KEYS; ++i) {
        uint8_t type = data[offset];
        uint16_t usage = static_cast<uint16_t>(data[offset + 1]) | 
                         (static_cast<uint16_t>(data[offset + 2]) << 8);
        offset += 3;
        m_keyLabels.append(lookupActionLabel(type, usage));
    }
    emit keyLabelsChanged();

    // 2. Parse Display Settings (Bytes 48 - 50)
    m_displayMode = data[offset];
    m_displayOrientation = data[offset + 1];
    m_displayBrightness = data[offset + 2];
    emit displayConfigLoaded();

    updateStatus(QStringLiteral("Connected to Keypad (Config Synced)"));
    return true;
}

bool DeviceController::connectDevice() {
    if (m_device) return true;

    m_device = hid_open(VID, PID, nullptr);
    if (!m_device) {
        updateStatus(QStringLiteral("Device not found"));
        return false;
    }

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
    if (!m_device || keyIndex < 0 || keyIndex >= NUM_MACRO_KEYS) return false;

    uint8_t reportBuf[CONFIG_REPORT_SIZE] = {0};
    reportBuf[0] = CONFIG_REPORT_ID;
    reportBuf[1] = static_cast<uint8_t>(keyIndex);
    reportBuf[2] = static_cast<uint8_t>(reportType);
    reportBuf[3] = static_cast<uint8_t>(usageCode & 0xFF);
    reportBuf[4] = static_cast<uint8_t>((usageCode >> 8) & 0xFF);

    if (hid_send_feature_report(m_device, reportBuf, sizeof(reportBuf)) < 0) {
        return false;
    }

    // Update local label immediately
    if (keyIndex < m_keyLabels.size()) {
        m_keyLabels[keyIndex] = lookupActionLabel(reportType, usageCode);
        emit keyLabelsChanged();
    }
    return true;
}

bool DeviceController::sendDisplayConfig(int mode, int orientation, int brightness) {
    if (!m_device) return false;

    uint8_t reportBuf[CONFIG_REPORT_SIZE] = {0};
    reportBuf[0] = CONFIG_REPORT_ID;
    reportBuf[1] = CMD_CONFIG_TARGET_DISPLAY;
    reportBuf[2] = static_cast<uint8_t>(mode);
    reportBuf[3] = static_cast<uint8_t>(orientation);
    reportBuf[4] = static_cast<uint8_t>(brightness);

    return hid_send_feature_report(m_device, reportBuf, sizeof(reportBuf)) >= 0;
}

bool DeviceController::saveToFlash() {
    if (!m_device) return false;

    uint8_t reportBuf[CONFIG_REPORT_SIZE] = {0};
    reportBuf[0] = CONFIG_REPORT_ID;
    reportBuf[1] = CMD_SAVE_TO_FLASH;

    return hid_send_feature_report(m_device, reportBuf, sizeof(reportBuf)) >= 0;
}