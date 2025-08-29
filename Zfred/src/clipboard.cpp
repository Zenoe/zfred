#include "clipboard.h"
#include <atomic>
#include <chrono>
#include <string>
#include <iostream>

#include <spdlog/spdlog.h>
#include "utils/stringutil.h"
#include "utils/sysutil.h"
#include "db/ResultSet.h"

Clipboard::Clipboard(const std::string& dbPath): db_(dbPath)  {
    static const char* sql =
        "CREATE TABLE IF NOT EXISTS clipboard_items("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "content TEXT,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";
    db_.execute(sql);
    initItems();
    // auto items = getItems();
    // allItems.insert(allItems.end(), items.begin(), items.end());
    // db_ = std::make_unique<Database>(L"clipboard.sqlite");
    // allItems = db_->getRecord(L"clipboard_items", 0, clipItemSize);
    // filteredItems.reserve(allItems.size());
    // for (const ClipItem& clip : allItems) {
    //     std::vector<bool> mask;
    //     filteredItems.emplace_back(clip, mask);
    // }
    // error redirect to xmemeory
    // DisplayClipItem::DisplayClipItem(const DisplayClipItem &)”: 无法将参数 1 从“ClipItem”转换为“const DisplayClipItem &”	Zfred	C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.32.31326\include\xmemory	680
    // filteredItems.insert(filteredItems.end(), allItems.begin(), allItems.end());
}

void Clipboard::initItems(int start, int limit) {
    auto rs = db_.query(
        "SELECT id, content, timestamp FROM clipboard_items "
        "ORDER BY timestamp DESC LIMIT ? OFFSET ?;",
        { std::to_string(limit), std::to_string(start) }
    );
    while (rs->next()) {
        ClipItem item;
        item.id_ = rs->getInt(0);
        item.content = string_util::utf8_to_wstring(rs->getString(1));
        item.timestamp = string_util::utf8_to_wstring(rs->getString(2));
        allItems.push_back(std::move(item));
    }

    filteredItems.reserve(allItems.size());
    for (const ClipItem& clip : allItems) {
        std::vector<bool> mask;
        filteredItems.emplace_back(clip, mask);
    }
}
void Clipboard::add (std::wstring copiedItem){
    std::string utf8_item = string_util::wstring_to_utf8(copiedItem);
    db_.execute(
                "INSERT INTO clipboard_items (content) VALUES (?);",
                { utf8_item }
               );
    // timestamp may be incorrect if multiple rows are added at exactly the same time by other threads/processes.
    // auto rs = db_.query("SELECT last_insert_rowid(), timestamp FROM clipboard_items ORDER BY id DESC LIMIT 1;");
    // Correct: get id of just inserted row
    auto rs = db_.query(
                        "SELECT id, content, timestamp FROM clipboard_items WHERE id = last_insert_rowid();"
                       );
    ClipItem item;

    if (rs->next()) {
        item.id_ = rs->getInt(0);
        item.content = string_util::utf8_to_wstring(rs->getString(1));
        item.timestamp = string_util::utf8_to_wstring(rs->getString(2));
        if (allItems.size() >= clipItemSize) {
            allItems.pop_front();
        }
		allItems.emplace_back(item);
		std::vector<bool> mask;
		filteredItems.emplace_back(item, mask);

    }
}

bool Clipboard::remove(int idx){
    if (idx < 0 || idx >= static_cast<int>(filteredItems.size()))
        return false;
    int item_id = filteredItems[idx].id_;

    int affected = db_.execute(
                               "DELETE FROM clipboard_items WHERE id = ?;",
                               {std::to_string(item_id)}
                              );
    if(affected > 0){
        // 2. Remove from filteredItems
        filteredItems.erase(filteredItems.begin() + idx);

        // 3. Remove from allItems (search by id_)
        auto it = std::find_if(allItems.begin(), allItems.end(),
                               [item_id](const ClipItem& item) { return item.id_ == item_id; });
        if (it != allItems.end()) {
            allItems.erase(it);
        }
        return true;
    }
    return false;
}

size_t Clipboard::getCount() {
    return filteredItems.size();
}

const std::vector<DisplayClipItem>& Clipboard::getItems(int start, int limit) {
	return filteredItems;
}

bool Clipboard::write(int idx){
    if(idx < 0 || idx >= filteredItems.size()) return false;
    const std::wstring& text= filteredItems[idx].content;
    return sys_util::Write2Clipboard(text);
}

std::vector<DisplayClipItem> filterContainerWithHighlights(const std::deque<ClipItem>& items, const std::wstring& pat)
{
    std::vector<DisplayClipItem> result;
    std::vector<std::wstring> pats = string_util::split_by_space(pat);
    std::vector<std::wstring_view> pattern_views;
    pattern_views.reserve(pats.size());
    for (const auto& s : pats)
        pattern_views.emplace_back(s);

    // Build Aho-Corasick ONCE
    AhoCorasick<wchar_t> ac;
    ac.build(pattern_views);

    std::vector<bool> found(pattern_views.size(), false);

    for (const auto& item : items) {
        std::fill(found.begin(), found.end(), false);

        std::wstring_view item_view = item.content;
        // 1. Find which patterns are present
        ac.search(item_view, found);

        if (std::all_of(found.begin(), found.end(), [](bool b) { return b; })) {
            // 2. Compute highlight_mask:
            std::vector<bool> highlight_mask(item_view.size(), false);
            // Find all pattern occurrences for this item
            auto matches = ac.find_all_matches(item_view);
            // Assume find_all_matches returns std::vector<std::pair<size_t /*end_pos*/, size_t /*pat_idx*/>>
            for (const auto& [end_pos, pat_idx] : matches) {
                size_t patlen = pattern_views[pat_idx].size();
                if (end_pos + 1 >= patlen) {
                    for (size_t i = end_pos + 1 - patlen; i <= end_pos && i < highlight_mask.size(); ++i)
                        highlight_mask[i] = true;
                }
            }

            // 3. Build result item
            DisplayClipItem dci(item, std::move(highlight_mask));
            //DisplayClipItem dci;
            // dci.id_ = item.id_;
            // dci.highlight_mask = std::move(highlight_mask);

            result.push_back(std::move(dci));
        }
    }

    return result;
}
void Clipboard::filter(const std::wstring& pat){
    if (pat.empty()) {
        if (filteredItems.size() != allItems.size()) {
            filteredItems.clear();
			for (const ClipItem& clip : allItems) {
				std::vector<bool> mask;
				filteredItems.emplace_back(clip, mask);
			}
        }
        return;
	}
	filteredItems.clear();
	auto getContent = [](const ClipItem& d) -> std::wstring_view { return d.content; };
	// auto getContent = [](const std::unique_ptr<ClipItem>& item) -> std::wstring_view {
	//     return item->content;
	// };
	// filteredItems = string_util::filterContainerWithPats(allItems, pat, getContent);
	filteredItems = filterContainerWithHighlights(allItems, pat);
}
