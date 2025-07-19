// string_util.h
#pragma once
#include <string>
#include <cwctype>
#include <cctype>
#include <algorithm>
#include <locale>

namespace string_util {

    // Helper for tolower, handles wchar_t and char
    inline wchar_t to_lower(wchar_t ch) { return std::towlower(ch); }
    inline char    to_lower(char ch) { return std::tolower(static_cast<unsigned char>(ch)); }

    // Fuzzy match: all chars of pattern appear in order in str (case-insensitive)
    template<typename String>
    bool fuzzy_match(const String& pattern, const String& str) {
        if (pattern.empty()) return true;
        size_t pi = 0;
        for (auto c : str) {
            if (to_lower(c) == to_lower(pattern[pi])) {
                if (++pi == pattern.size())
                    return true;
            }
        }
        return false;
    }

    // Substring (case-insensitive)
    // Uses locale-independent lowercase comparison
    template<typename String>
    bool substring_match(const String& pattern, const String& str) {
        if (pattern.empty()) return true;
        auto it = std::search(
            str.begin(), str.end(),
            pattern.begin(), pattern.end(),
            [](typename String::value_type ch1, typename String::value_type ch2) {
                return to_lower(ch1) == to_lower(ch2);
            }
        );
        return it != str.end();
    }

    // Delete one word backward (like Ctrl+Backspace)
    std::wstring delete_word_backward(const wchar_t* input);
} // namespace string_util