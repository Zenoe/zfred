#include "mainwindow.h"
#include "fileactions.h"
#include <commctrl.h>
#include <shlwapi.h>
#include <cwctype>
#include <vector>
#include <sstream>

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
    wc.lpszClassName = L"AlfredIndustrial_MainWnd";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassExW(&wc);

    int width = 660, height = 340;
    int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2, y = 100;
    hwnd_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        wc.lpszClassName, L"Alfred-industrial",
        WS_POPUP | WS_CLIPCHILDREN, x, y, width, height,
        nullptr, nullptr, hInstance_, nullptr);
    if (!hwnd_) return false;

    edit_ = CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        10, 10, width - 20, 32, hwnd_, (HMENU)1, hInstance_, nullptr);
    HFONT hFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    SendMessageW(edit_, WM_SETFONT, (WPARAM)hFont, TRUE);

    listbox_ = CreateWindowExW(0, L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL,
        10, 50, width - 20, height - 60, hwnd_, (HMENU)2, hInstance_, nullptr);
    SendMessageW(listbox_, WM_SETFONT, (WPARAM)hFont, TRUE);

    SetWindowSubclass(edit_, &MainWindow::EditProc, 0, 0);

    return true;
}

void MainWindow::show(bool visible) {
    ShowWindow(hwnd_, visible ? SW_SHOW : SW_HIDE);
    if (visible) {
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
        for (const auto& e : browser_.results()) {
            std::wstring disp;
            if (e.is_parent) disp = L"⤴ ..";
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
}
void MainWindow::activate_filebrowser(int idx) {
    const auto& results = browser_.results();
    if (idx >= 0 && idx < (int)results.size()) {
        const auto& e = results[idx];
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
        else if (wParam == VK_DOWN) {
            int n = (int)SendMessageW(self->listbox_, LB_GETCOUNT, 0, 0);
            if (n == 0) return 0;
            self->sel_ = (self->sel_ + 1) % n;
            SendMessageW(self->listbox_, LB_SETCURSEL, self->sel_, 0);
            return 0;
        }
        else if (wParam == VK_UP) {
            int n = (int)SendMessageW(self->listbox_, LB_GETCOUNT, 0, 0);
            if (n == 0) return 0;
            self->sel_ = (self->sel_ > 0) ? self->sel_ - 1 : n - 1;
            SendMessageW(self->listbox_, LB_SETCURSEL, self->sel_, 0);
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
    case WM_CHAR: {
        LRESULT res = DefSubclassProc(hEdit, msg, wParam, lParam);
        wchar_t buffer[512]; GetWindowTextW(hEdit, buffer, 511);
        self->parse_input(buffer);
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
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}