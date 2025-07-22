#pragma once
#include <windows.h>
#include <string>
#include "commandlibrary.h"
#include "filebrowser.h"
#include "history.h"
#include "bookmarks.h"
#include "utils/simpleundo.h"

enum class Mode { Command, FileBrowser, History, Bookmarks };

class MainWindow {
public:
    MainWindow(HINSTANCE hInstance);
    bool create();
    void set_mode(Mode mode);
    void show(bool visible);
    void run();

private:
    HINSTANCE hInstance_;
    HWND hwnd_, edit_, listbox_, combo_mode_;
    Mode mode_;
    int sel_;
    bool show_hidden_;

    CommandLibrary commands_;
    FileBrowser browser_;
    HistoryManager history_;
    BookmarkManager bookmarks_;
    
    SimpleUndo simpleundo_;

    
    // State: last input, for context
    std::wstring last_input_;

    void update_list();
    void parse_input(const std::wstring& text);

    void activate_command(int idx);
    void activate_filebrowser(int idx);
    void activate_history(int idx);
    void activate_bookmarks(int idx);

    void file_actions_menu(const std::wstring& path);
    void autofill_input_by_selection();

    void save_all();

    const LRESULT& processAltBackspace();
    const LRESULT& processBackspace();

    const LRESULT& processWMCommand(WPARAM wpParam);
    LRESULT undo_delete_word();

    static MainWindow* self;
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
    static LRESULT CALLBACK EditProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
};;