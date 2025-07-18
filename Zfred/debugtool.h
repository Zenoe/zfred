#pragma once
#include <windows.h>
#include <sstream>
#include <vector>
template <typename... Args>
void OutputDebugPrint(Args&&... args) {
    std::wstringstream ss;
    (ss << ... << args);     // Fold expression (C++17)
    ss << L"\n";
    OutputDebugStringW(ss.str().c_str());
}

// ��ֹ���ض��壬Ҫôinline��Ҫô��cpp�ж��塣 .h �ļ���������
// ģ�庯�����ܴ�����
inline void OutputDebugPrintVev(const std::vector<FileEntry>& entries) {
    OutputDebugPrint("vev begin");
    for (auto e : entries) {
        OutputDebugPrint(e.fullpath, " ", e.label);
    }
    OutputDebugPrint("vev end");
}
