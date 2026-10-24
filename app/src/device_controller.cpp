#include "device_controller.hpp"
#include "actions.hpp"
#include <array>
#include <protocol.hpp>

using namespace zdc::protocol;

namespace {
constexpr uint8_t ConfigReportId = static_cast<uint8_t>(ReportId::ConfigFeature);
}

DeviceController::DeviceController(QObject* parent)
    : QObject(parent), m_transport(std::make_unique<HidTransport>(VID, PID, this)),
      m_keyModel(new KeyModel(NumMacroKeys, this)) {

    connect(m_transport.get(), &HidTransport::connected, this, [this]() {
        emit connectionChanged();
        fetchDeviceConfig();
    });

    connect(m_transport.get(), &HidTransport::disconnected, this, [this]() {
        emit connectionChanged();
        updateStatus(QStringLiteral("Disconnected"));
    });

    connect(m_transport.get(), &HidTransport::errorOccurred, this,
            [this](const QString&) { updateStatus(QStringLiteral("Device disconnected")); });

    connect(m_keyModel, &QAbstractItemModel::dataChanged, this, &DeviceController::keysChanged);
    connect(m_keyModel, &QAbstractItemModel::modelReset, this, &DeviceController::keysChanged);
}

void DeviceController::updateStatus(const QString& text) {
    if (m_statusText != text) {
        m_statusText = text;
        emit statusChanged();
    }
}

QVariantList DeviceController::availableActions() const {
    return Actions::toVariantList();
}

QStringList DeviceController::displayModes() const {
    return Actions::displayModes();
}

QStringList DeviceController::displayOrientations() const {
    return Actions::displayOrientations();
}

bool DeviceController::connectDevice() {
    return m_transport->openDevice();
}

void DeviceController::disconnectDevice() {
    m_transport->closeDevice(true);
}

bool DeviceController::fetchDeviceConfig() {
    if (!m_transport->isConnected())
        return false;

    std::array<uint8_t, ConfigReportSize> raw{};
    raw[0] = ConfigReportId;

    if (!m_transport->getFeatureReport(raw)) {
        return false;
    }

    const uint8_t* data = (raw[0] == ConfigReportId) ? &raw[1] : &raw[0];

    std::vector<KeyState> newKeys(NumMacroKeys);
    size_t offset = 0;
    for (size_t i = 0; i < NumMacroKeys; ++i) {
        newKeys[i].reportType = data[offset];
        newKeys[i].usageCode = static_cast<uint16_t>(data[offset + 1]) | (static_cast<uint16_t>(data[offset + 2]) << 8);
        offset += 3;
    }
    m_keyModel->resetKeys(newKeys);

    m_displayMode = data[offset];
    m_displayOrientation = data[offset + 1];
    m_displayBrightness = data[offset + 2];
    emit displayConfigLoaded();

    updateStatus(QStringLiteral("Connected to Keypad"));
    return true;
}

bool DeviceController::sendKeyConfig(int keyIndex, int reportType, int usageCode) {
    if (!m_transport->isConnected() || keyIndex < 0 || static_cast<size_t>(keyIndex) >= NumMacroKeys) {
        return false;
    }

    std::array<uint8_t, ConfigReportSize> reportBuf{};
    reportBuf[0] = ConfigReportId;
    reportBuf[1] = static_cast<uint8_t>(keyIndex);
    reportBuf[2] = static_cast<uint8_t>(reportType);
    reportBuf[3] = static_cast<uint8_t>(usageCode & 0xFF);
    reportBuf[4] = static_cast<uint8_t>((usageCode >> 8) & 0xFF);

    if (!m_transport->sendFeatureReport(reportBuf)) {
        return false;
    }

    m_keyModel->updateKey(static_cast<size_t>(keyIndex), static_cast<uint8_t>(reportType),
                          static_cast<uint16_t>(usageCode));
    return true;
}

bool DeviceController::sendDisplayConfig(int mode, int orientation, int brightness) {
    if (!m_transport->isConnected())
        return false;

    std::array<uint8_t, ConfigReportSize> reportBuf{};
    reportBuf[0] = ConfigReportId;
    reportBuf[1] = static_cast<uint8_t>(Command::ConfigTargetDisplay);
    reportBuf[2] = static_cast<uint8_t>(mode);
    reportBuf[3] = static_cast<uint8_t>(orientation);
    reportBuf[4] = static_cast<uint8_t>(brightness);

    if (!m_transport->sendFeatureReport(reportBuf)) {
        return false;
    }

    m_displayMode = mode;
    m_displayOrientation = orientation;
    m_displayBrightness = brightness;
    emit displayConfigLoaded();
    return true;
}

bool DeviceController::saveToFlash() {
    if (!m_transport->isConnected())
        return false;

    std::array<uint8_t, ConfigReportSize> reportBuf{};
    reportBuf[0] = ConfigReportId;
    reportBuf[1] = static_cast<uint8_t>(Command::SaveToFlash);

    if (!m_transport->sendFeatureReport(reportBuf)) {
        return false;
    }
    updateStatus(QStringLiteral("Configuration saved to flash"));
    return true;
}

bool DeviceController::rebootBootloader() {
    if (!m_transport->isConnected())
        return false;

    std::array<uint8_t, ConfigReportSize> reportBuf{};
    reportBuf[0] = ConfigReportId;
    reportBuf[1] = static_cast<uint8_t>(Command::Bootloader);

    m_transport->sendFeatureReport(reportBuf);
    disconnectDevice();
    updateStatus(QStringLiteral("Rebooted to BOOTSEL"));
    return true;
}