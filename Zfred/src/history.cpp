// history.cpp
#include "history.h"
#include <fstream>
#include "utils/sysutil.h"

void HistoryManager::add(const std::wstring& path) {
    if (path.empty()) return;
    if (!items_.empty() && items_.front() == path) return;
    items_.erase(std::remove(items_.begin(), items_.end(), path), items_.end());

    items_.push_front(path);
    if (items_.size() > max_items_) items_.pop_back();
}
const std::deque<std::wstring>& HistoryManager::all() const { return items_; }
void HistoryManager::save() {
    std::wofstream out(L"alfred_history.txt");
    for (const auto& s : items_) out << s << L"\n";
}
void HistoryManager::load() {
    items_.clear();
    std::wifstream in(L"alfred_history.txt");
    std::wstring s;
    while (std::getline(in, s)) if (!s.empty()) items_.push_back(s);

    const std::vector<std::wstring> recentVec = sys_util::loadSystemRecent();
    items_.insert(items_.end(), recentVec.begin(), recentVec.end());
}