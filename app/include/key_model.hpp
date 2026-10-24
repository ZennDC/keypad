#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include <cstdint>
#include <vector>

struct KeyState {
    uint8_t reportType = 0;
    uint16_t usageCode = 0;
};

class KeyModel : public QAbstractListModel {
    Q_OBJECT
    QML_UNCREATABLE("KeyModel cannot be instantiated directly from QML. Access it via Backend.keyModel.")

public:
    enum KeyRoles {
        IndexRole = Qt::UserRole + 1,
        TypeRole,
        CodeRole,
        LabelRole,
        ShortLabelRole,
        IconRole,
        IsUnassignedRole,
        ActionIndexRole
    };
    Q_ENUM(KeyRoles)

    explicit KeyModel(size_t keyCount, QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE [[nodiscard]] int actionIndex(int keyIndex) const;

    void updateKey(size_t index, uint8_t reportType, uint16_t usageCode);
    void resetKeys(const std::vector<KeyState>& keys);
    [[nodiscard]] const std::vector<KeyState>& keys() const noexcept { return m_keys; }
    [[nodiscard]] QStringList labels() const;

private:
    std::vector<KeyState> m_keys;
};