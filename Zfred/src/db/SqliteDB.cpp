#include "SqliteDB.h"
#include "ResultSet.h"

SqliteDB::SqliteDB(const std::string& filename) : db_(nullptr) {
    if (sqlite3_open(filename.c_str(), &db_) != SQLITE_OK) {
        std::string msg = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        throw SqliteException("SQLite open error: " + msg);
    }
}

SqliteDB::~SqliteDB() {
    if (db_) sqlite3_close(db_);
}

SqliteDB::SqliteDB(SqliteDB&& other) noexcept : db_(other.db_) {
    other.db_ = nullptr;
}
SqliteDB& SqliteDB::operator=(SqliteDB&& other) noexcept {
    if (this != &other) {
        if (db_) sqlite3_close(db_);
        db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

int SqliteDB::execute(const std::string& sql, const std::vector<std::string>& params) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        throw SqliteException("SQL error: " + std::string(sqlite3_errmsg(db_)));

    for (size_t i = 0; i < params.size(); ++i) {
        if (sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
            throw SqliteException("Bind error: " + std::string(sqlite3_errmsg(db_)));
    }

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw SqliteException("Execution error: " + std::string(sqlite3_errmsg(db_)));
    }
    int changes = sqlite3_changes(db_);
    sqlite3_finalize(stmt);
    return changes;
}

std::unique_ptr<ResultSet> SqliteDB::query(const std::string& sql, const std::vector<std::string>& params) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        throw SqliteException("SQL error: " + std::string(sqlite3_errmsg(db_)));

    for (size_t i = 0; i < params.size(); ++i) {
        if (sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
            sqlite3_finalize(stmt);
            throw SqliteException("Bind error: " + std::string(sqlite3_errmsg(db_)));
        }
    }
    return std::make_unique<ResultSet>(stmt);
}
