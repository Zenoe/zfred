#include "clipboard.h"
#include <atomic>
#include <chrono>
#include <string>
#include <iostream>

#include <spdlog/spdlog.h>
#include "utils/stringutil.h"

Clipboard::Clipboard()  {
    db_ = std::make_unique<Database<ClipItem>>(L"clipboard.sqlite");
    allItems = db_->getRecord(L"clipboard_items", 0, clipItemSize);
    filteredItems.reserve(allItems.size());
    for (const ClipItem& clip : allItems) {
        std::vector<bool> mask;
        filteredItems.emplace_back(clip, mask);
    }
    // error redirect to xmemeory
    // DisplayClipItem::DisplayClipItem(const DisplayClipItem &)”: 无法将参数 1 从“ClipItem”转换为“const DisplayClipItem &”	Zfred	C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.32.31326\include\xmemory	680
    // filteredItems.insert(filteredItems.end(), allItems.begin(), allItems.end());
}


void Clipboard::add (std::wstring item){
    std::string utf8_item = string_util::wstring_to_utf8(item);
    spdlog::info("Added clipboard item: {}", utf8_item);
    //spdlog::info(string_util::wstring_to_utf8(L"Added clipboard item:"), item);
    ClipItem newItem = db_->addItem(item);
    if (newItem.id_ != -1) {
        if (allItems.size() > clipItemSize) {
            allItems.pop_front();
        }
		allItems.emplace_back(newItem);
		std::vector<bool> mask;
		filteredItems.emplace_back(newItem, mask);
	}
}

size_t Clipboard::getCount() {
	return filteredItems.size();
}


const std::vector<DisplayClipItem>& Clipboard::getItems(int start, int limit) {
	return filteredItems;
}

bool Clipboard::write(int idx){
    if(idx < 0 || idx >= clipItemSize) return false;

    const std::wstring& text= allItems[idx].content;
    if (text.empty()) return false;

    if (!OpenClipboard(nullptr)) return false;

    EmptyClipboard();

    // 分配全局内存（包含结尾的 L'\0'）
    size_t data_size = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, data_size);
    if (!hMem) {
        CloseClipboard();
        return false;
    }

    // 将内容拷贝到分配的内存
    void* pMem = GlobalLock(hMem);
    memcpy(pMem, text.c_str(), data_size);
    GlobalUnlock(hMem);

    // 设置剪贴板内容为 Unicode 文本
    SetClipboardData(CF_UNICODETEXT, hMem);

    // 剪贴板现在拥有数据的所有权，不需要手动 GlobalFree(hMem)
    CloseClipboard();

    return true;
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
		for (const ClipItem& clip : allItems) {
			std::vector<bool> mask;
			filteredItems.emplace_back(clip, mask);
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
