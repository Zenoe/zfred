// history.h
#pragma once
#include <deque>
#include <string>

class HistoryManager {
public:
    void add(const std::wstring& path);
    const std::deque<std::wstring>& all() const;
    void save();
    void load();
private:
    std::deque<std::wstring> items_;
    static constexpr size_t max_items_ = 40;
};