#pragma once
#include <windows.h>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class FsUtils {
public:
    static bool is_pathlike(const std::wstring& input);
    static bool is_hidden(const fs::directory_entry& entry);
    static bool is_system(const fs::directory_entry& entry);
    static bool is_dir(const std::wstring& p);
    static bool is_file(const std::wstring& p);
    static std::wstring get_parent_path(const std::wstring& path);
};
