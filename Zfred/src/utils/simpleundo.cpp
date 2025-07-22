#include "simpleundo.h"

void SimpleUndo::push(const std::wstring & text, const size_t caret) {
	if (undo_stack.size() == max_size) {
		undo_stack.erase(undo_stack.begin());
	}
		// push_back do the copy
	undo_stack.push_back({ text, caret });
	//redo_stack.clear();
}

//bool SimpleUndo::pop(std::wstring& text, size_t& caret) {
//	if (undo_stack.empty()) return false;
//	text = undo_stack.back().first;
//	caret = undo_stack.back().second;
//	undo_stack.pop_back();
//	return true;
//}

void SimpleUndo::clear() {
	undo_stack.clear();
}
std::optional<std::pair<std::wstring, size_t>> SimpleUndo::pop() {
	if (undo_stack.size() > 0) {
		auto last = undo_stack.back();
		//redo_stack.push_back(last);
		undo_stack.pop_back();
		return  last;
	}
	return std::nullopt;
}
