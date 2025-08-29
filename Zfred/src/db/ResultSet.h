#pragma once
#include <sqlite3.h>
#include <vector>
#include <string>
#include <memory>

class ResultSet {
public:
    explicit ResultSet(sqlite3_stmt* stmt);
    ~ResultSet();

    ResultSet(const ResultSet&) = delete;
    ResultSet& operator=(const ResultSet&) = delete;
    ResultSet(ResultSet&&) noexcept;
    ResultSet& operator=(ResultSet&&) noexcept;

    bool next(); // Move to next row; false if done

    // Typed getters (by column index, 0-based)
    int getInt(int col) const;
    double getDouble(int col) const;
    std::string getString(int col) const;
    bool isNull(int col) const;
    int columnCount() const;
    std::string columnName(int col) const;

private:
    sqlite3_stmt* stmt_;
};
