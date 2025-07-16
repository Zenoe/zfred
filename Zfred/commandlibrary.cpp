#include "commandlibrary.h"
#include <windows.h>
#include <cwctype>  // For std::towlower in C++

CommandLibrary::CommandLibrary() {
    commands_.push_back({ L"exit", L"Exit the application", []() { PostQuitMessage(0); } });
    commands_.push_back({ L"calc", L"Open calculator", []() { ShellExecuteW(nullptr, L"open", L"calc.exe", nullptr, nullptr, SW_SHOW); } });
    commands_.push_back({ L"notepad", L"Open Notepad", []() { ShellExecuteW(nullptr, L"open", L"notepad.exe", nullptr, nullptr, SW_SHOW); } });
    commands_.push_back({ L"explorer", L"Open file explorer", []() { ShellExecuteW(nullptr, L"open", L"explorer.exe", nullptr, nullptr, SW_SHOW); } });
    commands_.push_back({ L"mspaint", L"Open Paint", []() { ShellExecuteW(nullptr, L"open", L"mspaint.exe", nullptr, nullptr, SW_SHOW); } });
    commands_.push_back({ L"cmd", L"Open Command Prompt", []() { ShellExecuteW(nullptr, L"open", L"cmd.exe", nullptr, nullptr, SW_SHOW); } });
    commands_.push_back({ L"powershell", L"Open PowerShell", []() { ShellExecuteW(nullptr, L"open", L"powershell.exe", nullptr, nullptr, SW_SHOW); } });
}

const std::vector<Command>& CommandLibrary::getAllCommands() const {
    return commands_;
}

// Simple case-insensitive prefix filter
std::vector<const Command*> CommandLibrary::filter(const std::wstring& input) const {
    std::vector<const Command*> result;
    bool use_substring = (!input.empty() && input[0] == L'*');
    std::wstring actual = use_substring ? input.substr(1) : input;
    for (const auto& cmd : commands_) {
        if (use_substring) {
            if (substring_match(actual, cmd.keyword)) result.push_back(&cmd);
        }
        else {
            if (fuzzy_match(actual, cmd.keyword)) result.push_back(&cmd);
        }
    }
    return result;
}
