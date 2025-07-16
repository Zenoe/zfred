// bookmarks.cpp
#include "bookmarks.h"
#include <fstream>

void BookmarkManager::add(const std::wstring& path) {
    if (path.empty()) return;
    if (std::find(bookmarks_.begin(), bookmarks_.end(), path) == bookmarks_.end())
        bookmarks_.push_back(path);
}
void BookmarkManager::remove(const std::wstring& path) {
    bookmarks_.erase(std::remove(bookmarks_.begin(), bookmarks_.end(), path), bookmarks_.end());
}
const std::vector<std::wstring>& BookmarkManager::all() const { return bookmarks_; }
void BookmarkManager::save() {
    std::wofstream out(L"alfred_bookmarks.txt");
    for (const auto& s : bookmarks_) out << s << L"\n";
}
void BookmarkManager::load() {
    bookmarks_.clear();
    std::wifstream in(L"alfred_bookmarks.txt");
    std::wstring s;
    while (std::getline(in, s)) if (!s.empty()) bookmarks_.push_back(s);
}