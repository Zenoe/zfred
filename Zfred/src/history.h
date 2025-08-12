// history.h
#pragma once
#include <deque>
#include <string>
#include <mutex>
#include <functional>
#include <atomic>
#include <condition_variable>

#include "debugtool.h"
class HistoryManager {
public:
    using FilterCallback = std::function<void()>;
    HistoryManager();
    void request_filter(const std::wstring& pat, FilterCallback filterDone);
    void add(const std::wstring& path);
    const std::deque<std::wstring>& all() const;
    //std::shared_ptr<const std::deque<std::wstring>> all() const;
    void allWith(std::function<void(const std::deque<std::wstring> &)>) const;
    void save();
    void loadSync();
    void load_async_batch(std::function<void( std::vector<std::wstring>&)> on_batch);
    void load_async(std::function< void() >);
    //void load_async(std::function< void(const std::wstring&) >);
    //void load_async(void(*on_append)(const std::wstring&));  // function pointer more lightweight, less flexible

    bool loaded_done() const;
    std::vector<const std::wstring*> filter(const std::wstring& pat) const;
    void filterModifyItemSync(const std::wstring& pat);
    void filterModifyItemThread(const std::wstring& pat, std::function<void()> filterDone);

    std::wstring operator [] (size_t idx) const;
    size_t size() const;

	//template<typename Func>
	//void withItems(Func&& fn) const {
	//	std::lock_guard<std::mutex> lock(filtered_items_mtx);
	//	fn(filtered_items_);
	//}

    ~HistoryManager() {
        {
            std::lock_guard<std::mutex> lock(filtered_items_mtx);
            stop_ = true;
        }
        cv_.notify_one();
        if (filter_worker_.joinable()) {
            filter_worker_.join();
        }
    }
private:
    std::thread filter_worker_;
    // are always accessed/modified under the filter_mutex_ lock. no need atomic
    bool stop_ = false;
    bool newRequest_ = false;
    std::condition_variable cv_;
    FilterCallback pending_cb_;
    std::wstring pending_pat_;
	//std::deque<std::wstring> items_;
	//std::deque<std::wstring> filtered_items_; 
    std::shared_ptr<std::deque<std::wstring>> items_ = std::make_shared<std::deque<std::wstring>>();
    std::shared_ptr<std::deque<std::wstring>> filtered_items_ = std::make_shared<std::deque<std::wstring>>();
	static constexpr size_t max_items_ = 40;
    std::mutex filter_mutex_;
	mutable std::mutex items_mtx;
	mutable std::mutex filtered_items_mtx;
	std::mutex loaded_mtx;
	std::atomic<bool> loaded_{ false };

};
