QT += core gui widgets network

CONFIG += c++17
QMAKE_CXXFLAGS += -Wno-error=implicit-function-declaration -Wno-implicit-function-declaration

TARGET = CppMemVisualizer_ver7
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    ClassParser.cpp \
    ProbeGenerator.cpp \
    LayoutEngine.cpp

HEADERS += \
    mainwindow.h \
    MemoryModel.h \
    ClassParser.h \
    ProbeGenerator.h \
    LayoutEngine.h

FORMS += \
    mainwindow.ui

macx {
    QMAKE_INFO_PLIST =
}
