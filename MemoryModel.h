#pragma once
#include <string>
#include <vector>
using namespace std;

struct FieldInfo {
    string name;
    string typeName;
    int offset;
    int size;
    bool isVptr;
    bool isPadding;
    string ownerClass;
    string ownerPath;
};

struct BaseClassInfo {
    string name;
    string access;
    bool isVirtual;
    int offset;
    int size;
    string path;
};

struct FunctionInfo {
    string signature;
    bool isVirtual;
    string ownerClass;
    string actualClass;
    int vtableIndex;
};

struct VTableEntry {
    int index;
    string functionName;
    string signature;
    string ownerClass;    // 这个虚函数最早属于哪个类
    string actualClass;   // 当前虚表里实际指向哪个类的实现
    bool isOverride;
};

struct ClassLayout {
    string className;
    int totalSize;
    string parentClass;
    vector<BaseClassInfo> baseClasses;
    vector<FieldInfo> fields;
    vector<FunctionInfo> functions;
    vector<VTableEntry> vtable;
};
