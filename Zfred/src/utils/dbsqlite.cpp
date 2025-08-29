#include "dbsqlite.h"
#include <iostream>
#include <regex> // For table name validation
#include "stringutil.h"

Database::Database(const std::wstring& dbPath) {
   std::string dbPath8 = string_util::wstring_to_utf8(dbPath);
   if (sqlite3_open(dbPath8.c_str(), &db_) != SQLITE_OK) {
       db_ = nullptr;
       std::cerr << "Failed to open database!\n";
   } else {
       const char* sql =
           "CREATE TABLE IF NOT EXISTS clipboard_items("
           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "content TEXT,"
           "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
           ");";
       char* err;
       if (sqlite3_exec(db_, sql, 0, 0, &err) != SQLITE_OK) {
           std::cerr << "Failed to create table: " << err << std::endl;
           sqlite3_free(err);
       }
   }
}

Database::~Database() {
   if (db_) sqlite3_close(db_);
}

ClipItem Database::addItem(const std::wstring& content) {
		const char* sql = "INSERT INTO clipboard_items(content) VALUES (?);";
		sqlite3_stmt* stmt;
		std::lock_guard<std::mutex> lock(mtx_);
		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL) != SQLITE_OK) return {};
		sqlite3_bind_text(stmt, 1, string_util::wstring_to_utf8(content).c_str(), -1, SQLITE_TRANSIENT);
		bool ok = sqlite3_step(stmt) == SQLITE_DONE;
		sqlite3_finalize(stmt);
		if(!ok) return {};

		sqlite3_int64 rowid = sqlite3_last_insert_rowid(db_);
		// SELECT row back
		ClipItem item;
		const char* sql_select = "SELECT id, content FROM clipboard_items WHERE id = ?;";
		sqlite3_stmt* select_stmt;
		if (sqlite3_prepare_v2(db_, sql_select, -1, &select_stmt, NULL) == SQLITE_OK) {
			sqlite3_bind_int64(select_stmt, 1, rowid);
			if (sqlite3_step(select_stmt) == SQLITE_ROW) {
				item.id_ = sqlite3_column_int(select_stmt, 0);
				const char* content_utf8 = (const char*)sqlite3_column_text(select_stmt, 1);
				item.content = string_util::utf8_to_wstring(content_utf8);
			}
			sqlite3_finalize(select_stmt);
		}
		return item;
}


bool Database::removeItem(int id) {
  sqlite3_stmt *stmt = nullptr;
  const char* sql = "DELETE FROM clipboard_items WHERE id = ?";
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }
  sqlite3_bind_int(stmt, 1, id);
  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  // SQLITE_DONE means no result rows, but completed successfully
  return rc == SQLITE_DONE;
}

int Database::getItemCount() const {
		std::lock_guard<std::mutex> lock(mtx_);
		int count = 0;
		const char* sql = "SELECT COUNT(*) FROM clipboard_items;";
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL) == SQLITE_OK) {
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				count = sqlite3_column_int(stmt, 0);
			}
		}
		sqlite3_finalize(stmt);
		return count;
}

std::deque<ClipItem> Database::getRecord(const std::wstring& tableName, int start, int limit) {

		std::deque<ClipItem> items;

		// Validate table name is safe (alphanumeric + underscore)
		std::wstring tableNameSafe = tableName;
		if (!std::regex_match(tableNameSafe, std::wregex(LR"([a-zA-Z_][a-zA-Z0-9_]*)"))) {
			// Log or throw error here.
			return items;
		}

		// Convert wstring to UTF-8 for SQLite
		std::string tableNameUtf8 = string_util::wstring_to_utf8(tableNameSafe);

		// Compose SQL statement
		std::string sql = "SELECT id, content, timestamp FROM \"" + tableNameUtf8 +
			"\" ORDER BY id DESC LIMIT ? OFFSET ?";

		sqlite3_stmt* stmt = nullptr;
		{
			std::lock_guard<std::mutex> lock(mtx_);
			if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
				// Optional: log error with sqlite3_errmsg(db_);
				return items;
			}

			sqlite3_bind_int(stmt, 1, limit);
			sqlite3_bind_int(stmt, 2, start);

			while (sqlite3_step(stmt) == SQLITE_ROW) {
				items.push_back(ClipItem{
					sqlite3_column_int(stmt, 0),
					string_util::utf8_to_wstring(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))),
					string_util::utf8_to_wstring(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)))
					});
			}
			sqlite3_finalize(stmt);
		}
		return items;
	}
