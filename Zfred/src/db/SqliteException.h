#pragma once
#include <stdexcept>
#include <string>

class SqliteException : public std::runtime_error {
public:
    explicit SqliteException(const std::string& what_arg)
        : std::runtime_error(what_arg) {}
};
