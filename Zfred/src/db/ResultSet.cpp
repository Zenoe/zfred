#include "ResultSet.h"
#include "SqliteException.h"

ResultSet::ResultSet(sqlite3_stmt* stmt) : stmt_(stmt) {}

ResultSet::~ResultSet() {
    if (stmt_) sqlite3_finalize(stmt_);
}

ResultSet::ResultSet(ResultSet&& other) noexcept : stmt_(other.stmt_) {
    other.stmt_ = nullptr;
}
ResultSet& ResultSet::operator=(ResultSet&& other) noexcept {
    if (this != &other) {
        if (stmt_) sqlite3_finalize(stmt_);
        stmt_ = other.stmt_;
        other.stmt_ = nullptr;
    }
    return *this;
}

bool ResultSet::next() {
    int rc = sqlite3_step(stmt_);
    return rc == SQLITE_ROW;
}

int ResultSet::getInt(int col) const {
    if (sqlite3_column_type(stmt_, col) == SQLITE_NULL)
        throw SqliteException("Trying to get int from NULL column");
    return sqlite3_column_int(stmt_, col);
}
double ResultSet::getDouble(int col) const {
    if (sqlite3_column_type(stmt_, col) == SQLITE_NULL)
        throw SqliteException("Trying to get double from NULL column");
    return sqlite3_column_double(stmt_, col);
}
std::string ResultSet::getString(int col) const {
    if (sqlite3_column_type(stmt_, col) == SQLITE_NULL)
        return "";
    const unsigned char* text = sqlite3_column_text(stmt_, col);
    return text ? reinterpret_cast<const char*>(text) : "";
}
bool ResultSet::isNull(int col) const {
    return sqlite3_column_type(stmt_, col) == SQLITE_NULL;
}
int ResultSet::columnCount() const {
    return sqlite3_column_count(stmt_);
}
std::string ResultSet::columnName(int col) const {
    const char* name = sqlite3_column_name(stmt_, col);
    return name ? name : "";
}
