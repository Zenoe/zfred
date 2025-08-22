// string_util.h
#pragma once
#include <Windows.h>
#include <string>
#include <cwctype>
#include <cctype>
#include <algorithm>
#include <locale>
#include <vector>
#include "utils/ahocorasick.h"

#include <functional>
#include <algorithm>

namespace string_util {

    // Helper for tolower, handles wchar_t and char
    inline wchar_t to_lower(wchar_t ch) { return std::towlower(ch); }
    inline char    to_lower(char ch) { return std::tolower(static_cast<unsigned char>(ch)); }

    // Fuzzy match: all chars of pattern appear in order in str (case-insensitive)
    template<typename T>
    bool fuzzy_match(const T& pattern, const T& str) {
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
    std::pair<std::wstring, std::wstring> delete_word_backward(const wchar_t* input, size_t from_pos);

    // Helper for isspace (normal or wide char)
    template<typename CharT>
    bool is_space(CharT ch);

    template<>
    inline bool is_space<char>(char ch) {
        return std::isspace(static_cast<unsigned char>(ch));
    }

    template<>
    inline bool is_space<wchar_t>(wchar_t ch) {
        return std::iswspace(ch);
    }

    // Template function for splitting
    template<typename StringT>
    std::vector<StringT> split_by_space(const StringT& str) {
        using CharT = typename StringT::value_type;
        std::vector<StringT> result;
        size_t i = 0, n = str.length();
        while (i < n) {
            // skip spaces
            while (i < n && is_space(str[i])) ++i;
            size_t j = i;
            // find next space
            while (j < n && !is_space(str[j])) ++j;
            if (i < j)
                result.emplace_back(str.substr(i, j - i));
            i = j;
        }
        return result;
    }

    // Convert UTF-16 (wstring) to UTF-8 (string)
    inline std::string wstring_to_utf8(const std::wstring& wstr) {
        if (wstr.empty()) return {};
        int sz = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        std::string str(sz, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &str[0], sz, nullptr, nullptr);
        return str;
    }

    // Convert UTF-8 (string) to UTF-16 (wstring)
    inline std::wstring utf8_to_wstring(const std::string& str) {
        if (str.empty()) return {};
        int sz = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
        std::wstring wstr(sz, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], sz);
        return wstr;
    }

    // std::deque<std::wstring> filterVectorWithPats(
    //     const std::deque<std::wstring>& items,
    //     const std::wstring& pat
    // );


    template <
        typename Container,
        typename Extractor = std::function<std::wstring_view(const typename Container::value_type&)>
    >
    Container filterContainerWithPats(
        const Container& items,
        const std::wstring& pat,
        Extractor extractor = [](const auto& item) -> std::wstring_view {
            // Default: just return the string view if the value_type is std::wstring
            if constexpr (std::is_same_v<std::decay_t<decltype(item)>, std::wstring>) {
                return std::wstring_view(item);
            }
            else {
                static_assert(sizeof(item) == 0,
                    "Please provide a custom extractor for custom types!");
            }
        }
    ) {
        Container result;
        std::vector<std::wstring> pats = string_util::split_by_space(pat);
        std::vector<std::wstring_view> pattern_views;
        pattern_views.reserve(pats.size());
        for (const auto& s : pats)
            pattern_views.emplace_back(s);

        // Build Aho-Corasick ONCE
        AhoCorasick<wchar_t> ac;
        ac.build(pattern_views);

        //  create a single found vector outside the loop and std::fill(found.begin(), found.end(), false) before each use.
        // This eliminates allocation/deallocation every iteration.
        std::vector<bool> found(pattern_views.size(), false);
        for (const auto& item : items) {
            // std::vector<bool> found(pattern_views.size(), false); // move outside of loop
            std::fill(found.begin(), found.end(), false); // Reuse and reset
            std::wstring_view item_view = extractor(item);
            ac.search(item_view, found);
            if (std::all_of(found.begin(), found.end(), [](bool b) { return b; })) {
                result.push_back(item);
            }
        }

        return result;
    }
} // namespace string_util
