#pragma once
#include "utils/dbsqlite.h"
#include "constant.h"
#include "utils/dbsqlite.h"
#include "db/SqliteDB.h"
struct ClipItem {
    int id_=0;
    std::wstring content;
    std::wstring timestamp;
    ClipItem() = default;
// template<typename S1, typename S2>
//     ClipItem(int id, S1&& pcontent, S2&& ptimestamp)
//         : id_(id), content(std::forward<S1>(pcontent)), timestamp(std::forward<S2>(ptimestamp)) {}
    // Copy version
    ClipItem(int id, const std::wstring& pcontent, const std::wstring& ptimestamp)
        : id_(id), content(pcontent), timestamp(ptimestamp) {}

    // Move version
    ClipItem(int id, std::wstring&& pcontent, std::wstring&& ptimestamp)
        : id_(id), content(std::move(pcontent)), timestamp(std::move(ptimestamp)) {}
};

struct DisplayClipItem : ClipItem {
    std::vector<bool> highlight_mask;
    DisplayClipItem() = default;

    DisplayClipItem(const ClipItem& clip, std::vector<bool> mask)
        : ClipItem(clip), highlight_mask(std::move(mask)) {}
};
// struct DisplayClipItem {
//     const ClipItem* base;
//     std::vector<bool> highlight_mask;
// };

class Clipboard {
public:
    Clipboard(const std::string& dbPath="clipboard.db");
    void add(std::wstring item);
    bool remove(int idx);
    size_t getCount();
    const std::vector<DisplayClipItem>& getItems(int start = 0, int limit = clipItemSize);
    bool write(int idx);
    void filter(const std::wstring&);
private:
    SqliteDB db_;
    // std::unique_ptr<Database> db_;
    std::deque<ClipItem> allItems;
    std::vector<DisplayClipItem> filteredItems;
    void initItems(int start = 0, int limit = clipItemSize);
};
