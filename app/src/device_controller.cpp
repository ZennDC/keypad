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

QStringList DeviceController::profiles() const {
    return {QStringLiteral("Custom"), QStringLiteral("Numpad"), QStringLiteral("Media"),
            QStringLiteral("Navigation")};
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

bool DeviceController::selectProfile(int profileIndex) {
    if (profileIndex < 0 || profileIndex >= 4) {
        return false;
    }

    m_currentProfile = profileIndex;
    emit currentProfileChanged();

    if (m_transport->isConnected()) {
        std::array<uint8_t, ConfigReportSize> reportBuf{};
        reportBuf[0] = ConfigReportId;
        reportBuf[1] = static_cast<uint8_t>(Command::SelectProfile);
        reportBuf[2] = static_cast<uint8_t>(profileIndex);

        if (!m_transport->sendFeatureReport(reportBuf)) {
            return false;
        }

        // Fetch the keys for the newly selected profile from firmware
        fetchDeviceConfig();
    } else {
        // Fallback offline preview: reload presets matching firmware definitions
        static const std::array<KeyState, NumMacroKeys> NumpadPreset = {{
            {1, 0x5F}, {1, 0x60}, {1, 0x61}, {1, 0x54},
            {1, 0x5C}, {1, 0x5D}, {1, 0x5E}, {1, 0x55},
            {1, 0x59}, {1, 0x5A}, {1, 0x5B}, {1, 0x56},
            {1, 0x62}, {1, 0x63}, {1, 0x58}, {1, 0x57}
        }};

        static const std::array<KeyState, NumMacroKeys> MediaPreset = {{
            {2, 0x00E2}, {2, 0x00EA}, {2, 0x00E9}, {2, 0x00CD},
            {2, 0x00B6}, {2, 0x00B5}, {2, 0x00B7}, {2, 0x00B3},
            {2, 0x00B4}, {2, 0x0192}, {2, 0x0194}, {2, 0x0223},
            {2, 0x0070}, {2, 0x006F}, {2, 0x0032}, {2, 0x00B8}
        }};

        static const std::array<KeyState, NumMacroKeys> NavPreset = {{
            {1, 0x29}, {1, 0x4A}, {1, 0x52}, {1, 0x4B},
            {1, 0x2B}, {1, 0x50}, {1, 0x51}, {1, 0x4F},
            {1, 0x2A}, {1, 0x4D}, {1, 0x28}, {1, 0x4E},
            {1, 0x68}, {1, 0x69}, {1, 0x6A}, {1, 0x4C}
        }};

        static const std::array<KeyState, NumMacroKeys> CustomPreset = {{
            {2, 0x00E9}, {2, 0x00EA}, {2, 0x00CD}, {2, 0x00B5},
            {2, 0x00B6}, {1, 0x28},   {1, 0x29},   {1, 0x2B},
            {1, 0x2C},   {1, 0x06},   {1, 0x19},   {1, 0x1D},
            {1, 0x68},   {1, 0x69},   {1, 0x6A},   {1, 0x6B}
        }};

        const KeyState* preset = nullptr;
        switch (profileIndex) {
        case 1:
            preset = NumpadPreset.data();
            break;
        case 2:
            preset = MediaPreset.data();
            break;
        case 3:
            preset = NavPreset.data();
            break;
        default:
            preset = CustomPreset.data();
            break;
        }

        std::vector<KeyState> newKeys(preset, preset + NumMacroKeys);
        m_keyModel->resetKeys(newKeys);
    }

    const QStringList profList = profiles();
    if (profileIndex < profList.size()) {
        updateStatus(QStringLiteral("Selected Profile: %1").arg(profList[profileIndex]));
    }
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