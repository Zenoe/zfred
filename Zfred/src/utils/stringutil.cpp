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
}