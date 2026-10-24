#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <array>
#include <vector>

#include "protocol.hpp"

struct hid_device_;
typedef struct hid_device_ hid_device;

struct ActionInfo {
    QString label;        // Full action description (e.g., "Volume Up", "Key: Enter")
    QString shortLabel;   // Compact badge label (e.g., "Vol +", "Enter")
    QString icon;         // Resource SVG path
    int reportType = 0;   // 0: None, 1: Keyboard, 2: Consumer
    uint16_t usageCode = 0; // HID usage code
};

struct KeyState {
    uint8_t reportType = 0;
    uint16_t usageCode = 0;
};

class DeviceController : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Backend)
    QML_SINGLETON

    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QVariantList keys READ keys NOTIFY keysChanged)
    Q_PROPERTY(QStringList keyLabels READ keyLabels NOTIFY keysChanged)
    Q_PROPERTY(QVariantList availableActions READ availableActions CONSTANT)
    Q_PROPERTY(QStringList displayModes READ displayModes CONSTANT)
    Q_PROPERTY(QStringList displayOrientations READ displayOrientations CONSTANT)
    Q_PROPERTY(int displayMode READ displayMode NOTIFY displayConfigLoaded)
    Q_PROPERTY(int displayOrientation READ displayOrientation NOTIFY displayConfigLoaded)
    Q_PROPERTY(int displayBrightness READ displayBrightness NOTIFY displayConfigLoaded)

public:
    explicit DeviceController(QObject *parent = nullptr);
    ~DeviceController() override;

    [[nodiscard]] bool isConnected() const noexcept { return m_device != nullptr; }
    [[nodiscard]] QString statusText() const { return m_statusText; }
    [[nodiscard]] QVariantList keys() const;
    [[nodiscard]] QStringList keyLabels() const;
    [[nodiscard]] QVariantList availableActions() const;
    [[nodiscard]] QStringList displayModes() const;
    [[nodiscard]] QStringList displayOrientations() const;
    [[nodiscard]] int displayMode() const noexcept { return m_displayMode; }
    [[nodiscard]] int displayOrientation() const noexcept { return m_displayOrientation; }
    [[nodiscard]] int displayBrightness() const noexcept { return m_displayBrightness; }

    Q_INVOKABLE bool connectDevice();
    Q_INVOKABLE void disconnectDevice();
    Q_INVOKABLE bool fetchDeviceConfig();

    Q_INVOKABLE bool sendKeyConfig(int keyIndex, int reportType, int usageCode);
    Q_INVOKABLE bool sendDisplayConfig(int mode, int orientation, int brightness);
    Q_INVOKABLE bool saveToFlash();
    Q_INVOKABLE bool rebootBootloader();

signals:
    void connectionChanged();
    void statusChanged();
    void keysChanged();
    void displayConfigLoaded();

private slots:
    void pollConnection();

private:
    void updateStatus(const QString &message);
    void handleDeviceError(const QString &context);

    static const std::vector<ActionInfo>& actionCatalog();
    static ActionInfo resolveAction(uint8_t type, uint16_t code);
    static int findActionIndex(uint8_t type, uint16_t code);

    hid_device *m_device = nullptr;
    QTimer m_pollTimer;
    QString m_statusText = QStringLiteral("Disconnected");
    bool m_userDisconnected = false;

    std::array<KeyState, zdc::protocol::NumMacroKeys> m_keys{};
    int m_displayMode = 0;
    int m_displayOrientation = 0;
    int m_displayBrightness = 80;

    static constexpr uint16_t VID = 0xCafe;
    static constexpr uint16_t PID = 0x4000;
};
