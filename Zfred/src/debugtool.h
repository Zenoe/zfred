#pragma once
#include <windows.h>
#include <sstream>
#include <vector>
template <typename... Args>
void OutputDebugPrint(Args&&... args) {
#ifdef _DEBUG
    std::wstringstream ss;
    (ss << ... << args);     // Fold expression (C++17)
    ss << L"\n";
    OutputDebugStringW(ss.str().c_str());
#endif
}

// 防止多重定义，要么inline，要么在cpp中定义。 .h 文件负责声明
// 模板函数不受此限制
//inline void OutputDebugPrintVev(const std::vector<FileEntry>& entries) {
//    OutputDebugPrint("vev begin");
//    for (auto e : entries) {
//        OutputDebugPrint(e.fullpath, " ", e.label);
//    }
//    OutputDebugPrint("vev end");
//}
