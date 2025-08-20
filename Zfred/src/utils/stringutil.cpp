#include "stringutil.h"

namespace string_util {
    // Delete one word backward (like Ctrl+Backspace)
    std::pair<std::wstring, std::wstring> delete_word_backward(const wchar_t* input, const size_t from_pos) {
        if (!input) return { L"", L"" };
        std::wstring s = input; // work on a copy

        // Find end of string (simulate caret at end)
        size_t end = s.size();
        if (from_pos < end) {
            end = from_pos;
        }
        if (end == 0) return { s, L"" };

        size_t i = end;

        // Skip any trailing spaces
        while (i > 0 && iswspace(s[i - 1])) --i;

        // Skip the word itself
        while (i > 0 && !iswspace(s[i - 1])) --i;

        std::wstring deleted = s.substr(i, end - i);
        // Remove from i to end
        s.erase(i, end - i);

        return {s, deleted};
    }


    // std::deque<std::wstring> filterVectorWithPats(
    //     const std::deque<std::wstring>& items,
    //     const std::wstring& pat
    // ) {
    //     std::deque<std::wstring> result;
    //     std::vector<std::wstring> pats = string_util::split_by_space(pat);
    //     std::vector<std::wstring_view> pattern_views;
    //     pattern_views.reserve(pats.size());
    //     for (const auto& s : pats)
    //         pattern_views.emplace_back(s);

    //     // Build Aho-Corasick ONCE
    //     AhoCorasick<wchar_t> ac;
    //     ac.build(pattern_views);

    //     for (const auto& item : items) {
    //         std::wstring_view item_view(item);
    //         std::vector<bool> found(pattern_views.size(), false);
    //         ac.search(item_view, found);
    //         if (std::all_of(found.begin(), found.end(), [](bool b) { return b; })) {
    //             result.push_back(item);
    //         }
    //     }

    //     return result;
    // }
}
