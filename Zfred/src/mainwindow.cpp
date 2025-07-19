#include "mainwindow.h"
#include "fileactions.h"
#include <commctrl.h>
#include <shlwapi.h>
#include <cwctype>
#include <vector>
#include <sstream>
#include <algorithm>
#include <dwmapi.h> // For DwmExtendFrameIntoClientArea (link with dwmapi.lib)
#ifdef _DEBUG
#include "debugtool.h"
#endif

#include "constant.h"
#include "utils/stringutil.h"

//#include "guihelper.h"

// static instance pointer
MainWindow* MainWindow::self = nullptr;

MainWindow::MainWindow(HINSTANCE hInstance)
    : hInstance_(hInstance), hwnd_(nullptr), edit_(nullptr), listbox_(nullptr),
    mode_(Mode::Command), sel_(0), show_hidden_(false), last_input_(L"")
{
    self = this;
    history_.load();
    bookmarks_.load();
}

bool MainWindow::create() {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &MainWindow::WndProc;
    wc.hInstance = hInstance_;
    wc.lpszClassName = L"zfredwnd";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassExW(&wc);

    int x = (GetSystemMetrics(SM_CXSCREEN) - WND_W) / 2, y = 100;
    LONG exStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED;
    hwnd_ = CreateWindowExW(exStyle,
        wc.lpszClassName, L"zfred",
        WS_POPUP | WS_CLIPCHILDREN, x, y, WND_W, WND_H,
        nullptr, nullptr, hInstance_, nullptr);
    if (!hwnd_) return false;

    // this would make the corner jagged
    //GuiHelper::SetWindowRoundRgn(hwnd_, WND_W, WND_H, 14);

    // --------- Make Background White and Slightly Transparent -----------
    SetLayeredWindowAttributes(hwnd_, 0, 245, LWA_ALPHA); // 240/255 opacity (more readable than 200)

    MARGINS margins = { 0, 0, 0, 0 };
    DwmExtendFrameIntoClientArea(hwnd_, &margins); // No glass
    // To use complete alpha (blur glass) on Win10+:
    // a “weird” border or black spots around the EDIT control when your window uses extended styles like WS_EX_LAYERED or enables DWM glass (blur) via DwmExtendFrameIntoClientArea. The issue:
    //Standard controls like EDIT do not draw their own fully - opaque backgrounds or borders.
    //    With glass / transparent layered windows, what’s behind the control can bleed through, especially at the border / shadow.
    //MARGINS margins = { -1 };
    //DwmExtendFrameIntoClientArea(hwnd_, &margins); // Enable "Aero" glass background

    edit_ = CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NOHIDESEL | ES_LEFT | WS_BORDER, // Add flat border,
        PADDING, PADDING, WND_W - (PADDING << 1), EDIT_H, hwnd_, (HMENU)1, hInstance_, nullptr);

    // Use nice size and font (Segoe UI, 18px height)
    HFONT hFont = CreateFontW(
        18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    SendMessageW(edit_, WM_SETFONT, (WPARAM)hFont, TRUE);

    listbox_ = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | LBS_DISABLENOSCROLL,
        PADDING, PADDING+EDIT_H+ EDIT_LIST_MARGIN, LIST_W, 0,
        hwnd_, (HMENU)2, hInstance_, nullptr);

    SendMessageW(listbox_, WM_SETFONT, (WPARAM)hFont, TRUE);

    SetWindowSubclass(edit_, &MainWindow::EditProc, 0, 0);

    //ShowWindow(listbox_, SW_HIDE);
    return true;
}

void MainWindow::show(bool visible) {
    ShowWindow(hwnd_, visible ? SW_SHOW : SW_HIDE);
    if (visible) {
        SetForegroundWindow(hwnd_);
        SetActiveWindow(hwnd_);
        SetWindowTextW(edit_, L"");
        SetFocus(edit_);
        mode_ = Mode::Command;
        last_input_.clear();
        update_list();
    }
}

void MainWindow::run() {
    RegisterHotKey(hwnd_, 1, MOD_ALT, VK_SPACE);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    UnregisterHotKey(hwnd_, 1);
}

void MainWindow::save_all() {
    history_.save();
    bookmarks_.save();
}

void MainWindow::update_list() {
    SendMessageW(listbox_, LB_RESETCONTENT, 0, 0);
    sel_ = 0;
#ifdef _DEBUG
    OutputDebugPrint("last_input: " , last_input_.c_str());
#endif
    if (mode_ == Mode::Command) {
       if (last_input_.empty() && !history_.all().empty()) {
            // Recent history
            for (const auto& h : history_.all()) {
                std::wstring label = (PathIsDirectoryW(h.c_str()) ? L"🗂 " : L"🗎 ") + h;
                SendMessageW(listbox_, LB_ADDSTRING, 0, (LPARAM)label.c_str());
            }
        }
        else {
            auto matches = commands_.filter(last_input_);
            for (const auto* cmd : matches) {
                std::wstring line = L"⚡ " + cmd->keyword + L"    — " + cmd->description;
                SendMessageW(listbox_, LB_ADDSTRING, 0, (LPARAM)line.c_str());
            }
        }
    }
    else if (mode_ == Mode::FileBrowser) {
        //OutputDebugPrintVev( browser_.results());
        for (const auto& e : browser_.results()) {
            std::wstring disp;
            if (e.is_parent) {
                //OutputDebugPrint("e is paent  ", e.fullpath);
                continue;
                //disp = L"⤴ ..";
            }
            else if (e.is_dir && e.is_hidden) disp = L"[HID][DIR] " + e.label;
            else if (e.is_dir) disp = L"[DIR] " + e.label;
            else if (e.is_hidden) disp = L"[HID] " + e.label;
            else disp = L"   " + e.label;
            SendMessageW(listbox_, LB_ADDSTRING, 0, (LPARAM)disp.c_str());
        }
    }
    else if (mode_ == Mode::History) {
        for (const auto& h : history_.all()) {
            std::wstring label = (PathIsDirectoryW(h.c_str()) ? L"🗂 " : L"🗎 ") + h;
            SendMessageW(listbox_, LB_ADDSTRING, 0, (LPARAM)label.c_str());
        }
    }
    else if (mode_ == Mode::Bookmarks) {
        for (const auto& b : bookmarks_.all()) {
            std::wstring label = (PathIsDirectoryW(b.c_str()) ? L"🧡 " : L"♥ ") + b;
            SendMessageW(listbox_, LB_ADDSTRING, 0, (LPARAM)label.c_str());
        }
    }
    int n = (int)SendMessageW(listbox_, LB_GETCOUNT, 0, 0);
    sel_ = (n > 0) ? 0 : -1;

	if (n > 0) {
        // can be history or file/folder list
		int item_height = (int)SendMessageW(listbox_, LB_GETITEMHEIGHT, 0, 0);

		int visible_items = (n < MAX_ITEMS) ? n : MAX_ITEMS;
		int list_height = (item_height * visible_items) + LIST_BOTTOM_PADDING;
		int total_height = WND_H + EDIT_LIST_MARGIN + list_height;

		SetWindowPos(hwnd_, NULL, 0, 0, WND_W, total_height, SWP_NOMOVE | SWP_NOZORDER);
		SetWindowPos(listbox_, NULL, 0, 0, LIST_W, list_height, SWP_NOMOVE | SWP_NOZORDER);

		ShowWindow(listbox_, SW_SHOW);
	}
	else {
		ShowWindow(listbox_, SW_HIDE);
		SetWindowPos(hwnd_, NULL, 0, 0, WND_W, WND_H, SWP_NOMOVE | SWP_NOZORDER);
	}
    if (sel_ >= 0)
        SendMessageW(listbox_, LB_SETCURSEL, sel_, 0);
}

void MainWindow::parse_input(const std::wstring& text) {
    last_input_ = text;
    if (text == L":b" || text == L"bm" || text == L"bookmarks") {
        mode_ = Mode::Bookmarks;
        update_list();
        return;
    }
    else if (text.empty()) {
        mode_ = Mode::History;
        update_list();
        return;
    }
    else if (text.size() >= 2 && text[1] == L':' && (text[2] == L'/' || text[2] == L'\\')) {
        // Looks like C:\ or C:/ path
        mode_ = Mode::FileBrowser;
        size_t lastsep = text.find_last_of(L"\\/");
        std::wstring dir = (lastsep != std::wstring::npos) ? text.substr(0, lastsep + 1) : text;
        std::wstring pat = (lastsep != std::wstring::npos) ? text.substr(lastsep + 1) : L"";
        browser_.set_cwd(dir);
        browser_.update(pat, show_hidden_);
        update_list();
        return;
    }
    // default: command mode
    mode_ = Mode::Command;
    update_list();
}

// Activation methods: what to do when Enter/Double-click
void MainWindow::activate_command(int idx) {
    auto matches = commands_.filter(last_input_);
    if (!last_input_.empty() && idx >= 0 && idx < (int)matches.size()) {
        matches[idx]->action();
        show(false);
    }
    else if (last_input_.empty() && idx >= 0 && idx < (int)history_.all().size()) {
        std::wstring sel = history_.all()[idx];
        if (PathIsDirectoryW(sel.c_str())) {
            mode_ = Mode::FileBrowser;
            browser_.set_cwd(sel);
            browser_.update(L"", show_hidden_);
            SetWindowTextW(edit_, sel.c_str());
            update_list();
        }
        else {
            fileactions::launch(sel);
            show(false);
        }
        history_.add(sel);
    }
    else {
        // just run
        if (!last_input_.empty()) {
            auto result = (INT_PTR)ShellExecuteW(nullptr, L"open", last_input_.c_str(), nullptr, nullptr, SW_SHOW);
            if (result <= 32) {
                MessageBoxW(hwnd_, (L"'" + last_input_ + L"' was not found in system PATH or could not be executed.").c_str(),
                    L"Command failed", MB_OK | MB_ICONWARNING);
            }
        }
    }
}
void MainWindow::activate_filebrowser(int idx) {
    const auto& results = browser_.results();
    if (idx >= 0 && idx < (int)results.size()) {
        //const auto& e = results[idx];
        // copy, cause results may be clear by browser_.update()
        const auto e = results[idx];
        if (e.is_parent || e.is_dir) {
            browser_.set_cwd(e.fullpath + L"\\");
            browser_.update(L"", show_hidden_);
            SetWindowTextW(edit_, browser_.cwd().c_str());
            update_list();
            history_.add(e.fullpath);
        }
        else if (e.is_file) {
            fileactions::launch(e.fullpath);
            history_.add(e.fullpath);
            show(false);
        }
    }
}
void MainWindow::activate_history(int idx) {
    auto hist = history_.all();
    if (idx >= 0 && idx < (int)hist.size()) {
        std::wstring sel = hist[idx];
        if (PathIsDirectoryW(sel.c_str())) {
            mode_ = Mode::FileBrowser;
            browser_.set_cwd(sel);
            browser_.update(L"", show_hidden_);
            SetWindowTextW(edit_, sel.c_str());
            update_list();
        }
        else {
            fileactions::launch(sel);
            show(false);
        }
        history_.add(sel);
    }
}
void MainWindow::activate_bookmarks(int idx) {
    auto bm = bookmarks_.all();
    if (idx >= 0 && idx < (int)bm.size()) {
        std::wstring sel = bm[idx];
        if (PathIsDirectoryW(sel.c_str())) {
            mode_ = Mode::FileBrowser;
            browser_.set_cwd(sel);
            browser_.update(L"", show_hidden_);
            SetWindowTextW(edit_, sel.c_str());
            update_list();
        }
        else {
            fileactions::launch(sel);
            show(false);
        }
        history_.add(sel);
    }
}

void MainWindow::autofill_input_by_selection() {
    std::wstring text;
    if (mode_ == Mode::FileBrowser) {
        const auto& entries = browser_.results();
        if (sel_ >= 0 && sel_ < (int)entries.size()) {
            const auto& e = entries[sel_];
            text = e.fullpath;
            if (e.is_dir || e.is_parent) {
                if (text.back() != L'\\' && text.back() != L'/')
                    text += L'\\'; // you can adapt for '/' if you prefer
            }
        }
    }
    else if (mode_ == Mode::Command) {
        auto matches = commands_.filter(last_input_);
        if (sel_ >= 0 && sel_ < (int)matches.size())
            text = matches[sel_]->keyword;
    }
    else if (mode_ == Mode::Bookmarks) {
        auto bm = bookmarks_.all();
        if (sel_ >= 0 && sel_ < (int)bm.size())
            text = bm[sel_];
    }
    else if (mode_ == Mode::History) {
        const auto& h = history_.all();
        if (sel_ >= 0 && sel_ < (int)h.size())
            text = h[sel_];
    }

    if (!text.empty()) {
        SetWindowTextW(edit_, text.c_str());
        SendMessageW(edit_, EM_SETSEL, text.size(), text.size());
        // Also: update browser list if in FileBrowser and folder selected
        if (mode_ == Mode::FileBrowser) {
            // If it's a dir or parent, update the browser's cwd/pattern and list
            const auto& entries = browser_.results();
            if (sel_ >= 0 && sel_ < (int)entries.size()) {
                const auto& e = entries[sel_];
                if (e.is_dir || e.is_parent) {
                    browser_.set_cwd(e.fullpath + L"\\");
                    browser_.update(L"", show_hidden_);
                    update_list();
                }
            }
        }
    }
}

// Popup file actions menu for browser/bookmarks/history
void MainWindow::file_actions_menu(const std::wstring& path) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 1, L"Open");
    AppendMenuW(hMenu, MF_STRING, 2, L"Open folder");
    AppendMenuW(hMenu, MF_STRING, 3, L"Copy path");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 4, L"Move to Trash");
    AppendMenuW(hMenu, MF_STRING, 5, L"Delete (permanent)");
    AppendMenuW(hMenu, MF_STRING, 6, L"Rename...");
    bool is_bookmarked = std::find(bookmarks_.all().begin(), bookmarks_.all().end(), path) != bookmarks_.all().end();
    AppendMenuW(hMenu, MF_STRING, 7, is_bookmarked ? L"Remove Bookmark" : L"Add Bookmark");
    POINT pt; GetCursorPos(&pt);
    int res = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd_, NULL);
    switch (res) {
    case 1: fileactions::launch(path); break;
    case 2: fileactions::open_location(path); break;
    case 3: fileactions::copy_path_to_clipboard(path); break;
    case 4: fileactions::move_to_trash(path); break;
    case 5: fileactions::delete_file(path); break;
    case 6: {
        wchar_t newname[MAX_PATH] = { 0 };
        if (DialogBoxParamW(hInstance_, MAKEINTRESOURCEW(101), hwnd_, [](HWND dlg, UINT msg, WPARAM wp, LPARAM lp) -> INT_PTR {
            if (msg == WM_COMMAND && LOWORD(wp) == IDOK) {
                GetDlgItemTextW(dlg, 100, (LPWSTR)GetWindowLongPtrW(dlg, GWLP_USERDATA), MAX_PATH);
                EndDialog(dlg, IDOK); return TRUE;
            }
            if (msg == WM_INITDIALOG) { SetWindowLongPtrW(dlg, GWLP_USERDATA, lp); return TRUE; }
            if (msg == WM_COMMAND && LOWORD(wp) == IDCANCEL) { EndDialog(dlg, IDCANCEL); return TRUE; }
            return FALSE;
            }, (LPARAM)newname) == IDOK && newname[0]) {
            std::wstring newpath = path.substr(0, path.find_last_of(L"\\/") + 1) + newname;
            fileactions::rename_file(path, newpath);
        }
        break;
    }
    case 7: {
        if (is_bookmarked) bookmarks_.remove(path);
        else bookmarks_.add(path);
        bookmarks_.save();
        update_list();
        break;
    }
    }
    DestroyMenu(hMenu);
}

LRESULT CALLBACK MainWindow::EditProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    if (!self) return DefSubclassProc(hEdit, msg, wParam, lParam);
    switch (msg) {
    case WM_KEYDOWN:
        if (wParam == VK_RETURN) {
            if (self->mode_ == Mode::Command) self->activate_command(self->sel_);
            else if (self->mode_ == Mode::FileBrowser) self->activate_filebrowser(self->sel_);
            else if (self->mode_ == Mode::History) self->activate_history(self->sel_);
            else if (self->mode_ == Mode::Bookmarks) self->activate_bookmarks(self->sel_);
            SetWindowTextW(hEdit, L"");
            return 0;
        }
        else if (wParam == VK_TAB) {
            // Autofill the edit box with the currently selected item (if any)
            self->autofill_input_by_selection();
            return 0;
        }
        else if (wParam == VK_DOWN || (wParam == L'N' && (GetKeyState(VK_CONTROL) & 0x8000))) {
            int n = (int)SendMessageW(self->listbox_, LB_GETCOUNT, 0, 0);
            if (n == 0) return 0;
            self->sel_ = (self->sel_ + 1) % n;
            SendMessageW(self->listbox_, LB_SETCURSEL, self->sel_, 0);
            return 0;
        }
        else if (wParam == VK_UP || (wParam == L'P' && (GetKeyState(VK_CONTROL) & 0x8000))) {
            int n = (int)SendMessageW(self->listbox_, LB_GETCOUNT, 0, 0);
            if (n == 0) return 0;
            self->sel_ = (self->sel_ > 0) ? self->sel_ - 1 : n - 1;
            SendMessageW(self->listbox_, LB_SETCURSEL, self->sel_, 0);
            return 0;
        }
        else if (wParam == VK_BACK ) { 
            if (self->mode_ == Mode::FileBrowser) {
                // cut one path section on each key stroke
                wchar_t buffer[512];
                GetWindowTextW(self->edit_, buffer, 511);
                std::wstring text = buffer;
                size_t pos = text.length() ? text.length() - 1 : 0;
                // Remove trailing slash/backslash if present
                if (pos && (text[pos] == L'\\' || text[pos] == L'/')) --pos;
                size_t last_sep = text.rfind(L'\\', pos);
                size_t last_slash = text.rfind(L'/', pos);
                size_t cut = (last_sep == std::wstring::npos) ? last_slash : (last_slash == std::wstring::npos ? last_sep : (std::max)(last_sep, last_slash));
                if (cut != std::wstring::npos) {
                    text.erase(cut + 2);
                    SetWindowTextW(self->edit_, text.c_str());
                }
                else {
                    // If no slash, clear box
                    SetWindowTextW(self->edit_, L"");
                }
                // Move the caret to the end
                int len = GetWindowTextLengthW(self->edit_);
                SendMessageW(self->edit_, EM_SETSEL, len, len);
                self->parse_input(text);
            }
            return 0;
        }
        else if ((wParam == L'H' || wParam == L'h') && (GetKeyState(VK_CONTROL) & 0x8000)) {
            self->show_hidden_ = !self->show_hidden_;
            self->parse_input(self->last_input_);
            return 0;
        }
        else if ((wParam == L'B' || wParam == L'b') && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+B: toggle bookmark on selection (only browser/history/bookmarks)
            std::wstring path;
            if (self->mode_ == Mode::FileBrowser) {
                auto e = self->browser_.selected();
                if (e) path = e->fullpath;
            }
            else if (self->mode_ == Mode::History && self->sel_ < (int)self->history_.all().size()) {
                path = self->history_.all()[self->sel_];
            }
            else if (self->mode_ == Mode::Bookmarks && self->sel_ < (int)self->bookmarks_.all().size()) {
                path = self->bookmarks_.all()[self->sel_];
            }
            if (!path.empty()) {
                auto& bm = self->bookmarks_;
                if (std::find(bm.all().begin(), bm.all().end(), path) == bm.all().end()) bm.add(path);
                else bm.remove(path);
                bm.save();
                self->update_list();
            }
            return 0;
        }
        else if (wParam == VK_F2 || wParam == VK_APPS) {
            // F2 or "menu" key: file actions menu
            std::wstring path;
            if (self->mode_ == Mode::FileBrowser) {
                auto e = self->browser_.selected();
                if (e) path = e->fullpath;
            }
            else if (self->mode_ == Mode::History && self->sel_ < (int)self->history_.all().size()) {
                path = self->history_.all()[self->sel_];
            }
            else if (self->mode_ == Mode::Bookmarks && self->sel_ < (int)self->bookmarks_.all().size()) {
                path = self->bookmarks_.all()[self->sel_];
            }
            if (!path.empty()) self->file_actions_menu(path);
            return 0;
        }
        else if (wParam == VK_ESCAPE) {
            self->show(false);
            SetWindowTextW(hEdit, L"");
            return 0;
        }
        break;
    case WM_SYSKEYDOWN:
        if (wParam == VK_BACK && (GetKeyState(VK_MENU) & 0x8000)) { // Alt+Backspace
            wchar_t buf[512];
            GetWindowTextW(hEdit, buf, 511);
            std::wstring newText = string_util::delete_word_backward(buf);

			SetWindowTextW(hEdit, newText.c_str());
			int len = GetWindowTextLengthW(hEdit);
			SendMessageW(hEdit, EM_SETSEL, len, len);
            
            return 0;
        }
    case WM_CHAR: {
        LRESULT res = DefSubclassProc(hEdit, msg, wParam, lParam);
        // Allow only printable (`wParam >= 32`) to reach your parse logic
        // You may also allow Enter (13), Tab (9), etc., as needed
        if (wParam >= 32)
        {
            wchar_t buffer[512];
            GetWindowTextW(hEdit, buffer, 511);
            self->parse_input(buffer);
        }
        return res;
    }
    }
    return DefSubclassProc(hEdit, msg, wParam, lParam);
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!self) return DefWindowProcW(hwnd, msg, wParam, lParam);
    switch (msg) {
    case WM_HOTKEY:
        if (wParam == 1)
            self->show(!IsWindowVisible(hwnd));
        return 0;
    case WM_COMMAND:
        if (HIWORD(wParam) == LBN_DBLCLK && LOWORD(wParam) == 2) {
            // double click in listbox is same as Enter
            if (self->mode_ == Mode::Command) self->activate_command(self->sel_);
            else if (self->mode_ == Mode::FileBrowser) self->activate_filebrowser(self->sel_);
            else if (self->mode_ == Mode::History) self->activate_history(self->sel_);
            else if (self->mode_ == Mode::Bookmarks) self->activate_bookmarks(self->sel_);
            SetWindowTextW(self->edit_, L"");
            return 0;
        }
        if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == 2)
            self->sel_ = (int)SendMessageW(self->listbox_, LB_GETCURSEL, 0, 0);
        break;
    case WM_ACTIVATE:
        if (wParam == WA_INACTIVE)
            self->show(false);
        break;
    case WM_DESTROY:
        self->save_all();
        PostQuitMessage(0); break;
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, RGB(245, 245, 245));   // Soft gray
        SetTextColor(hdc, RGB(25, 25, 25));   // Near black
        static HBRUSH hbr = CreateSolidBrush(RGB(245, 245, 245));
        return (INT_PTR)hbr;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, RGB(255, 255, 255));
        SetTextColor(hdc, RGB(25, 25, 25));
        static HBRUSH hbr = CreateSolidBrush(RGB(255, 255, 255));
        return (INT_PTR)hbr;
    }
    case WM_LBUTTONDOWN:
        // Make client area work like a title bar for moving window
        ReleaseCapture();
        SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}