#pragma once
#include "ClassParser.h"
#include <QString>

class ProbeGenerator {
public:
    QString generate(const QString &originalCode,
                     const QVector<RawClassInfo> &classes);
};
