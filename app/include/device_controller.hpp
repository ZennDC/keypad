#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <memory>

#include "hid_transport.hpp"
#include "key_model.hpp"

class DeviceController : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Backend)
    QML_SINGLETON

    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(KeyModel* keyModel READ keyModel CONSTANT)
    Q_PROPERTY(QStringList keyLabels READ keyLabels NOTIFY keysChanged)
    Q_PROPERTY(QVariantList availableActions READ availableActions CONSTANT)
    Q_PROPERTY(QStringList displayModes READ displayModes CONSTANT)
    Q_PROPERTY(QStringList displayOrientations READ displayOrientations CONSTANT)
    Q_PROPERTY(int displayMode READ displayMode NOTIFY displayConfigLoaded)
    Q_PROPERTY(int displayOrientation READ displayOrientation NOTIFY displayConfigLoaded)
    Q_PROPERTY(int displayBrightness READ displayBrightness NOTIFY displayConfigLoaded)

public:
    explicit DeviceController(QObject *parent = nullptr);

    [[nodiscard]] bool isConnected() const noexcept { return m_transport->isConnected(); }
    [[nodiscard]] QString statusText() const { return m_statusText; }
    [[nodiscard]] KeyModel* keyModel() const noexcept { return m_keyModel; }
    [[nodiscard]] QStringList keyLabels() const { return m_keyModel->labels(); }
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

private:
    void updateStatus(const QString &text);

    std::unique_ptr<HidTransport> m_transport;
    KeyModel *m_keyModel = nullptr;

    QString m_statusText = QStringLiteral("Disconnected");
    int m_displayMode = 0;
    int m_displayOrientation = 0;
    int m_displayBrightness = 80;

    static constexpr uint16_t VID = 0xCafe;
    static constexpr uint16_t PID = 0x4000;
};