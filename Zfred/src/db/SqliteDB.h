#pragma once
#include <sqlite3.h>
#include <memory>
#include <string>
#include <mutex>
#include <vector>
#include "SqliteException.h"

// Forward declaration for ResultSet
class ResultSet;

class SqliteDB {
public:
    explicit SqliteDB(const std::string& filename);
    ~SqliteDB();

    // Non-copyable, but moveable
    SqliteDB(const SqliteDB&) = delete;
    SqliteDB& operator=(const SqliteDB&) = delete;
    SqliteDB(SqliteDB&&) noexcept;
    SqliteDB& operator=(SqliteDB&&) noexcept;

    // Execute update (INSERT, UPDATE, DELETE)
    int execute(const std::string& sql, const std::vector<std::string>& params = {});
    // Query (SELECT), returns a ResultSet
    std::unique_ptr<ResultSet> query(const std::string& sql, const std::vector<std::string>& params = {});

private:
    sqlite3* db_;
    std::mutex mutex_;
};
