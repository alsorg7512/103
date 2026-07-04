#include "LayoutEngine.h"
#include "ProbeGenerator.h"

#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QTemporaryDir>
#include <QTextStream>

#include <algorithm>

namespace {

QString functionNameFromSignature(const QString &signature) {
    QRegularExpression nameRe("([A-Za-z_]\\w*)\\s*\\(");
    QRegularExpressionMatchIterator it = nameRe.globalMatch(signature);

    QString name;
    while (it.hasNext()) {
        name = it.next().captured(1);
    }
    return name;
}

ClassLayout *findLayout(QVector<ClassLayout> &layouts, const string &name) {
    for (ClassLayout &layout : layouts) {
        if (layout.className == name) return &layout;
    }
    return nullptr;
}

const ClassLayout *findLayoutConst(const QVector<ClassLayout> &layouts,
                                   const string &name) {
    for (const ClassLayout &layout : layouts) {
        if (layout.className == name) return &layout;
    }
    return nullptr;
}

const RawClassInfo *findRawClass(const QVector<RawClassInfo> &classes,
                                 const string &name) {
    for (const RawClassInfo &cls : classes) {
        if (cls.name == name) return &cls;
    }
    return nullptr;
}

QString typeNameForField(const QVector<RawClassInfo> &classes,
                         const string &ownerClass,
                         const string &fieldName) {
    const RawClassInfo *raw = findRawClass(classes, ownerClass);
    if (!raw) return "";

    for (const FieldInfo &field : raw->fields) {
        if (field.name == fieldName) {
            return QString::fromStdString(field.typeName);
        }
    }

    return "";
}

bool rawClassHasVirtualInChain(const QVector<RawClassInfo> &classes,
                               const string &className) {
    const RawClassInfo *raw = findRawClass(classes, className);
    if (!raw) return false;

    for (const FunctionInfo &fn : raw->functions) {
        if (fn.isVirtual) return true;
    }

    for (const BaseClassInfo &base : raw->baseClasses) {
        if (rawClassHasVirtualInChain(classes, base.name)) return true;
    }

    return false;
}

bool hasVirtualBase(const QVector<RawClassInfo> &classes, QString *detail) {
    for (const RawClassInfo &cls : classes) {
        for (const BaseClassInfo &base : cls.baseClasses) {
            if (base.isVirtual) {
                if (detail) {
                    *detail = QString("%1 virtual-inherits %2")
                                  .arg(QString::fromStdString(cls.name),
                                       QString::fromStdString(base.name));
                }
                return true;
            }
        }
    }
    return false;
}

void buildVTableForClass(QVector<ClassLayout> &layouts,
                         ClassLayout &cls,
                         QSet<QString> &done) {
    QString className = QString::fromStdString(cls.className);
    if (done.contains(className)) return;

    vector<VTableEntry> table;

    // 1. 先合并普通父类的逻辑虚表。这里仍然是教学用的逻辑视图，
    // 不是 ABI 级真实 vtable dump。
    for (const BaseClassInfo &base : cls.baseClasses) {
        QString basePath = QString::fromStdString(base.path);
        if (basePath.count(">") != 1) continue;

        ClassLayout *parent = findLayout(layouts, base.name);
        if (parent) {
            buildVTableForClass(layouts, *parent, done);
            for (const VTableEntry &entry : parent->vtable) {
                bool exists = false;
                for (const VTableEntry &current : table) {
                    if (current.ownerClass == entry.ownerClass &&
                        current.functionName == entry.functionName) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    VTableEntry copied = entry;
                    copied.index = static_cast<int>(table.size());
                    table.push_back(copied);
                }
            }
        }
    }

    // 2. 处理本类声明的虚函数
    for (FunctionInfo &fn : cls.functions) {
        if (!fn.isVirtual) continue;

        QString fname = functionNameFromSignature(QString::fromStdString(fn.signature));
        if (fname.isEmpty()) continue;

        bool replaced = false;

        // 简化逻辑：如果和父类虚函数同名，就认为是 override，覆盖原槽位。
        // 注意：这仍然是逻辑虚表，不是 ABI 级真实 vtable dump。
        for (VTableEntry &entry : table) {
            if (entry.functionName == fname.toStdString()) {
                entry.signature = fn.signature;
                entry.actualClass = cls.className;
                entry.isOverride = true;
                fn.vtableIndex = entry.index;
                replaced = true;
                break;
            }
        }

        // 如果不是 override，就是本类新增虚函数，追加到虚表末尾。
        if (!replaced) {
            VTableEntry entry;
            entry.index = static_cast<int>(table.size());
            entry.functionName = fname.toStdString();
            entry.signature = fn.signature;
            entry.ownerClass = cls.className;
            entry.actualClass = cls.className;
            entry.isOverride = false;

            fn.vtableIndex = entry.index;
            table.push_back(entry);
        }
    }

    cls.vtable = table;
    done.insert(className);
}

void buildAllVTables(QVector<ClassLayout> &layouts) {
    QSet<QString> done;
    for (ClassLayout &cls : layouts) {
        buildVTableForClass(layouts, cls, done);
    }
}

void sortFieldsByOffset(ClassLayout &cls) {
    std::sort(cls.fields.begin(), cls.fields.end(),
              [](const FieldInfo &a, const FieldInfo &b) {
                  if (a.offset != b.offset) return a.offset < b.offset;
                  if (a.isVptr != b.isVptr) return a.isVptr;
                  if (a.isPadding != b.isPadding) return !a.isPadding;
                  return a.name < b.name;
              });
}

bool classHasVirtualInChain(const QVector<ClassLayout> &layouts,
                            const ClassLayout &cls) {
    for (const FunctionInfo &fn : cls.functions) {
        if (fn.isVirtual) return true;
    }

        for (const BaseClassInfo &base : cls.baseClasses) {
            QString basePath = QString::fromStdString(base.path);
            if (basePath.count(">") != 1) continue;

            const ClassLayout *parent = findLayoutConst(layouts, base.name);
            if (parent && classHasVirtualInChain(layouts, *parent)) return true;
    }

    return false;
}

void removePaddingFields(ClassLayout &cls) {
    vector<FieldInfo> clean;
    for (const FieldInfo &field : cls.fields) {
        if (!field.isPadding) clean.push_back(field);
    }
    cls.fields = clean;
}

} // namespace

QVector<ClassLayout> LayoutEngine::run(const QString &originalCode,
                                       const QVector<RawClassInfo> &classes) {
    m_lastError.clear();

    QString virtualBaseDetail;
    if (hasVirtualBase(classes, &virtualBaseDetail)) {
        m_lastError =
            "检测到虚继承：" + virtualBaseDetail + "。\n\n"
            "当前支持普通非虚多继承、递归继承和非虚菱形继承；"
            "但虚继承会引入共享虚基类子对象和 ABI 相关的 vbptr/vbtable 布局，"
            "不能按普通父类子对象线性展开。为了保证结果正确，暂不支持分析虚继承布局。";
        return {};
    }

    ProbeGenerator gen;
    QString probeCode = gen.generate(originalCode, classes);

    QString output = compileAndRun(probeCode);
    qDebug() << "探针输出：" << output;

    return parseOutput(output, classes);
}

QString LayoutEngine::lastError() const {
    return m_lastError;
}

QString LayoutEngine::compileAndRun(const QString &probeCode) {
    QTemporaryDir tmpDir;
    if (!tmpDir.isValid()) {
        m_lastError = "无法创建临时目录。";
        return "ERROR:" + m_lastError;
    }

    QString srcPath = tmpDir.path() + "/probe.cpp";
    QString binPath = tmpDir.path() + "/probe";

    QFile srcFile(srcPath);
    if (!srcFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = "无法写入临时探针文件：" + srcPath;
        return "ERROR:" + m_lastError;
    }

    QTextStream out(&srcFile);
    out << probeCode;
    srcFile.close();

    QProcess compiler;

    // 用 clang++ 而不是 clang：对 C++ 标准库链接更稳，尤其是 std::string 等类型。
    compiler.start("clang++", {"-std=c++17", "-w", srcPath, "-o", binPath});

    if (!compiler.waitForFinished(10000)) {
        compiler.kill();
        compiler.waitForFinished();
        m_lastError = "探针编译超时。";
        return "ERROR:" + m_lastError;
    }

    if (compiler.exitStatus() != QProcess::NormalExit || compiler.exitCode() != 0) {
        QString err = QString::fromLocal8Bit(compiler.readAllStandardError());
        QString outText = QString::fromLocal8Bit(compiler.readAllStandardOutput());
        if (err.trimmed().isEmpty()) err = outText;

        m_lastError =
            "探针编译失败。\n\n"
            "【clang++ 输出】\n" + err;

        qDebug() << "编译错误：" << err;
        return "ERROR:" + m_lastError;
    }

    QProcess runner;
    runner.start(binPath);

    if (!runner.waitForFinished(5000)) {
        runner.kill();
        runner.waitForFinished();
        m_lastError = "探针运行超时。";
        return "ERROR:" + m_lastError;
    }

    if (runner.exitStatus() != QProcess::NormalExit || runner.exitCode() != 0) {
        QString err = QString::fromLocal8Bit(runner.readAllStandardError());
        m_lastError = "探针运行失败。\n\n" + err;
        return "ERROR:" + m_lastError;
    }

    return QString::fromLocal8Bit(runner.readAllStandardOutput());
}

QVector<ClassLayout> LayoutEngine::parseOutput(const QString &output,
                                               const QVector<RawClassInfo> &rawClasses) {
    QVector<ClassLayout> result;
    if (output.startsWith("ERROR:")) return result;

    ClassLayout current;
    bool inClass = false;

    for (const QString &line : output.split('\n')) {
        if (line.startsWith("CLASS:")) {
            if (inClass) result.append(current);

            current = ClassLayout();
            current.className = line.mid(6).toStdString();
            current.totalSize = 0;

            for (const RawClassInfo &raw : rawClasses) {
                if (raw.name == current.className) {
                    current.parentClass = raw.parentName;
                    current.functions = raw.functions;
                    break;
                }
            }

            inClass = true;

        } else if (line.startsWith("SIZE:")) {
            QStringList parts = line.split(':');
            if (parts.size() >= 3) {
                current.totalSize = parts[2].toInt();
            }

        } else if (line.startsWith("BASE:")) {
            QStringList parts = line.split(':');
            if (parts.size() >= 7) {
                QString targetClass = parts[1];
                if (targetClass.toStdString() != current.className) {
                    continue;
                }

                BaseClassInfo base;
                base.path = parts[2].toStdString();
                base.name = parts[3].toStdString();
                base.access = parts[4].toStdString();
                base.offset = parts[5].toInt();
                base.size = parts[6].toInt();
                base.isVirtual = false;
                current.baseClasses.push_back(base);
            }

        } else if (line.startsWith("FIELD:")) {
            QStringList parts = line.split(':');

            FieldInfo f;
            QString targetClass;
            QString ownerPath;
            QString ownerClass;
            QString fieldName;
            int offset = 0;
            int size = 0;

            // ver7 格式：FIELD:TargetClass:OwnerPath:OwnerClass:FieldName:Offset:Size
            if (parts.size() >= 7) {
                targetClass = parts[1];
                ownerPath = parts[2];
                ownerClass = parts[3];
                fieldName = parts[4];
                offset = parts[5].toInt();
                size = parts[6].toInt();

                if (targetClass.toStdString() != current.className) {
                    continue;
                }

            // ver5/ver6 格式：FIELD:TargetClass:OwnerClass:FieldName:Offset:Size
            } else if (parts.size() >= 6) {
                targetClass = parts[1];
                ownerPath = parts[2];
                ownerClass = parts[2];
                fieldName = parts[3];
                offset = parts[4].toInt();
                size = parts[5].toInt();

                if (targetClass.toStdString() != current.className) {
                    continue;
                }

            // 兼容旧格式：FIELD:ClassName:FieldName:Offset:Size
            } else if (parts.size() >= 5) {
                targetClass = QString::fromStdString(current.className);
                ownerClass = parts[1];
                fieldName = parts[2];
                offset = parts[3].toInt();
                size = parts[4].toInt();

            } else {
                continue;
            }

            f.ownerClass = ownerClass.toStdString();
            f.ownerPath = ownerPath.toStdString();
            f.name = fieldName.toStdString();
            f.offset = offset;
            f.size = size;
            f.isVptr = false;
            f.isPadding = false;
            f.typeName = typeNameForField(rawClasses, f.ownerClass, f.name).toStdString();

            current.fields.push_back(f);
        }
    }

    if (inClass) result.append(current);

    // 不再手动把父类字段复制到子类。
    // 继承字段在派生类对象中的 offset 由 ProbeGenerator 的探针直接测得。
    for (ClassLayout &cls : result) {
        removePaddingFields(cls);

        QSet<int> vptrOffsets;
        if (classHasVirtualInChain(result, cls)) {
            vptrOffsets.insert(0);
        }

        for (const BaseClassInfo &base : cls.baseClasses) {
            if (rawClassHasVirtualInChain(rawClasses, base.name)) {
                vptrOffsets.insert(base.offset);
            }
        }

        for (int offset : vptrOffsets) {
            FieldInfo vptr;
            vptr.name = "vptr";
            vptr.typeName = "void*";
            vptr.offset = offset;
            vptr.size = 8; // 本项目主要面向 64 位环境。
            vptr.isVptr = true;
            vptr.isPadding = false;
            vptr.ownerClass = cls.className;
            vptr.ownerPath = cls.className;

            for (const BaseClassInfo &base : cls.baseClasses) {
                if (base.offset == offset && rawClassHasVirtualInChain(rawClasses, base.name)) {
                    vptr.ownerClass = base.name;
                    vptr.ownerPath = base.path;
                    break;
                }
            }

            cls.fields.push_back(vptr);
        }

        // padding 必须在字段按 offset 排序之后计算。
        sortFieldsByOffset(cls);

        vector<FieldInfo> withPadding;
        int coveredEnd = 0;

        for (const FieldInfo &field : cls.fields) {
            if (field.offset > coveredEnd) {
                FieldInfo pad;
                pad.name = "padding";
                pad.typeName = "";
                pad.offset = coveredEnd;
                pad.size = field.offset - coveredEnd;
                pad.isVptr = false;
                pad.isPadding = true;
                pad.ownerClass = "";
                withPadding.push_back(pad);
            }

            withPadding.push_back(field);

            int fieldEnd = field.offset + field.size;
            if (fieldEnd > coveredEnd) {
                coveredEnd = fieldEnd;
            }
        }

        if (coveredEnd < cls.totalSize) {
            FieldInfo pad;
            pad.name = "padding";
            pad.typeName = "";
            pad.offset = coveredEnd;
            pad.size = cls.totalSize - coveredEnd;
            pad.isVptr = false;
            pad.isPadding = true;
            pad.ownerClass = "";
            withPadding.push_back(pad);
        }

        cls.fields = withPadding;
        sortFieldsByOffset(cls);
    }

    buildAllVTables(result);
    return result;
}
