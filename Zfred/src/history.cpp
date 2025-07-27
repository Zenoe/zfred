// history.cpp
#include "history.h"
#include <fstream>
#include "utils/sysutil.h"
#include <thread>
#include <unordered_set>
#include "utils/stringutil.h"

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
	std::lock_guard<std::mutex> lock(items_mtx);
	return filtered_items_;
}
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


void HistoryManager::load_async(std::function<void(std::vector<std::wstring>&)> on_batch) {
    return;
    //if (!items_.empty()) return;
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

//void load_async(void(*on_append)(const std::wstring&))
void HistoryManager::load_async(std::function< void() > on_loaded) {
	std::thread([this, on_loaded]() {
		{
			std::lock_guard<std::mutex> lock(this->items_mtx);
			this->items_.clear();
		}

        std::wifstream in(L"alfred_history.txt");
        std::wstring s;
        while (std::getline(in, s)) if (!s.empty()) {
			std::lock_guard<std::mutex> lock(this->items_mtx);
            this->items_.push_back(s);
        }

        for (const auto& r : sys_util::loadSystemRecent()) {
            {
                std::lock_guard<std::mutex> lk(this->items_mtx);
                this->items_.push_back(r);
            }
        }
        this->filtered_items_ = this->items_;
		on_loaded();
		loaded_.store(true);
		//const std::vector<std::wstring> recentVec = sys_util::loadSystemRecent();
		//{
		//	std::lock_guard<std::mutex> lock(this->items_mtx);
		//	items_.insert(items_.end(), recentVec.begin(), recentVec.end());
		//}
		}
	).detach();
}

bool HistoryManager::loaded_done() const{
    // Return true when load_async thread is finished and all expected items loaded
    // For simple apps, you can guard with a bool atomic loaded_ that the thread sets at end
    return loaded_.load();
}

void HistoryManager::filterModifyItem(const std::wstring& pat) {   
    std::thread([&, this]() {
		if (pat.empty()) {
			std::lock_guard<std::mutex> lock(filtered_items_mtx);
			//this->filtered_items_.resize(this->items_.size());
			this->filtered_items_ = this->items_; // Restore all
		}else {
            {
				std::lock_guard<std::mutex> lock(filtered_items_mtx);
				this->filtered_items_.clear();
            }
            for (const auto& item : this->items_) {
                if (string_util::fuzzy_match(pat, item)) {
					std::lock_guard<std::mutex> lock(filtered_items_mtx);
                    this->filtered_items_.push_back(item);
                }
            }
        }
        }).detach();
		//items_.erase(std::remove_if(items_.begin(), items_.end(), [&](const std::wstring& item) {
		//    return !string_util::fuzzy_match(pat, item);
		//    }), items_.end());
}

std::vector<const std::wstring*> HistoryManager::filter(const std::wstring& pat) const {   
    std::vector<const std::wstring*> result;
    for (const auto& item : items_) {
		if (string_util::fuzzy_match(pat, item)) result.push_back(&item);
    }
    return result;
}
