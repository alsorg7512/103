#include "ClassParser.h"
#include <QRegularExpression>
#include <QStringList>

namespace {

QString stripCommentsAndPreprocessor(const QString &code) {
    QString cleaned = code;
    cleaned.remove(QRegularExpression("#[^\n]*(?:\n|$)"));
    cleaned.remove(QRegularExpression("//[^\n]*(?=\n|$)"));
    cleaned.remove(QRegularExpression("/\\*.*?\\*/",
                                      QRegularExpression::DotMatchesEverythingOption));
    cleaned.remove(QRegularExpression("\\busing\\s+namespace\\s+[^;]+;"));
    return cleaned;
}

int findMatchingBrace(const QString &code, int openPos) {
    int depth = 0;
    bool inString = false;
    bool inChar = false;
    bool escaped = false;

    for (int i = openPos; i < code.size(); ++i) {
        QChar ch = code[i];

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

        if (ch == '{') {
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0) return i;
        }
    }

    return -1;
}

bool isTemplateClassHeader(const QString &code, int classPos) {
    int lineStart = code.lastIndexOf('\n', classPos);
    QString before = code.mid(qMax(0, lineStart + 1), classPos - lineStart - 1).trimmed();
    if (before.contains(QRegularExpression("template\\s*<"))) return true;

    QString previous = code.left(classPos).right(120);
    return previous.contains(QRegularExpression("template\\s*<[^;{}]*>\\s*$"));
}

bool isInsideBlock(const QString &code, int pos) {
    int depth = 0;
    bool inString = false;
    bool inChar = false;
    bool escaped = false;

    for (int i = 0; i < pos; ++i) {
        QChar ch = code[i];

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

        if (ch == '{') ++depth;
        else if (ch == '}' && depth > 0) --depth;
    }

    return depth > 0;
}

bool looksLikeFunctionBodyStart(const QString &prefix) {
    QString trimmed = prefix.trimmed();
    return trimmed.contains(QRegularExpression(
        "\\)\\s*(const\\s*)?(override\\s*)?(final\\s*)?$"
        ));
}

QString removeInlineFunctionBodies(const QString &body) {
    QString out;
    out.reserve(body.size());

    bool inString = false;
    bool inChar = false;
    bool escaped = false;
    for (int i = 0; i < body.size(); ++i) {
        QChar ch = body[i];

        if (escaped) {
            out += ch;
            escaped = false;
            continue;
        }
        if (ch == '\\' && (inString || inChar)) {
            out += ch;
            escaped = true;
            continue;
        }
        if (ch == '"' && !inChar) {
            out += ch;
            inString = !inString;
            continue;
        }
        if (ch == '\'' && !inString) {
            out += ch;
            inChar = !inChar;
            continue;
        }
        if (inString || inChar) {
            out += ch;
            continue;
        }

        if (ch == '{') {
            if (!looksLikeFunctionBodyStart(out)) {
                out += ch;
                continue;
            }

            int close = findMatchingBrace(body, i);
            if (close < 0) break;
            out += ';';
            i = close;
            continue;
        }

        out += ch;
    }

    return out;
}

QStringList splitDeclarations(const QString &body) {
    QStringList declarations;
    QString current;
    int parenDepth = 0;
    int angleDepth = 0;
    int bracketDepth = 0;

    for (QChar ch : body) {
        if (ch == '(') ++parenDepth;
        else if (ch == ')' && parenDepth > 0) --parenDepth;
        else if (ch == '<') ++angleDepth;
        else if (ch == '>' && angleDepth > 0) --angleDepth;
        else if (ch == '[') ++bracketDepth;
        else if (ch == ']' && bracketDepth > 0) --bracketDepth;

        if (ch == ';' && parenDepth == 0 && angleDepth == 0 && bracketDepth == 0) {
            QString declaration = current.trimmed();
            if (!declaration.isEmpty()) declarations.append(declaration + ";");
            current.clear();
        } else {
            current += ch;
        }
    }

    return declarations;
}

QStringList splitTopLevel(const QString &text, QChar separator) {
    QStringList parts;
    QString current;
    int parenDepth = 0;
    int angleDepth = 0;
    int bracketDepth = 0;
    int braceDepth = 0;

    for (QChar ch : text) {
        if (ch == '(') ++parenDepth;
        else if (ch == ')' && parenDepth > 0) --parenDepth;
        else if (ch == '<') ++angleDepth;
        else if (ch == '>' && angleDepth > 0) --angleDepth;
        else if (ch == '[') ++bracketDepth;
        else if (ch == ']' && bracketDepth > 0) --bracketDepth;
        else if (ch == '{') ++braceDepth;
        else if (ch == '}' && braceDepth > 0) --braceDepth;

        if (ch == separator && parenDepth == 0 && angleDepth == 0 &&
            bracketDepth == 0 && braceDepth == 0) {
            parts.append(current.trimmed());
            current.clear();
        } else {
            current += ch;
        }
    }

    QString tail = current.trimmed();
    if (!tail.isEmpty()) parts.append(tail);
    return parts;
}

QString stripTopLevelInitializer(const QString &declarator) {
    int parenDepth = 0;
    int angleDepth = 0;
    int bracketDepth = 0;
    int braceDepth = 0;

    for (int i = 0; i < declarator.size(); ++i) {
        QChar ch = declarator[i];
        if (ch == '(') ++parenDepth;
        else if (ch == ')' && parenDepth > 0) --parenDepth;
        else if (ch == '<') ++angleDepth;
        else if (ch == '>' && angleDepth > 0) --angleDepth;
        else if (ch == '[') ++bracketDepth;
        else if (ch == ']' && bracketDepth > 0) --bracketDepth;
        else if (ch == '{') ++braceDepth;
        else if (ch == '}' && braceDepth > 0) --braceDepth;
        else if (ch == '=' && parenDepth == 0 && angleDepth == 0 &&
                 bracketDepth == 0 && braceDepth == 0) {
            return declarator.left(i).trimmed();
        }
    }

    return declarator.trimmed();
}

void appendField(const QString &typeName, const QString &fieldName, RawClassInfo &info) {
    if (typeName.isEmpty() || fieldName.isEmpty()) return;

    FieldInfo field;
    field.typeName = typeName.trimmed().toStdString();
    field.name = fieldName.trimmed().toStdString();
    field.offset = 0;
    field.size = 4;
    field.isVptr = false;
    field.isPadding = false;
    field.ownerClass = info.name;
    field.ownerPath = info.name;
    info.fields.push_back(field);
}

QString commonTypeForLaterDeclarators(QString typeName) {
    typeName = typeName.trimmed();
    while (!typeName.isEmpty()) {
        QChar last = typeName.back();
        if (last == '*' || last == '&') {
            typeName.chop(1);
            typeName = typeName.trimmed();
            continue;
        }
        break;
    }
    return typeName;
}

void parseFieldDeclaration(const QString &declaration, RawClassInfo &info) {
    QString body = declaration;
    if (body.endsWith(';')) body.chop(1);
    body = body.trimmed();
    if (body.isEmpty()) return;

    QStringList declarators = splitTopLevel(body, ',');
    if (declarators.isEmpty()) return;

    QRegularExpression firstRe("^\\s*(.+?[\\s*&]+)([A-Za-z_]\\w*)\\s*(\\[[^\\]]+\\])?\\s*$");
    QRegularExpressionMatch first = firstRe.match(stripTopLevelInitializer(declarators[0]));
    if (!first.hasMatch()) return;

    QString baseType = first.captured(1).trimmed();
    QString commonBaseType = commonTypeForLaterDeclarators(baseType);
    appendField(baseType + first.captured(3).trimmed(), first.captured(2), info);

    for (int i = 1; i < declarators.size(); ++i) {
        QString item = stripTopLevelInitializer(declarators[i]);
        QRegularExpression restRe("^\\s*([*&\\s]*)([A-Za-z_]\\w*)\\s*(\\[[^\\]]+\\])?\\s*$");
        QRegularExpressionMatch rest = restRe.match(item);
        if (!rest.hasMatch()) continue;

        QString typeName = commonBaseType;
        QString pointerPart = rest.captured(1).trimmed();
        if (!pointerPart.isEmpty()) typeName += " " + pointerPart;
        typeName += rest.captured(3).trimmed();
        appendField(typeName, rest.captured(2), info);
    }
}

QString functionNameFromSignature(const QString &signature) {
    QRegularExpression nameRe("([A-Za-z_]\\w*)\\s*\\(");
    QRegularExpressionMatchIterator it = nameRe.globalMatch(signature);
    QString name;
    while (it.hasNext()) {
        name = it.next().captured(1);
    }
    return name;
}

RawClassInfo *findClass(QVector<RawClassInfo> &classes, const string &name) {
    for (RawClassInfo &cls : classes) {
        if (cls.name == name) return &cls;
    }
    return nullptr;
}

QString normalizeBaseAccess(const QString &kind, const QString &access) {
    if (!access.trimmed().isEmpty()) return access.trimmed();
    return kind == "struct" ? "public" : "private";
}

vector<BaseClassInfo> parseBaseList(const QString &baseList,
                                    const QString &classKind) {
    vector<BaseClassInfo> bases;

    for (QString item : splitTopLevel(baseList, ',')) {
        item = item.trimmed();
        if (item.isEmpty()) continue;

        QStringList tokens = item.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        BaseClassInfo base;
        base.isVirtual = false;
        base.offset = 0;
        base.size = 0;

        QString access;
        QString name;
        for (const QString &token : tokens) {
            if (token == "virtual") {
                base.isVirtual = true;
            } else if (token == "public" || token == "protected" || token == "private") {
                access = token;
            } else {
                name = token;
            }
        }

        if (name.isEmpty()) continue;

        base.name = name.toStdString();
        base.access = normalizeBaseAccess(classKind, access).toStdString();
        base.path = name.toStdString();
        bases.push_back(base);
    }

    return bases;
}

bool classHasVirtualFunctionNamed(QVector<RawClassInfo> &classes,
                                  const RawClassInfo &cls,
                                  const QString &name) {
    for (const FunctionInfo &fn : cls.functions) {
        if (fn.isVirtual &&
            functionNameFromSignature(QString::fromStdString(fn.signature)) == name) {
            return true;
        }
    }

    for (const BaseClassInfo &base : cls.baseClasses) {
        RawClassInfo *parent = findClass(classes, base.name);
        if (parent && classHasVirtualFunctionNamed(classes, *parent, name)) {
            return true;
        }
    }

    return false;
}

void parseClassBody(const QString &body, RawClassInfo &info) {
    QString normalized = removeInlineFunctionBodies(body);
    normalized.remove(QRegularExpression("(^|\\n)\\s*(public|private|protected)\\s*:\\s*"));

    for (QString declaration : splitDeclarations(normalized)) {
        declaration = declaration.trimmed();
        if (declaration.isEmpty()) continue;

        // static members do not live inside each object, so they should not
        // participate in object memory layout or vtable display.
        if (declaration.contains(QRegularExpression("^\\s*static\\b"))) continue;

        QRegularExpression destructorRe(
            "^\\s*(virtual\\s+)?~([A-Za-z_]\\w*)\\s*\\([^;]*\\)\\s*(override\\s*)?(final\\s*)?;\\s*$"
            );
        QRegularExpressionMatch dm = destructorRe.match(declaration);
        if (dm.hasMatch()) {
            FunctionInfo fn;
            fn.signature = declaration.toStdString();
            fn.isVirtual = dm.captured(1).trimmed() == "virtual" ||
                           !dm.captured(3).trimmed().isEmpty();
            fn.ownerClass = info.name;
            fn.actualClass = info.name;
            fn.vtableIndex = -1;
            info.functions.push_back(fn);
            continue;
        }

        QRegularExpression funcRe(
            "^\\s*(virtual\\s+)?[A-Za-z_:][\\w:\\s<>,*&~]*\\s+([A-Za-z_]\\w*)\\s*"
            "\\([^;]*\\)\\s*(const\\s*)?(override\\s*)?(final\\s*)?;\\s*$"
            );
        QRegularExpressionMatch fm = funcRe.match(declaration);
        if (fm.hasMatch()) {
            FunctionInfo fn;
            fn.signature = declaration.toStdString();
            fn.isVirtual = fm.captured(1).trimmed() == "virtual" ||
                           !fm.captured(4).trimmed().isEmpty();
            fn.ownerClass = info.name;
            fn.actualClass = info.name;
            fn.vtableIndex = -1;
            info.functions.push_back(fn);
            continue;
        }

        if (declaration.contains('(')) continue;
        parseFieldDeclaration(declaration, info);
    }
}

} // namespace

QVector<RawClassInfo> ClassParser::parse(const QString &code) {
    QVector<RawClassInfo> result;
    QString cleaned = stripCommentsAndPreprocessor(code);

    QRegularExpression classHeaderRe(
        "\\b(class|struct)\\s+([A-Za-z_]\\w*)\\s*(?:final\\s*)?"
        "(?:\\:\\s*([^{};]+))?\\s*\\{"
        );

    int searchPos = 0;
    while (true) {
        QRegularExpressionMatch m = classHeaderRe.match(cleaned, searchPos);
        if (!m.hasMatch()) break;

        int headerStart = m.capturedStart(0);
        int openBrace = m.capturedEnd(0) - 1;
        int closeBrace = findMatchingBrace(cleaned, openBrace);
        if (closeBrace < 0) break;

        searchPos = closeBrace + 1;
        if (isInsideBlock(cleaned, headerStart)) continue;
        if (isTemplateClassHeader(cleaned, headerStart)) continue;

        int afterClose = closeBrace + 1;
        while (afterClose < cleaned.size() && cleaned[afterClose].isSpace()) ++afterClose;
        if (afterClose >= cleaned.size() || cleaned[afterClose] != ';') continue;

        RawClassInfo info;
        QString classKind = m.captured(1);
        info.name = m.captured(2).toStdString();
        info.baseClasses = parseBaseList(m.captured(3), classKind);
        if (!info.baseClasses.empty()) {
            info.parentName = info.baseClasses.front().name;
        }

        QString body = cleaned.mid(openBrace + 1, closeBrace - openBrace - 1);
        parseClassBody(body, info);
        result.append(info);
    }

    // 补充识别隐式 override：本类显式声明的函数只要覆盖祖先类虚函数，
    // 在 C++ 语义里仍然是虚函数；但不把继承来的函数塞进本类列表。
    bool changed = true;
    while (changed) {
        changed = false;

        for (RawClassInfo &cls : result) {
            for (FunctionInfo &fn : cls.functions) {
                if (fn.isVirtual || cls.baseClasses.empty()) continue;
                QString name = functionNameFromSignature(QString::fromStdString(fn.signature));
                if (name.isEmpty()) continue;

                for (const BaseClassInfo &base : cls.baseClasses) {
                    RawClassInfo *parent = findClass(result, base.name);
                    if (parent && classHasVirtualFunctionNamed(result, *parent, name)) {
                        fn.isVirtual = true;
                        changed = true;
                        break;
                    }
                }
            }
        }
    }

    return result;
}
