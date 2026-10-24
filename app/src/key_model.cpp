#include "key_model.hpp"
#include "actions.hpp"

KeyModel::KeyModel(size_t keyCount, QObject* parent) : QAbstractListModel(parent), m_keys(keyCount) {}

int KeyModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_keys.size());
}

QVariant KeyModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || static_cast<size_t>(index.row()) >= m_keys.size()) {
        return {};
    }

    const auto& k = m_keys[static_cast<size_t>(index.row())];
    ActionInfo act = Actions::resolve(k.reportType, k.usageCode);

    switch (role) {
    case IndexRole:
        return index.row();
    case TypeRole:
        return k.reportType;
    case CodeRole:
        return k.usageCode;
    case LabelRole:
        return act.label;
    case ShortLabelRole:
        return act.shortLabel;
    case IconRole:
        return act.icon;
    case IsUnassignedRole:
        return (k.usageCode == 0 || k.reportType == 0);
    case ActionIndexRole: {
        int idx = Actions::findIndex(k.reportType, k.usageCode);
        return idx >= 0 ? idx : 0;
    }
    default:
        return {};
    }
}

QHash<int, QByteArray> KeyModel::roleNames() const {
    return {{IndexRole, "index"},
            {TypeRole, "type"},
            {CodeRole, "code"},
            {LabelRole, "label"},
            {ShortLabelRole, "shortLabel"},
            {IconRole, "icon"},
            {IsUnassignedRole, "isUnassigned"},
            {ActionIndexRole, "actionIndex"}};
}

int KeyModel::actionIndex(int keyIndex) const {
    if (keyIndex < 0 || static_cast<size_t>(keyIndex) >= m_keys.size()) {
        return 0;
    }
    int idx = Actions::findIndex(m_keys[static_cast<size_t>(keyIndex)].reportType,
                                 m_keys[static_cast<size_t>(keyIndex)].usageCode);
    return idx >= 0 ? idx : 0;
}

void KeyModel::updateKey(size_t index, uint8_t reportType, uint16_t usageCode) {
    if (index >= m_keys.size())
        return;
    m_keys[index].reportType = reportType;
    m_keys[index].usageCode = usageCode;
    QModelIndex modelIdx = createIndex(static_cast<int>(index), 0);
    emit dataChanged(modelIdx, modelIdx);
}

void KeyModel::resetKeys(const std::vector<KeyState>& keys) {
    beginResetModel();
    m_keys = keys;
    endResetModel();
}

QStringList KeyModel::labels() const {
    QStringList out;
    out.reserve(static_cast<int>(m_keys.size()));
    for (const auto& k : m_keys) {
        out.append(Actions::resolve(k.reportType, k.usageCode).shortLabel);
    }
    return out;
}