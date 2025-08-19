#include "dbsqlite.h"
#include <iostream>
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

bool Database::addItem(const std::wstring& content) {
    const char* sql = "INSERT INTO clipboard_items(content) VALUES (?);";
    sqlite3_stmt* stmt;
    std::lock_guard<std::mutex> lock(mtx_);
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, string_util::wstring_to_utf8(content).c_str(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
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

std::vector<ClipItem> Database::getRecentItems(int limit) {
    std::vector<ClipItem> items;
    std::string sql = "SELECT id, content, timestamp FROM clipboard_items ORDER BY id DESC LIMIT ?";
    sqlite3_stmt* stmt;
    std::lock_guard<std::mutex> lock(mtx_);
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) return items;
    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        items.push_back(ClipItem{
            sqlite3_column_int(stmt, 0),
            string_util::utf8_to_wstring(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))),
            string_util::utf8_to_wstring(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)))
        });
    }
    sqlite3_finalize(stmt);
    return items;
}