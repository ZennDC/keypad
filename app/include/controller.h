#pragma once

#include <QObject>
#include <QString>
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

public:
    explicit DeviceController(QObject *parent = nullptr);
    ~DeviceController() override;

    bool isConnected() const noexcept { return m_device != nullptr; }
    QString statusText() const { return m_statusText; }

    Q_INVOKABLE bool connectDevice();
    Q_INVOKABLE void disconnectDevice();
    
    // Configuration commands
    Q_INVOKABLE bool sendKeyConfig(int keyIndex, int reportType, int usageCode);
    Q_INVOKABLE bool sendDisplayConfig(int mode, int orientation, int brightness);
    Q_INVOKABLE bool saveToFlash();

signals:
    void connectionChanged();
    void statusChanged();

private:
    void updateStatus(const QString &message);

    hid_device *m_device = nullptr;
    QString m_statusText = QStringLiteral("Disconnected");

    // USB Identifiers matching firmware/src/usb_descriptors.c
    static constexpr uint16_t VID = 0xCafe;
    static constexpr uint16_t PID = 0x4000;
    static constexpr uint8_t  CONFIG_REPORT_ID = REPORT_ID_CONFIG_FEATURE;
};