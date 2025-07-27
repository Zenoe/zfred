// history.h
#pragma once
#include <deque>
#include <string>
#include <mutex>
#include <functional>
#include <atomic>

class HistoryManager {
public:
    void add(const std::wstring& path);
    const std::deque<std::wstring>& all() const;
    void save();
    void load();
    void load_async(std::function<void( std::vector<std::wstring>&)> on_batch);
    void load_async(std::function< void() >);
    //void load_async(std::function< void(const std::wstring&) >);
    //void load_async(void(*on_append)(const std::wstring&));  // function pointer more lightweight, less flexible

    bool loaded_done() const;
    std::vector<const std::wstring*> filter(const std::wstring& pat) const;
    void filterModifyItem(const std::wstring& pat);
private:
    std::deque<std::wstring> items_;
    std::deque<std::wstring> filtered_items_;  // the master list (never filtered)
    static constexpr size_t max_items_ = 40;
    mutable std::mutex items_mtx;
    mutable std::mutex filtered_items_mtx;
    std::mutex loaded_mtx;
    std::atomic<bool> loaded_{ false };

};