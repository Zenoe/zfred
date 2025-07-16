#pragma once
#include <windows.h>
#include <sstream>

template <typename... Args>
void OutputDebugPrint(Args&&... args) {
    std::wstringstream ss;
    (ss << ... << args);     // Fold expression (C++17)
    ss << L"\n";
    OutputDebugStringW(ss.str().c_str());
}
