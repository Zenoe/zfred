// filebrowser.cpp
#include "filebrowser.h"
#include <windows.h>
#include <algorithm>
#include <cwctype>

namespace fs = std::filesystem;

static bool fuzzy_match(const std::wstring& pattern, const std::wstring& str) {
    if (pattern.empty()) return true;
    size_t pi = 0;
    for (wchar_t c : str) {
        if (towlower(c) == towlower(pattern[pi]))
            if (++pi == pattern.size()) return true;
    }
    return false;
}
static bool substring_match(const std::wstring& pattern, const std::wstring& str) {
    if (pattern.empty()) return true;
    auto it = std::search(str.begin(), str.end(), pattern.begin(), pattern.end(),
        [](wchar_t ch1, wchar_t ch2) { return towlower(ch1) == towlower(ch2); });
    return it != str.end();
}
static bool is_hidden(const fs::directory_entry& entry) {
    DWORD attr = GetFileAttributesW(entry.path().c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_HIDDEN);
}
FileBrowser::FileBrowser() : cwd_(L"C:/") {}
void FileBrowser::set_cwd(const std::wstring& dir) { cwd_ = dir; }
std::wstring FileBrowser::cwd() const { return cwd_; }
void FileBrowser::select(int idx) {
    if (idx >= 0 && idx < (int)entries_.size()) sel_ = idx;
}
int FileBrowser::selected_index() const { return sel_; }
const FileEntry* FileBrowser::selected() const {
    if (sel_ >= 0 && sel_ < (int)entries_.size()) return &entries_[sel_];
    return nullptr;
}
const std::vector<FileEntry>& FileBrowser::results() const { return entries_; }
void FileBrowser::update(const std::wstring& pattern, bool show_hidden) {
    show_hidden_ = show_hidden;
    entries_.clear();
    fs::path bdir(cwd_.empty() ? L"." : cwd_);
    if (bdir.has_parent_path() || bdir.root_path() != bdir) {
        FileEntry e;
        e.label = L"..";
        e.fullpath = (bdir / L"..").lexically_normal().wstring();
        e.is_dir = true;
        e.is_parent = true;
        entries_.push_back(e);
    }
    bool use_substr = (!pattern.empty() && pattern[0] == L'*');
    std::wstring pat = use_substr ? pattern.substr(1) : pattern;
    try {
        for (const auto& entry : fs::directory_iterator(bdir)) {
            bool ishid = is_hidden(entry);
            if (!show_hidden_ && ishid) continue;
            FileEntry e;
            e.is_dir = entry.is_directory();
            e.is_file = entry.is_regular_file();
            e.is_hidden = ishid;
            e.is_parent = false;
            e.label = entry.path().filename().wstring() + (e.is_dir ? L"/" : L"");
            e.fullpath = entry.path().wstring();
            if (use_substr) {
                if (substring_match(pat, entry.path().filename().wstring()))
                    entries_.push_back(e);
            }
            else {
                if (fuzzy_match(pat, entry.path().filename().wstring()))
                    entries_.push_back(e);
            }
        }
        std::sort(entries_.begin(), entries_.end(), [](const FileEntry& a, const FileEntry& b) {
            if (a.is_parent) return true;
            if (b.is_parent) return false;
            if (a.is_dir != b.is_dir) return a.is_dir > b.is_dir;
            return _wcsicmp(a.label.c_str(), b.label.c_str()) < 0;
            });
    }
    catch (...) {}
    sel_ = entries_.empty() ? -1 : 0;
}