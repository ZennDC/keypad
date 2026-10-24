#pragma once

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <cstdint>
#include <vector>

struct ActionInfo {
    QString label;
    QString shortLabel;
    QString icon;
    uint8_t reportType = 0;
    uint16_t usageCode = 0;
};

namespace Actions {
    const std::vector<ActionInfo>& catalog();
    int findIndex(uint8_t reportType, uint16_t usageCode);
    ActionInfo resolve(uint8_t reportType, uint16_t usageCode);
    QVariantList toVariantList();

    const QStringList& displayModes();
    const QStringList& displayOrientations();
}