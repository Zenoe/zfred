#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <mutex>

struct ClipItem {
    int id;
    std::wstring content;
    std::wstring timestamp;
};

class Database {
public:
    Database(const std::wstring& dbPath);
    ~Database();

    bool addItem(const std::wstring& content);
    int getItemCount() const;
    std::vector<ClipItem> getRecentItems(int limit = 100);

private:
    sqlite3* db_;
    std::mutex mutable mtx_;
};