#include "ClassParser.h"
#include <QCoreApplication>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <iostream>

namespace {

QString boolText(bool value) {
    return value ? "true" : "false";
}

void printUsage(const QString &programName) {
    std::cout << "Usage: " << programName.toStdString()
              << " [cpp-input-file]\n"
              << "No file is given, parser_test_input.cpp will be used.\n";
}

QString fieldText(const FieldInfo &field) {
    QString typeName = QString::fromStdString(field.typeName);
    QString name = QString::fromStdString(field.name);

    QRegularExpression arrayRe("^(.*)(\\[[^\\]]+\\])$");
    QRegularExpressionMatch match = arrayRe.match(typeName);
    if (match.hasMatch()) {
        return QString("%1 %2%3")
            .arg(match.captured(1).trimmed(), name, match.captured(2));
    }

    return QString("%1 %2").arg(typeName, name);
}

} // namespace

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();

    if (args.size() > 2) {
        printUsage(args[0]);
        return 1;
    }

    QString inputPath = args.size() == 2 ? args[1] : "parser_test_input.cpp";
    QFile input(inputPath);
    if (!input.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "Cannot open input file: " << inputPath.toStdString() << "\n";
        return 1;
    }

    QTextStream in(&input);
    QString code = in.readAll();

    ClassParser parser;
    QVector<RawClassInfo> classes = parser.parse(code);

    if (classes.isEmpty()) {
        std::cout << "No class definitions found.\n";
        return 0;
    }

    for (const RawClassInfo &cls : classes) {
        std::cout << "CLASS " << cls.name;
        if (!cls.parentName.empty()) {
            std::cout << " : " << cls.parentName;
        }
        std::cout << "\n";

        std::cout << "  FIELDS " << cls.fields.size() << "\n";
        for (const FieldInfo &field : cls.fields) {
            std::cout << "    " << fieldText(field).toStdString() << "\n";
        }

        std::cout << "  FUNCTIONS " << cls.functions.size() << "\n";
        for (const FunctionInfo &fn : cls.functions) {
            std::cout << "    " << fn.signature
                      << " | virtual=" << boolText(fn.isVirtual).toStdString()
                      << "\n";
        }

        std::cout << "\n";
    }

    return 0;
}
