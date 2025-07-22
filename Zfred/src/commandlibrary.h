#pragma once
#include <string>
#include <vector>
#include <functional>
#include <algorithm>

struct CommandItem {
    std::wstring keyword;
    std::wstring description;
    std::function<void()> action;
};

class CommandLibrary {
public:
    CommandLibrary();
    const std::vector<CommandItem>& getAllCommands() const;
    const std::vector<CommandItem>& all() const { return commands_; }

    std::vector<const CommandItem*> filter(const std::wstring& prefix) const;
private:
    std::vector<CommandItem> commands_;
};