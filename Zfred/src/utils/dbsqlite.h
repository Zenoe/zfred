#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <mutex>

#include <iostream>
#include <regex> // For table name validation
#include "stringutil.h"
#include "clipboard.h"
struct ClipItem;
class Database {
public:
	Database(const std::wstring& dbPath);
	~Database();

	ClipItem addItem(const std::wstring& content);

	bool removeItem(int id);
	int getItemCount() const;

	std::deque<ClipItem> getRecord(const std::wstring& tableName, int start = 0, int limit = 100);


private:
	sqlite3* db_ = nullptr;
	std::mutex mutable mtx_;
};
