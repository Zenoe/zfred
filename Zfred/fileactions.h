// fileactions.h
#pragma once
#include <string>

namespace fileactions {
    void launch(const std::wstring& filepath);
    void open_location(const std::wstring& filepath);
    bool move_to_trash(const std::wstring& filepath);
    bool delete_file(const std::wstring& filepath);
    bool rename_file(const std::wstring& filepath, const std::wstring& newname);
    void copy_path_to_clipboard(const std::wstring& filepath);
}