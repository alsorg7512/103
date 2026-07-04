#pragma once
#include <QSet>
#include "MemoryModel.h"
#include <QString>
#include <QVector>

struct RawClassInfo {
    string name;
    string parentName;
    vector<BaseClassInfo> baseClasses;
    vector<FieldInfo> fields;
    vector<FunctionInfo> functions;
};

class ClassParser {
public:
    QVector<RawClassInfo> parse(const QString &code);
};
