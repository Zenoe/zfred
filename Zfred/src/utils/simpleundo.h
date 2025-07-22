#pragma once
#include <vector>
#include <string>
#include <optional>

class SimpleUndo
{
public:
	SimpleUndo(size_t s = 10) : max_size(s) { undo_stack.reserve(max_size); }
	void push(const std::wstring& text, const size_t caret);
	std::optional<std::pair<std::wstring, size_t>> pop();
	SimpleUndo(const SimpleUndo&) = delete;
	SimpleUndo& operator=(const SimpleUndo&) = delete;
	//bool pop(std::wstring& text, size_t& caret);
	bool can_undo() const { return !undo_stack.empty(); }
	void clear();
private:
	std::vector<std::pair<std::wstring, size_t>>  undo_stack;
	//std::vector<std::pair<std::wstring, size_t>>  redo_stack;
	size_t max_size = 10;

};

