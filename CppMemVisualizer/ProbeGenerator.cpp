#include "ProbeGenerator.h"
#include <QRegularExpression>
#include <QStringList>

namespace {

bool isMainSignatureBeforeBrace(const QString &prefix) {
    QString trimmed = prefix.trimmed();
    return trimmed.contains(QRegularExpression(
        R"((^|[\s:*&])main\s*\([^;{}]*\)\s*$)"
        ));
}

int findMatchingBrace(const QString &code, int openPos) {
    int depth = 0;
    bool inString = false;
    bool inChar = false;
    bool escaped = false;
    bool inLineComment = false;
    bool inBlockComment = false;

    for (int i = openPos; i < code.size(); ++i) {
        QChar ch = code[i];
        QChar next = (i + 1 < code.size()) ? code[i + 1] : QChar();

        if (inLineComment) {
            if (ch == '\n') inLineComment = false;
            continue;
        }
        if (inBlockComment) {
            if (ch == '*' && next == '/') {
                inBlockComment = false;
                ++i;
            }
            continue;
        }
        if (escaped) {
            escaped = false;
            continue;
        }
        if (ch == '\\' && (inString || inChar)) {
            escaped = true;
            continue;
        }
        if (ch == '"' && !inChar) {
            inString = !inString;
            continue;
        }
        if (ch == '\'' && !inString) {
            inChar = !inChar;
            continue;
        }
        if (inString || inChar) continue;

        if (ch == '/' && next == '/') {
            inLineComment = true;
            ++i;
            continue;
        }
        if (ch == '/' && next == '*') {
            inBlockComment = true;
            ++i;
            continue;
        }

        if (ch == '{') {
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0) return i;
        }
    }

    return -1;
}

QString removeTopLevelMainFunction(const QString &code) {
    QString result = code;

    int braceDepth = 0;
    int segmentStart = 0;
    bool inString = false;
    bool inChar = false;
    bool escaped = false;
    bool inLineComment = false;
    bool inBlockComment = false;

    for (int i = 0; i < code.size(); ++i) {
        QChar ch = code[i];
        QChar next = (i + 1 < code.size()) ? code[i + 1] : QChar();

        if (inLineComment) {
            if (ch == '\n') inLineComment = false;
            continue;
        }
        if (inBlockComment) {
            if (ch == '*' && next == '/') {
                inBlockComment = false;
                ++i;
            }
            continue;
        }
        if (escaped) {
            escaped = false;
            continue;
        }
        if (ch == '\\' && (inString || inChar)) {
            escaped = true;
            continue;
        }
        if (ch == '"' && !inChar) {
            inString = !inString;
            continue;
        }
        if (ch == '\'' && !inString) {
            inChar = !inChar;
            continue;
        }
        if (inString || inChar) continue;

        if (ch == '/' && next == '/') {
            inLineComment = true;
            ++i;
            continue;
        }
        if (ch == '/' && next == '*') {
            inBlockComment = true;
            ++i;
            continue;
        }

        if (ch == '{') {
            if (braceDepth == 0 &&
                isMainSignatureBeforeBrace(code.mid(segmentStart, i - segmentStart))) {
                int close = findMatchingBrace(code, i);
                if (close < 0) break;
                for (int j = segmentStart; j <= close; ++j) {
                    if (result[j] != '\n') result[j] = ' ';
                }
                i = close;
                segmentStart = close + 1;
                continue;
            }
            ++braceDepth;
        } else if (ch == '}') {
            if (braceDepth > 0) --braceDepth;
        } else if (ch == ';' && braceDepth == 0) {
            segmentStart = i + 1;
        }
    }

    return result;
}

struct PreparedCode {
    QString includes;
    QString body;
};

QString exposeDefaultPrivateClassMembers(QString body) {
    QRegularExpression classDefinitionRe(
        R"(\b(class|struct)\s+([A-Za-z_]\w*)\s*(?:final\s*)?(?:\:\s*([^{};]+))?\s*\{)"
    );

    QString rewritten;
    int last = 0;
    QRegularExpressionMatchIterator rewriteIt = classDefinitionRe.globalMatch(body);
    while (rewriteIt.hasNext()) {
        QRegularExpressionMatch match = rewriteIt.next();
        rewritten += body.mid(last, match.capturedStart(0) - last);

        QString header = match.captured(0);
        QString kind = match.captured(1);
        QString name = match.captured(2);
        QString bases = match.captured(3);
        if (kind == "class" && !bases.trimmed().isEmpty()) {
            QStringList exposedBases;
            for (QString base : bases.split(',')) {
                base = base.trimmed();
                QStringList tokens = base.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (tokens.contains("public") || tokens.contains("protected") ||
                    tokens.contains("private")) {
                    exposedBases.append(base);
                } else {
                    exposedBases.append("public " + base);
                }
            }
            header = QString("class %1 : %2 {").arg(name, exposedBases.join(", "));
        }

        rewritten += header;
        last = match.capturedEnd(0);
    }
    rewritten += body.mid(last);
    body = rewritten;

    QRegularExpression publicInsertRe(
        R"(\bclass\s+[A-Za-z_]\w*\s*(?:final\s*)?(?:\:\s*[^{};]+)?\s*\{)"
    );

    QVector<int> insertPositions;
    QRegularExpressionMatchIterator it = publicInsertRe.globalMatch(body);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        insertPositions.push_back(match.capturedEnd(0));
    }

    for (int i = insertPositions.size() - 1; i >= 0; --i) {
        body.insert(insertPositions[i], "\npublic:\n");
    }

    return body;
}

PreparedCode prepareOriginalCode(const QString &originalCode) {
    PreparedCode prepared;
    QRegularExpression includeRe("^\\s*#\\s*include\\s+[<\"][^>\"]+[>\"]");

    for (const QString &line : removeTopLevelMainFunction(originalCode).split('\n')) {
        if (includeRe.match(line).hasMatch()) {
            prepared.includes += line.trimmed() + "\n";
        } else {
            prepared.body += line + "\n";
        }
    }

    prepared.body = exposeDefaultPrivateClassMembers(prepared.body);
    return prepared;
}

struct ProbeField {
    QString ownerPath;
    QString ownerClass;
    FieldInfo field;
};

const RawClassInfo *findRawClass(const QVector<RawClassInfo> &classes,
                                 const string &name) {
    for (const RawClassInfo &cls : classes) {
        if (cls.name == name) return &cls;
    }
    return nullptr;
}

void collectFieldsForClass(const QVector<RawClassInfo> &classes,
                           const string &className,
                           const QStringList &path,
                           QVector<ProbeField> &out) {
    const RawClassInfo *cls = findRawClass(classes, className);
    if (!cls) return;

    for (const BaseClassInfo &base : cls->baseClasses) {
        QStringList basePath = path;
        basePath.append(QString::fromStdString(base.name));
        collectFieldsForClass(classes, base.name, basePath, out);
    }

    for (const FieldInfo &field : cls->fields) {
        ProbeField pf;
        pf.ownerPath = path.join(">");
        pf.ownerClass = QString::fromStdString(cls->name);
        pf.field = field;
        out.push_back(pf);
    }
}

QString objectExpressionForPath(const QString &targetClass,
                                const QStringList &path) {
    QString expr = QString("__obj_%1").arg(targetClass);
    for (const QString &baseClass : path) {
        expr = QString("static_cast<%1*>(%2)").arg(baseClass, expr);
    }
    return expr;
}

QString fieldAccessExpression(const QString &targetClass,
                              const QStringList &path,
                              const QString &fieldName) {
    return objectExpressionForPath(targetClass, path) + "->" + fieldName;
}

struct ProbeBase {
    QString path;
    QString className;
    QString access;
};

void collectBasesForClass(const QVector<RawClassInfo> &classes,
                          const string &className,
                          const QStringList &path,
                          QVector<ProbeBase> &out) {
    const RawClassInfo *cls = findRawClass(classes, className);
    if (!cls) return;

    for (const BaseClassInfo &base : cls->baseClasses) {
        QStringList basePath = path;
        QString baseName = QString::fromStdString(base.name);
        basePath.append(baseName);

        ProbeBase pb;
        pb.path = basePath.join(">");
        pb.className = baseName;
        pb.access = QString::fromStdString(base.access);
        out.push_back(pb);

        collectBasesForClass(classes, base.name, basePath, out);
    }
}

} // namespace

QString ProbeGenerator::generate(const QString &originalCode,
                                 const QVector<RawClassInfo> &classes) {
    QString probe;
    PreparedCode original = prepareOriginalCode(originalCode);

    probe += "#include <cstdio>\n";
    probe += "#include <cstddef>\n";
    probe += "#include <type_traits>\n\n";
    probe += original.includes;
    probe += "\n";

    probe += "#define private public\n";
    probe += "#define protected public\n\n";
    probe += original.body;
    probe += "\n\n";
    probe += "#undef private\n";
    probe += "#undef protected\n\n";

    probe += "int main() {\n";

    for (const RawClassInfo &cls : classes) {
        QString targetClass = QString::fromStdString(cls.name);

        probe += QString("    printf(\"CLASS:%1\\n\");\n").arg(targetClass);
        probe += QString("    printf(\"SIZE:%1:%zu\\n\", sizeof(%1));\n").arg(targetClass);
        probe += QString("    alignas(%1) unsigned char __storage_%1[sizeof(%1)] = {};\n")
                     .arg(targetClass);
        probe += QString("    %1 *__obj_%1 = reinterpret_cast<%1*>(__storage_%1);\n")
                     .arg(targetClass);
        probe += QString("    const char *__base_%1 = reinterpret_cast<const char*>(__obj_%1);\n")
                     .arg(targetClass);

        QVector<ProbeField> fieldsToPrint;
        collectFieldsForClass(classes, cls.name, QStringList{targetClass}, fieldsToPrint);

        QVector<ProbeBase> basesToPrint;
        collectBasesForClass(classes, cls.name, QStringList{targetClass}, basesToPrint);

        for (const ProbeBase &base : basesToPrint) {
            QStringList path = base.path.split(">");
            if (!path.isEmpty()) path.removeFirst();
            QString baseExpr = objectExpressionForPath(targetClass, path);
            probe += QString(
                "    printf(\"BASE:%1:%2:%3:%4:%td:%zu\\n\", "
                "reinterpret_cast<const char*>(%5) - __base_%1, sizeof(%3));\n"
            ).arg(targetClass, base.path, base.className, base.access, baseExpr);
        }

        for (const ProbeField &pf : fieldsToPrint) {
            QString ownerPath = pf.ownerPath;
            QString ownerClass = pf.ownerClass;
            QString fieldName = QString::fromStdString(pf.field.name);
            QStringList path = ownerPath.split(">");
            if (!path.isEmpty()) path.removeFirst();
            QString access = fieldAccessExpression(targetClass, path, fieldName);

            probe += QString(
                "    printf(\"FIELD:%1:%2:%3:%4:%td:%zu\\n\", "
                "reinterpret_cast<const char*>(&(%5)) - __base_%1, sizeof(%5));\n"
            ).arg(targetClass, ownerPath, ownerClass, fieldName, access);
        }
    }

    probe += "    return 0;\n";
    probe += "}\n";

    return probe;
}
