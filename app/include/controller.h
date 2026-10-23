#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>
#include "protocol.h"

// Forward declare to avoid leaking C HIDAPI headers into QML translation units
struct hid_device_;
typedef struct hid_device_ hid_device;

class DeviceController : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Backend)
    QML_SINGLETON

    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QStringList keyLabels READ keyLabels NOTIFY keyLabelsChanged)
    Q_PROPERTY(int displayMode READ displayMode NOTIFY displayConfigLoaded)
    Q_PROPERTY(int displayOrientation READ displayOrientation NOTIFY displayConfigLoaded)
    Q_PROPERTY(int displayBrightness READ displayBrightness NOTIFY displayConfigLoaded)

public:
    explicit DeviceController(QObject *parent = nullptr);
    ~DeviceController() override;

    bool isConnected() const noexcept { return m_device != nullptr; }
    QString statusText() const { return m_statusText; }
    QStringList keyLabels() const { return m_keyLabels; }
    int displayMode() const noexcept { return m_displayMode; }
    int displayOrientation() const noexcept { return m_displayOrientation; }
    int displayBrightness() const noexcept { return m_displayBrightness; }

    Q_INVOKABLE bool connectDevice();
    Q_INVOKABLE void disconnectDevice();
    Q_INVOKABLE bool fetchDeviceConfig();

    // Configuration commands
    Q_INVOKABLE bool sendKeyConfig(int keyIndex, int reportType, int usageCode);
    Q_INVOKABLE bool sendDisplayConfig(int mode, int orientation, int brightness);
    Q_INVOKABLE bool saveToFlash();

signals:
    void connectionChanged();
    void statusChanged();
    void keyLabelsChanged();
    void displayConfigLoaded();

private:
    void updateStatus(const QString &message);
    QString lookupActionLabel(uint8_t type, uint16_t code);

    hid_device *m_device = nullptr;
    QString m_statusText = QStringLiteral("Disconnected");
    QStringList m_keyLabels;
    int m_displayMode = 0;
    int m_displayOrientation = 0;
    int m_displayBrightness = 80;

    // USB Identifiers matching firmware/src/usb_descriptors.c
    static constexpr uint16_t VID = 0xCafe;
    static constexpr uint16_t PID = 0x4000;
    static constexpr uint8_t  CONFIG_REPORT_ID = REPORT_ID_CONFIG_FEATURE;
};