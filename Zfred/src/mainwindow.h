#pragma once
#include <windows.h>
#include <string>
#include "commandlibrary.h"
#include "filebrowser.h"
#include "history.h"
#include "bookmarks.h"
#include "utils/simpleundo.h"
#include "clipboard.h"

#define WM_HISTORY_LOADED (WM_USER + 101)
#define WM_DEBOUNCED_UPDATE_LIST (WM_USER + 102)
#define SPINNER_TIMER_ID 1001

enum class Mode { History, FileBrowser, Bookmarks, Clipboard, Count };
inline const wchar_t* ModeToString(Mode mode) {
	switch (mode) {
	case Mode::FileBrowser:  return L"FileBrowser";
	case Mode::History:      return L"History";
	case Mode::Bookmarks:    return L"Bookmarks";
	case Mode::Clipboard:    return L"Clipboard";
	default:                 return L"Unknown";
	}
}
class MainWindow {
public:
    MainWindow(HINSTANCE hInstance);
    bool create();
    void show(bool visible);
    void run();
    HWND GetHwnd() const { return hwnd_; }

private:
    HINSTANCE hInstance_;
    HWND hwnd_, edit_, listbox_, combo_mode_;
    HWND hListview_ = nullptr;
    Mode mode_;
    int sel_;
    bool show_hidden_;

    int spinner_frame_ = 0;
    CommandLibrary commands_;
    FileBrowser browser_;
    HistoryManager history_;
    BookmarkManager bookmarks_;
    Clipboard clipboard_;
    
    SimpleUndo simpleundo_;

    std::atomic<bool> queued_update_ = false;
    std::atomic<bool> show_spinner_ = false;
    std::chrono::steady_clock::time_point spinner_start_time_;
    
    // State: last input, for context
    std::wstring last_input_;

    std::vector<std::wstring> uiCachePage;
    std::atomic<int> cachePageStart{ 0 };
    void updateUICachePage(int startRow);

    void update_listview();
    //void update_list();
    void parse_input(const std::wstring& text);

    void activate_filebrowser(int idx);
    void activate_history(int idx);
    void activate_bookmarks(int idx);
    void activate_clipboard(int idx);

    void file_actions_menu(const std::wstring& path);
    void autofill_input_by_selection();

    void save_all();

    void selectListview(int sel);
    LRESULT processAltBackspace();
    LRESULT processChar(UINT msg,WPARAM wParam,  LPARAM lParam);
    LRESULT processBackspace();
    LRESULT processAppendHistory();

    LRESULT processCustomContextMenu();
    //bool processListViewContent(LPARAM lParam, const std::vector<std::wstring>& items);
    void processListViewContent(LPARAM lParam);
    void update_spinner();


    LRESULT switchMode(WPARAM wParam);
    void processListviewNavigation(int direction);
    // the context just likt the explorer's
    LRESULT processContextMenu(WPARAM wParam, LPARAM lParam);
    LRESULT processWMCommand(WPARAM wpParam);
    LRESULT processDrawItem(LPARAM lParam);
    LRESULT processHotkey(WPARAM wParam);

    void processReturn();
    LRESULT undo_delete_word();

    static MainWindow* self;
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
    static LRESULT CALLBACK EditProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    static LRESULT CALLBACK ListviewProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    static LRESULT CALLBACK FlatComboSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR);

};
