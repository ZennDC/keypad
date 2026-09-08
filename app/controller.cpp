#include "controller.h"
#include <vector>

#include <iostream>

DeviceController::DeviceController(QObject *parent) : QObject(parent)
{
    hid_init();
}

DeviceController::~DeviceController()
{
    if (m_device)
    {
        hid_close(m_device);
    }
    hid_exit();
}

bool DeviceController::connectDevice()
{
    if (m_device)
    {
        hid_close(m_device);
        m_device = nullptr;
    }

    // Open Pico by VID/PID
    m_device = hid_open(VID, PID, nullptr);

    if (m_device)
    {
        m_statusText = "Connected to Pico";
    }
    else
    {
        m_statusText = "Pico not found. Check connection/permissions.";
    }

    emit connectionChanged();
    emit statusChanged();
    return m_device != nullptr;
}

bool DeviceController::disconnectDevice()
{
    if (!m_device)
    {
        return false;
    }

    hid_close(m_device);
    m_device = nullptr;

    m_statusText = "Disconnected";

    emit connectionChanged();
    emit statusChanged();
    return true;
}

bool DeviceController::sendConfig(int keyIndex, int keycode)
{
    if (!m_device)
    {
        m_statusText = "Cannot send: Device disconnected";
        emit statusChanged();
        return false;
    }

    // Report ID (1 byte) + 64 bytes payload = 65 bytes
    uint8_t reportBuf[65] = {0};
    reportBuf[0] = REPORT_ID; // 0x03
    reportBuf[1] = static_cast<uint8_t>(keyIndex);
    reportBuf[2] = (keycode >= 0xE000) ? 0x02 : 0x01; // 0x02: Consumer/Media, 0x01: Keyboard
    reportBuf[3] = static_cast<uint8_t>(keycode & 0xFF);

    int res = hid_send_feature_report(m_device, reportBuf, sizeof(reportBuf));
    if (res < 0)
    {
        m_statusText = "Failed to send configuration";
        emit statusChanged();
        return false;
    }

    m_statusText = QString("Key %1 configured (0x%2)").arg(keyIndex + 1).arg(keycode, 2, 16, QChar('0'));
    emit statusChanged();
    return true;
}

bool DeviceController::sendDisplayConfig(int mode, int rotation, int brightness)
{
    if (!m_device) return false;

    uint8_t reportBuf[65] = {0};
    reportBuf[0] = REPORT_ID; // 0x03
    reportBuf[1] = 0xFF;      // Reserve 0xFF index to identify screen config packets
    reportBuf[2] = static_cast<uint8_t>(mode);
    reportBuf[3] = static_cast<uint8_t>(rotation);
    reportBuf[4] = static_cast<uint8_t>(brightness);

    int res = hid_send_feature_report(m_device, reportBuf, sizeof(reportBuf));
    if (res < 0) {
        m_statusText = "Failed to send display config";
        emit statusChanged();
        return false;
    }

    m_statusText = QString("ST7789 updated: %1% brightness").arg(brightness);
    emit statusChanged();
    return true;
}

void DeviceController::handleKeyPress(int index)
{
    std::cout << index << std::endl;
}
