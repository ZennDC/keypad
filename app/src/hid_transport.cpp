#include "hid_transport.hpp"
#include <hidapi/hidapi.h>
#include <QDebug>

HidTransport::HidTransport(uint16_t vid, uint16_t pid, QObject *parent)
    : QObject(parent), m_vid(vid), m_pid(pid) {
    hid_init();
    connect(&m_pollTimer, &QTimer::timeout, this, &HidTransport::poll);
    m_pollTimer.start(1000);
}

HidTransport::~HidTransport() {
    m_pollTimer.stop();
    closeDevice(true);
    hid_exit();
}

bool HidTransport::openDevice() {
    m_userDisconnected = false;
    if (m_device) return true;

    m_device = hid_open(m_vid, m_pid, nullptr);
    if (!m_device) {
        return false;
    }

    hid_set_nonblocking(m_device, 1);
    emit connected();
    return true;
}

void HidTransport::closeDevice(bool userInitiated) {
    if (userInitiated) {
        m_userDisconnected = true;
    }
    if (m_device) {
        hid_close(m_device);
        m_device = nullptr;
        emit disconnected();
    }
}

bool HidTransport::sendFeatureReport(std::span<const uint8_t> buffer) {
    if (!m_device || buffer.empty()) return false;

    int res = hid_send_feature_report(m_device, buffer.data(), buffer.size());
    if (res < 0) {
        handleDeviceError(QStringLiteral("sendFeatureReport"));
        return false;
    }
    return true;
}

bool HidTransport::getFeatureReport(std::span<uint8_t> buffer) {
    if (!m_device || buffer.empty()) return false;

    int res = hid_get_feature_report(m_device, buffer.data(), buffer.size());
    if (res < 0) {
        handleDeviceError(QStringLiteral("getFeatureReport"));
        return false;
    }
    return true;
}

void HidTransport::poll() {
    if (!m_device) {
        if (!m_userDisconnected) {
            openDevice();
        }
        return;
    }

    struct hid_device_info *devs = hid_enumerate(m_vid, m_pid);
    if (!devs) {
        handleDeviceError(QStringLiteral("device enumeration check"));
        return;
    }
    hid_free_enumeration(devs);
}

void HidTransport::handleDeviceError(const QString &context) {
    qWarning() << "HID Transport Error during:" << context;
    if (m_device) {
        hid_close(m_device);
        m_device = nullptr;
        emit disconnected();
    }
    m_userDisconnected = false;
    emit errorOccurred(context);
}