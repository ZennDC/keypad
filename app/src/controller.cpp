#include "controller.h"
#include <QDebug>
#include <hidapi/hidapi.h>

DeviceController::DeviceController(QObject *parent) : QObject(parent)
{
    hid_init();
}

DeviceController::~DeviceController()
{
    if (m_device)
    {
        hid_close(m_device);
        m_device = nullptr;
    }
    hid_exit();
}

void DeviceController::updateStatus(const QString &message)
{
    if (m_statusText != message)
    {
        m_statusText = message;
        emit statusChanged();
    }
}

bool DeviceController::connectDevice()
{
    if (m_device)
    {
        hid_close(m_device);
        m_device = nullptr;
    }

    m_device = hid_open(VID, PID, nullptr);

    if (m_device)
    {
        updateStatus(QStringLiteral("Connected to Keypad"));
    }
    else
    {
        updateStatus(QStringLiteral("Keypad not found. Check connection or udev rules."));
    }

    emit connectionChanged();
    return m_device != nullptr;
}

void DeviceController::disconnectDevice()
{
    if (!m_device)
        return;

    hid_close(m_device);
    m_device = nullptr;

    updateStatus(QStringLiteral("Disconnected"));
    emit connectionChanged();
}

bool DeviceController::sendKeyConfig(int keyIndex, int reportType, int usageCode) {
    if (!m_device) {
        updateStatus(QStringLiteral("Cannot send: Device disconnected"));
        return false;
    }

    if (keyIndex < 0 || keyIndex >= NUM_MACRO_KEYS) {
        updateStatus(QStringLiteral("Invalid key index"));
        return false;
    }

    // Exactly 64 bytes total: [Report ID, Key Index, Type, LSB, MSB, 0, 0, ...]
    uint8_t reportBuf[CONFIG_REPORT_SIZE] = {0};
    reportBuf[0] = CONFIG_REPORT_ID;
    reportBuf[1] = static_cast<uint8_t>(keyIndex);
    reportBuf[2] = static_cast<uint8_t>(reportType);
    reportBuf[3] = static_cast<uint8_t>(usageCode & 0xFF);
    reportBuf[4] = static_cast<uint8_t>((usageCode >> 8) & 0xFF);

    int res = hid_send_feature_report(m_device, reportBuf, sizeof(reportBuf));
    if (res < 0) {
        const wchar_t *err = hid_error(m_device);
        qWarning() << "HID Error:" << QString::fromWCharArray(err ? err : L"unknown");
        updateStatus(QStringLiteral("Failed to send key configuration"));
        return false;
    }

    updateStatus(QString("Key %1 updated: Usage 0x%2")
                     .arg(keyIndex + 1)
                     .arg(usageCode, 4, 16, QChar('0')));
    return true;
}

bool DeviceController::sendDisplayConfig(int mode, int orientation, int brightness) {
    if (!m_device) {
        updateStatus(QStringLiteral("Cannot send: Device disconnected"));
        return false;
    }

    // Exactly 64 bytes total: [Report ID, 0xFF, Mode, Orientation, Brightness, 0, ...]
    uint8_t reportBuf[CONFIG_REPORT_SIZE] = {0};
    reportBuf[0] = CONFIG_REPORT_ID;
    reportBuf[1] = CMD_CONFIG_TARGET_DISPLAY; // 0xFF
    reportBuf[2] = static_cast<uint8_t>(mode);
    reportBuf[3] = static_cast<uint8_t>(orientation);
    reportBuf[4] = static_cast<uint8_t>(brightness);

    int res = hid_send_feature_report(m_device, reportBuf, sizeof(reportBuf));
    if (res < 0) {
        const wchar_t *err = hid_error(m_device);
        qWarning() << "HID Error:" << QString::fromWCharArray(err ? err : L"unknown");
        updateStatus(QStringLiteral("Failed to send display configuration"));
        return false;
    }

    updateStatus(QString("Display updated: %1% brightness").arg(brightness));
    return true;
}

bool DeviceController::saveToFlash() {
    if (!m_device) {
        updateStatus(QStringLiteral("Cannot save: Device disconnected"));
        return false;
    }

    // Exactly 64 bytes total: [Report ID, 0xAA, 0, 0, ...]
    uint8_t reportBuf[CONFIG_REPORT_SIZE] = {0};
    reportBuf[0] = CONFIG_REPORT_ID;
    reportBuf[1] = CMD_SAVE_TO_FLASH; // 0xAA

    int res = hid_send_feature_report(m_device, reportBuf, sizeof(reportBuf));
    if (res < 0) {
        const wchar_t *err = hid_error(m_device);
        qWarning() << "HID Error:" << QString::fromWCharArray(err ? err : L"unknown");
        updateStatus(QStringLiteral("Failed to commit settings to flash"));
        return false;
    }

    updateStatus(QStringLiteral("Configuration saved to flash"));
    return true;
}