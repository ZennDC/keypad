#pragma once

#include <QObject>
#include <QTimer>
#include <cstdint>
#include <span>

struct hid_device_;
typedef struct hid_device_ hid_device;

class HidTransport : public QObject {
    Q_OBJECT

public:
    explicit HidTransport(uint16_t vid, uint16_t pid, QObject* parent = nullptr);
    ~HidTransport() override;

    [[nodiscard]] bool isConnected() const noexcept { return m_device != nullptr; }

    bool openDevice();
    void closeDevice(bool userInitiated = false);

    bool sendFeatureReport(std::span<const uint8_t> buffer);
    bool getFeatureReport(std::span<uint8_t> buffer);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& context);

private slots:
    void poll();

private:
    void handleDeviceError(const QString& context);

    uint16_t m_vid;
    uint16_t m_pid;
    hid_device* m_device = nullptr;
    QTimer m_pollTimer;
    bool m_userDisconnected = false;
};