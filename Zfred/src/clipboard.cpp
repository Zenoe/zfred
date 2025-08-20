#include "clipboard.h"
#include <atomic>
#include <chrono>
#include <string>
#include <iostream>
#include "utils/stringutil.h"

Clipboard::Clipboard()  {
    db_ = std::make_unique<Database<ClipItem>>(L"clipboard.sqlite");
    allItems = db_->getRecord(L"clipboard_items", 0, clipItemSize);
    filteredItems.reserve(allItems.size());
    filteredItems.insert(filteredItems.end(), allItems.begin(), allItems.end());
}


void Clipboard::add (std::wstring item){
    db_->addItem(item);
}

int Clipboard::getCount(){
    return db_->getItemCount();
}


std::vector<ClipItem> Clipboard::getItems(int start, int limit ) {
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

void Clipboard::filter(const std::wstring& pat){
    if(pat.empty()) return;
    filteredItems.clear();
    auto getContent = [](const ClipItem& d) -> std::wstring_view { return d.content; };
    filteredItems = string_util::filterContainerWithPats(allItems, pat, getContent);
}
