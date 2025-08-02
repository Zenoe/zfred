// history.cpp
#include "history.h"
#include <fstream>
#include "utils/sysutil.h"
#include <thread>
#include <unordered_set>
#include "utils/stringutil.h"
#include "debugtool.h"
#include "utils/ahocorasick.h"

HistoryManager::HistoryManager(){
	filter_worker_ = std::thread([this] {
		std::unique_lock<std::mutex> lock(filter_mutex_);
		while (!stop_) {
			cv_.wait(lock, [this] { return stop_ || newRequest_; });

			if (stop_) break;

			// Copy pending job to local vars for thread safety
			auto pattern = pending_pat_;
			auto callback = pending_cb_;
			newRequest_ = false; // Mark current job as taken

			lock.unlock();

			// Do filtering
			if (pattern.empty()) {
				std::lock_guard<std::mutex> lck(filtered_items_mtx);
				this->filtered_items_ = this->items_; // Restore all
			}
			else {
				{
					std::lock_guard<std::mutex> lock(filtered_items_mtx);
					this->filtered_items_.clear();
				}
				// 先拷贝一份items_，减少加锁时间和粒度
				std::deque<std::wstring> items_copy;
				{
					std::lock_guard<std::mutex> items_lock(items_mtx);
					items_copy = this->items_;
				}

				// 局部变量做筛选，避免反复加锁
				std::deque<std::wstring> result;
				for (const auto& item : items_copy) {
					std::vector<std::wstring_view> pattern_views;
					// todo split_by_space 参数是引用, pat 可能在split_by_space中途执行过程中被外面修改
					std::vector<std::wstring> pats = string_util::split_by_space(pat);
					pattern_views.reserve(pats.size());
					for (const auto& s : pats)
						pattern_views.push_back(s);

					std::wstring_view item_view(item);
					if (ah_substring_match(pattern_views, item_view)) {
						result.push_back(item);
					}
				}
				// 一次性写回filtered_items_
				{
					std::lock_guard<std::mutex> filtered_lock(filtered_items_mtx);
					this->filtered_items_ = std::move(result);
				}
			}

			// Optional: check for new request _here_ to abort result delivery
			lock.lock();
			if (!stop_ && !newRequest_ && callback) {
				lock.unlock();
				callback(); // safe to signal GUI
			}
			else {
				lock.unlock();
			}
		}
		});
	filter_worker_ = std::thread([this] {
		std::unique_lock<std::mutex> lock(filter_mutex_);
		while (!stop_) {
			cv_.wait(lock, [this] { return stop_ || newRequest_; });
			if (stop_) { break; }
			auto pat = pending_pat_;
			auto cb = pending_cb_;
			newRequest_ = false;
			lock.unlock();

			if (pat.empty()) {
				std::lock_guard<std::mutex> lock(filtered_items_mtx);
				//std::lock_guard<std::mutex> lock2(items_mtx);
				this->filtered_items_ = this->items_; // Restore all
				// std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // Let server start
			}
			else {
				{
					std::lock_guard<std::mutex> lock(filtered_items_mtx);
					this->filtered_items_.clear();
				}
				// 先拷贝一份items_，减少加锁时间和粒度
				std::deque<std::wstring> items_copy;
				{
					std::lock_guard<std::mutex> items_lock(items_mtx);
					items_copy = this->items_;
				}

				// 局部变量做筛选，避免反复加锁
				std::deque<std::wstring> result;
				for (const auto& item : items_copy) {
					std::vector<std::wstring_view> pattern_views;
					// todo split_by_space 参数是引用, pat 可能在split_by_space中途执行过程中被外面修改
					std::vector<std::wstring> pats = string_util::split_by_space(pat);
					pattern_views.reserve(pats.size());
					for (const auto& s : pats)
						pattern_views.push_back(s);

					std::wstring_view item_view(item);
					if (ah_substring_match(pattern_views, item_view)) {
						result.push_back(item);
					}
				}
				// 一次性写回filtered_items_
				{
					std::lock_guard<std::mutex> filtered_lock(filtered_items_mtx);
					this->filtered_items_ = std::move(result);
				}
			}

			lock.lock();
			if (!stop_ && !newRequest_ && cb) {
				lock.unlock();
				cb(); // safe to signal GUI
			}
			else {
				lock.unlock();
			}

		}
		});
}
void HistoryManager::request_filter(const std::wstring& pat, FilterCallback filterDone) {
	std::lock_guard<std::mutex> lock(filter_mutex_);
	pending_pat_ = pat;
	pending_cb_ = filterDone;
	newRequest_ = true;
	cv_.notify_one();
}

void HistoryManager::add(const std::wstring& path) {
	if (path.empty()) return;
	//std::unordered_set<std::wstring> seen;
	//for (const std::wstring& item : items_) {
	//    if (seen.insert(item).second) {
	//        //listbox_add(item);
	//    }
	//}

	if (!items_.empty() && items_.front() == path) return;
	items_.erase(std::remove(items_.begin(), items_.end(), path), items_.end());

	items_.push_front(path);
	if (items_.size() > max_items_) items_.pop_back();
}
const std::deque<std::wstring>& HistoryManager::all() const {
	std::lock_guard<std::mutex> lock(filtered_items_mtx);
	return filtered_items_;
}


void HistoryManager::allWith(std::function<void(const std::deque<std::wstring>&)> cb) const {
	std::lock_guard<std::mutex> lock(filtered_items_mtx);
	cb(filtered_items_);
	//return filtered_items_;
}

void HistoryManager::save() {
	std::wofstream out(L"alfred_history.txt");
	for (const auto& s : items_) out << s << L"\n";
}
void HistoryManager::loadSync() {
	items_.clear();
	std::wifstream in(L"alfred_history.txt");
	std::wstring s;
	while (std::getline(in, s)) if (!s.empty()) items_.push_back(s);

	const std::vector<std::wstring> recentVec = sys_util::loadSystemRecent();
	items_.insert(items_.end(), recentVec.begin(), recentVec.end());
}

void HistoryManager::load_async_batch(std::function<void(std::vector<std::wstring>&)> on_batch) {
	std::thread([this, on_batch] {
		{
			std::lock_guard<std::mutex> lock(this->items_mtx);
			items_.clear();
		}
		std::vector<std::wstring> batch;
		auto flush = [&] {
			if (!batch.empty()) {
				{
					std::lock_guard<std::mutex> lk(this->items_mtx);
					this->items_.insert(this->items_.end(), batch.begin(), batch.end());
				}
				on_batch(batch);
				batch.clear();
			}
		};
		const size_t BATCH_SIZE = 100;
		auto last_flush = std::chrono::steady_clock::now();

		std::wifstream in(L"alfred_history.txt");
		if (in.is_open()) {
			std::wstring s;
			while (std::getline(in, s)) {
				if (!s.empty()) batch.push_back(s);
				if (batch.size() >= BATCH_SIZE ||
					(batch.size() && std::chrono::steady_clock::now() - last_flush > std::chrono::milliseconds(120))) {
					flush();
					last_flush = std::chrono::steady_clock::now();
				}
			}
		}
		for (const auto& r : sys_util::loadSystemRecent()) {
			batch.push_back(r);
			if (batch.size() >= BATCH_SIZE ||
				(batch.size() && std::chrono::steady_clock::now() - last_flush > std::chrono::milliseconds(120))) {
				flush();
				last_flush = std::chrono::steady_clock::now();
			}
		}
		flush(); // flush any remaining
		{
			std::lock_guard<std::mutex> lk(this->loaded_mtx);
			loaded_.store(true);
		}
		}).detach();
}

void HistoryManager::load_async(std::function< void() > on_loaded) {
	std::thread([this, cb = std::move(on_loaded)]() {
		try {
			{
				std::lock_guard<std::mutex> lock1(this->items_mtx);
				this->items_.clear();
			}
			{
				std::lock_guard<std::mutex> lock2(this->filtered_items_mtx);
				this->filtered_items_.clear();
			}
			std::deque<std::wstring> items, filtered_items;
			std::wifstream in(L"alfred_history.txt");
			if (in) {
				std::wstring s;
				while (std::getline(in, s)) if (!s.empty()) {
					items_.push_back(s);
					filtered_items_.push_back(s);
				}
			}

			for (const auto& r : sys_util::GetQuickAccessItems()) {
				items_.push_back(r);
				filtered_items_.push_back(r);
			}
			for (const auto& r : sys_util::loadSystemRecent()) {
				items_.push_back(r);
				filtered_items_.push_back(r);
			}

			// batch-insert with minmal locking
			{
				std::lock_guard<std::mutex> lock1(this->items_mtx);
				this->items_.insert(this->items_.end(), items.begin(), items.end());
			}
			{
				std::lock_guard<std::mutex> lock2(this->filtered_items_mtx);
				this->filtered_items_.insert(this->filtered_items_.end(), filtered_items.begin(), filtered_items.end());
			}
			//this->filtered_items_ = this->items_;
			if (cb) {
				try {
					cb();
				}
				catch (...) {
					//log error
				}
			}
			this->loaded_.store(true);
		}
		catch (const std::exception& e) {
			// log error
			//std::wcerr << L"Exception loading history: " << e.what() << std::endl;
		}
		catch (...) {
			// log Unknown exception in HistoryManager::load_async\n
		}
		}

	).detach();
}

bool HistoryManager::loaded_done() const {
	// Return true when load_async thread is finished and all expected items loaded
	// For simple apps, you can guard with a bool atomic loaded_ that the thread sets at end
	return loaded_.load();
}
void HistoryManager::filterModifyItemSync(const std::wstring& pat) {
	if (pat.empty()) {
		this->filtered_items_ = this->items_;
	}
	else {
		{
			this->filtered_items_.clear();
		}
		for (const auto& item : this->items_) {
			if (string_util::fuzzy_match(pat, item)) {
				this->filtered_items_.push_back(item);
			}
		}
	}
	//items_.erase(std::remove_if(items_.begin(), items_.end(), [&](const std::wstring& item) {
	//    return !string_util::fuzzy_match(pat, item);
	//    }), items_.end());
}
void HistoryManager::filterModifyItemThread(const std::wstring& pat, std::function<void()> filterDone) {
	std::thread([&, filterDone, this]() {
		if (pat.empty()) {
			std::lock_guard<std::mutex> lock(filtered_items_mtx);
			//std::lock_guard<std::mutex> lock2(items_mtx);
			this->filtered_items_ = this->items_; // Restore all
			// std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // Let server start
		}
		else {
			{
				std::lock_guard<std::mutex> lock(filtered_items_mtx);
				this->filtered_items_.clear();
			}
			// 先拷贝一份items_，减少加锁时间和粒度
			std::deque<std::wstring> items_copy;
			{
				std::lock_guard<std::mutex> items_lock(items_mtx);
				items_copy = this->items_;
			}

			// 局部变量做筛选，避免反复加锁
			std::deque<std::wstring> result;
			for (const auto& item : items_copy) {
				//if (string_util::fuzzy_match(pat, item)) {
				std::vector<std::wstring_view> pattern_views;
				// todo split_by_space 参数是引用, pat 可能在split_by_space中途执行过程中被外面修改
				std::vector<std::wstring> pats = string_util::split_by_space(pat);
				pattern_views.reserve(pats.size());
				for (const auto& s : pats)
					pattern_views.push_back(s);

				std::wstring_view item_view(item);
				if (ah_substring_match(pattern_views, item_view)) {
					result.push_back(item);
				}
			}

			// 一次性写回filtered_items_
			{
				std::lock_guard<std::mutex> filtered_lock(filtered_items_mtx);
				this->filtered_items_ = std::move(result);
			}
			//for (const auto& item : this->items_) {
			//	if (string_util::fuzzy_match(pat, item)) {
			//		std::lock_guard<std::mutex> lock(filtered_items_mtx);
			//		this->filtered_items_.push_back(item);
			//	}
			//}
		}
		filterDone();
		}).detach();
}

std::vector<const std::wstring*> HistoryManager::filter(const std::wstring& pat) const {
	std::vector<const std::wstring*> result;
	for (const auto& item : items_) {
		if (string_util::fuzzy_match(pat, item)) result.push_back(&item);
	}
	return result;
}

std::wstring HistoryManager::operator [] (size_t idx) const {
	std::lock_guard<std::mutex> lock(filtered_items_mtx);
	return filtered_items_[idx];
}

size_t HistoryManager::size() const {
	std::lock_guard<std::mutex> lock(filtered_items_mtx);
	return filtered_items_.size();
}
