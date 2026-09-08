#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <hidapi/hidapi.h>

class DeviceController : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Backend)
    QML_SINGLETON

    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)

public:
    explicit DeviceController(QObject *parent = nullptr);
    ~DeviceController();

    bool isConnected() const { return m_device != nullptr; }
    QString statusText() const { return m_statusText; }

    Q_INVOKABLE bool connectDevice();
    Q_INVOKABLE bool disconnectDevice();
    
    Q_INVOKABLE bool sendConfig(int pin, int keycode);
    Q_INVOKABLE bool sendDisplayConfig(int mode, int rotation, int brightness);

    Q_INVOKABLE void handleKeyPress(int index);

signals:
    void connectionChanged();
    void statusChanged();

private:
    hid_device *m_device = nullptr;
    QString m_statusText = "Disconnected";

    const uint16_t VID = 0xCafe;
    const uint16_t PID = 0x4000;
    const uint8_t REPORT_ID = 3;
};