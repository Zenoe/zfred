#include "FsUtils.h"

bool FsUtils::is_pathlike(const std::wstring& input) {
    if (input.size() >= 2 && input[1] == L':' &&
        (input[2] == L'\\' || input[2] == L'/')) return true;
    if (!input.empty() && (input[0] == L'\\' || input[0] == L'/')) return true;
    return false;
}

bool FsUtils::is_hidden(const fs::directory_entry& entry) {
    DWORD attr = GetFileAttributesW(entry.path().c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_HIDDEN);
}

bool FsUtils::is_system(const fs::directory_entry& entry) {
    DWORD attr = GetFileAttributesW(entry.path().c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_SYSTEM);
}

bool FsUtils::is_dir(const std::wstring& p) {
    if (p.empty()) return false;
    if(p[p.size()-1] == L'/') return true;
    try { return fs::is_directory(p); }
    catch (...) { return false; }
}

bool FsUtils::is_file(const std::wstring& p) {
    try { return fs::is_regular_file(p); }
    catch (...) { return false; }
}

std::wstring FsUtils::get_parent_path(const std::wstring& path) {
    try {
        return fs::path(path).parent_path().wstring();
    }
    catch (...) {
        return L"";
    }
}
