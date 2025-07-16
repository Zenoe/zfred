// filebrowser.h
#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include "fileactions.h"

struct FileEntry {
    std::wstring label;
    std::wstring fullpath;
    bool is_dir = false, is_file = false, is_hidden = false, is_parent = false;
};

class FileBrowser {
public:
    FileBrowser();
    void set_cwd(const std::wstring& dir);
    void update(const std::wstring& pattern, bool show_hidden);
    const std::vector<FileEntry>& results() const;
    std::wstring cwd() const;
    void select(int idx);
    int selected_index() const;
    const FileEntry* selected() const;
private:
    std::wstring cwd_;
    std::wstring partial_; // filter for fuzzy
    bool show_hidden_ = false;
    std::vector<FileEntry> entries_;
    int sel_ = 0;
};