#pragma once

#include "ClassParser.h"
#include <QVector>
#include <QString>

class LayoutEngine {
public:
    QVector<ClassLayout> run(const QString &originalCode,
                             const QVector<RawClassInfo> &classes);

    // 最近一次探针编译/运行失败的详细错误信息，供 UI 显示。
    QString lastError() const;

private:
    QString compileAndRun(const QString &probeCode);
    QVector<ClassLayout> parseOutput(const QString &output,
                                      const QVector<RawClassInfo> &classes);

    QString m_lastError;
};
