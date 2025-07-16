#pragma once
#include <string>
#include <vector>
#include <functional>
#include <algorithm>

struct Command {
    std::wstring keyword;
    std::wstring description;
    std::function<void()> action;
};

class CommandLibrary {
public:
    CommandLibrary();
    const std::vector<Command>& getAllCommands() const;
    const std::vector<Command>& all() const { return commands_; }

    // Fuzzy match: every char of pattern appears in string in order (not necessarily consecutive, case-insensitive)
    static bool fuzzy_match(const std::wstring& pattern, const std::wstring& str) {
        if (pattern.empty()) return true;
        size_t pi = 0;
        for (wchar_t c : str) {
            if (towlower(c) == towlower(pattern[pi]))
                if (++pi == pattern.size()) return true;
        }
        return false;
    }

    // Substring (case-insensitive)
    static bool substring_match(const std::wstring& pattern, const std::wstring& str) {
        if (pattern.empty()) return true;
        auto it = std::search(str.begin(), str.end(), pattern.begin(), pattern.end(),
            [](wchar_t ch1, wchar_t ch2) { return towlower(ch1) == towlower(ch2); });
        return it != str.end();
    }
    std::vector<const Command*> filter(const std::wstring& prefix) const;
private:
    std::vector<Command> commands_;
};