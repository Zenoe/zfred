#include "history.h"
#include <fstream>
#include "utils/sysutil.h"
#include <thread>
#include <algorithm>
#include <unordered_set>
#include "utils/stringutil.h"
#include "utils/ahocorasick.h"

HistoryManager::HistoryManager(){
	filter_worker_ = std::thread([this] {
		while (!stop_) {
			std::unique_lock<std::mutex> lock(filtered_items_mtx);
			cv_.wait(lock, [this] { return stop_ || newRequest_; });
			if (stop_) { break; }
			auto pat = pending_pat_;
			auto cb = pending_cb_;
			newRequest_ = false;
			lock.unlock();

			if (pat.empty()) {
				std::lock_guard<std::mutex> lock1(items_mtx);
				std::lock_guard<std::mutex> lock(filtered_items_mtx);
				//this->filtered_items_ = this->items_;// shallow copy
				if (!items_) {
					MessageBox(nullptr, L"items_ is null", L"caption", 0);
					continue;
				}
				filtered_items_ = std::make_shared<std::deque<std::wstring>>(*items_);
			}
			else {
				// 先拷贝一份items_，减少加锁时间和粒度
				std::deque<std::wstring> items_copy;
				{
					std::lock_guard<std::mutex> items_lock(items_mtx);
					items_copy = *items_;
				}
				{
					std::lock_guard<std::mutex> lock(filtered_items_mtx);
					filtered_items_->clear();
				}

				// 局部变量做筛选，避免反复加锁
				std::deque<std::wstring> result;
				auto t0 = std::chrono::steady_clock::now();
				std::vector<std::wstring_view> pattern_views;
				// split_by_space 参数是引用, pat是局部拷贝了一份成员变量，所以不会在split_by_space中途执行过程中被外面修改
				std::vector<std::wstring> pats = string_util::split_by_space(pat);
				pattern_views.reserve(pats.size());
				for (const auto& s : pats)
					pattern_views.push_back(s);

				// Build Aho-Corasick only ONCE. move it out of the for loop
				AhoCorasick<wchar_t> ac;
				ac.build(pattern_views);

				for (const auto& item : items_copy) {
					std::wstring_view item_view(item);

					// Inline the check instead of building every time
					std::vector<bool> found(pattern_views.size(), false);
					ac.search(item_view, found);
					if (std::all_of(found.begin(), found.end(), [](bool b) {return b; })) {
						result.push_back(item);
					}
				}
				auto t1 = std::chrono::steady_clock::now();

				OutputDebugPrint("time const: ", std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
				// 一次性写回filtered_items_
				{

					std::lock_guard<std::mutex> filtered_lock(filtered_items_mtx);
					this->filtered_items_ = std::make_shared<std::deque<std::wstring>>(std::move(result));
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

	if (!items_->empty() && items_->front() == path) return;
	items_->erase(std::remove(items_->begin(), items_->end(), path), items_->end());

	items_->push_front(path);
	if (items_->size() > max_items_) items_->pop_back();
}
const std::deque<std::wstring>& HistoryManager::all() const {

	std::lock_guard<std::mutex> lock(filtered_items_mtx);
	return *filtered_items_;
}

//std::shared_ptr<const std::deque<std::wstring>> HistoryManager::all() const {
//	std::lock_guard<std::mutex> lock(filtered_items_mtx);
//	return filtered_items_;
//}

void HistoryManager::allWith(std::function<void(const std::deque<std::wstring>&)> cb) const {
	// might use shared_mutex (read-lock) ?
	//std::lock_guard<std::mutex> lock(filtered_items_mtx);
	//cb(filtered_items_);
	std::shared_ptr<const std::deque<std::wstring>> snapshot;
	{
		OutputDebugPrint("lockfiltered_items_mtx");
		std::lock_guard<std::mutex> lock(filtered_items_mtx);
		//if(!filtered_items_)
		//	MessageBox(nullptr, L"filter items_ is null", L"caption", 0);

		snapshot = filtered_items_; // Refcount increment
	}
	OutputDebugPrint("lockfiltered_items_mtx endddddddd");
	cb(*snapshot);
	//return filtered_items_;
}

void HistoryManager::save() {
	std::wofstream out(L"alfred_history.txt");
	for (const auto& s : *items_) out << s << L"\n";
}
void HistoryManager::loadSync() {
	items_->clear();
	std::wifstream in(L"alfred_history->txt");
	std::wstring s;
	while (std::getline(in, s)) if (!s.empty()) items_->push_back(s);

	const std::vector<std::wstring> recentVec = sys_util::loadSystemRecent();
	items_->insert(items_->end(), recentVec.begin(), recentVec.end());
}

void HistoryManager::load_async_batch(std::function<void(std::vector<std::wstring>&)> on_batch) {
	std::thread([this, on_batch] {
		{
			std::lock_guard<std::mutex> lock(this->items_mtx);
			items_->clear();
		}
		std::vector<std::wstring> batch;
		auto flush = [&] {
			if (!batch.empty()) {
				{
					std::lock_guard<std::mutex> lk(this->items_mtx);
					items_->insert(items_->end(), batch.begin(), batch.end());
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
				items_->clear();
				std::lock_guard<std::mutex> lock2(this->filtered_items_mtx);
				this->filtered_items_->clear();
				std::deque<std::wstring> items, filtered_items;
				std::wifstream in(L"alfred_history.txt");
				if (in) {
					std::wstring s;
					while (std::getline(in, s)) if (!s.empty()) {
						this->items_->push_back(s);
						this->filtered_items_->push_back(s);
					}
				}

				for (const auto& r : sys_util::GetQuickAccessItems()) {
					items_->push_back(r);
					filtered_items_->push_back(r);
				}
				for (const auto& r : sys_util::loadSystemRecent()) {
					items_->push_back(r);
					filtered_items_->push_back(r);
				}

				// batch-insert with minmal locking
				{
					std::lock_guard<std::mutex> lock1(this->items_mtx);
					this->items_->insert(this->items_->end(), items.begin(), items.end());
				}
				{

					std::lock_guard<std::mutex> lock2(this->filtered_items_mtx);
					this->filtered_items_->insert(this->filtered_items_->end(), filtered_items.begin(), filtered_items.end());
				}
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
		//catch (const std::exception& e) {
		//	std::wcerr << L"Exception loading history: " << e.what() << std::endl;
		//}
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
		filtered_items_ = items_;
	}
	else {
		filtered_items_->clear();
		for (const auto& item : *items_) {
			if (string_util::fuzzy_match(pat, item)) {
				filtered_items_->push_back(item);
			}
		}
	}
	//items_.erase(std::remove_if(items_.begin(), items_.end(), [&](const std::wstring& item) {
	//    return !string_util::fuzzy_match(pat, item);
	//    }), items_.end());
}
// move the filterModifyItemThread function to a named thread in ctor. in order to join it in dtor
/*
void HistoryManager::filterModifyItemThread(const std::wstring& pat, std::function<void()> filterDone) {
	std::thread([&, filterDone, this]() {
		if (pat.empty()) {
			std::lock_guard<std::mutex> lock(filtered_items_mtx);
			//std::lock_guard<std::mutex> lock2(items_mtx);
			this->filtered_items_ = this->items_; // Restore all
		}
		else {
			{
				std::lock_guard<std::mutex> lock(filtered_items_mtx);
				this->filtered_items_->clear();
			}
			// 先拷贝一份items_，减少加锁时间和粒度
			std::deque<std::wstring> items_copy;
			{
				std::lock_guard<std::mutex> items_lock(items_mtx);
				items_copy = *items_;
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
				this->filtered_items_ = std::make_shared<std::deque<std::wstring>>(std::move(result));
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
}*/

std::vector<const std::wstring*> HistoryManager::filter(const std::wstring& pat) const {
	std::vector<const std::wstring*> result;
	for (const auto& item : *items_) {
		if (string_util::fuzzy_match(pat, item)) result.push_back(&item);
	}
	return result;
}

std::wstring HistoryManager::operator [] (size_t idx) const {
	std::lock_guard<std::mutex> lock(filtered_items_mtx);
	return ( * filtered_items_)[idx];
}

size_t HistoryManager::size() const {
	std::lock_guard<std::mutex> lock(filtered_items_mtx);
	return filtered_items_->size();
}

std::vector<std::wstring> HistoryManager::get_page(int startRow, size_t pageSize) {
	std::vector<std::wstring> results; 
	if (!filtered_items_ || startRow > filtered_items_->size()) return results;
	std::lock_guard<std::mutex> lock(filtered_items_mtx);
	size_t end = (std::min)(filtered_items_->size(), pageSize+startRow);
	results.reserve(end - startRow);
	auto it = filtered_items_->begin() + startRow;
	for (size_t i = startRow; i < end; ++i, ++it) {
		results.push_back(*it);
	}

	return results;
}
