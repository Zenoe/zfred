#pragma once
#include "utils/dbsqlite.h"
#include "constant.h"

struct ClipItem {
    int id;
    std::wstring content;
    std::wstring timestamp;
};

class Clipboard {
public:
    Clipboard();
    void add(std::wstring item);
    int getCount();
    std::vector<ClipItem> getItems(int start = 0, int limit = clipItemSize);
    bool write(int idx);
    void filter(const std::wstring&);
private:
    std::unique_ptr<Database<ClipItem>> db_;
    std::vector<ClipItem> allItems;
    std::vector<ClipItem> filteredItems;
};
