// bookmarks.h
#pragma once
#include <vector>
#include <string>

class BookmarkManager {
public:
    void add(const std::wstring& path);
    void remove(const std::wstring& path);
    const std::vector<std::wstring>& all() const;
    void save();
    void load();
private:
    std::vector<std::wstring> bookmarks_;
};