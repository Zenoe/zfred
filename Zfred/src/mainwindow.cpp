#include "mainwindow.h"
#include "fileactions.h"
#include <commctrl.h>
#include <shlwapi.h>
#include <cwctype>
#include <vector>
#include <sstream>
#include <commctrl.h>
#include <windowsx.h>

#include <algorithm>
#include <dwmapi.h> // For DwmExtendFrameIntoClientArea (link with dwmapi.lib)

#include "FsUtils.h"

//#ifdef _DEBUG
#include "debugtool.h"
//#endif

#include "constant.h"
#include "utils/stringutil.h"
#include "FsUtils.h"
#include "guihelper.h"

#define FLAT_BORDER_SUBCLASS_ID 1001

const COLORREF EDIT_BK_COLOR = HEXTOCOLORREF(0xEDFBF3);
const COLORREF LISTVIEW_BK_COLOR = RGB(247, 248, 250);
HBRUSH gEditBrush = nullptr;

MainWindow* MainWindow::self = nullptr;
MainWindow::MainWindow(HINSTANCE hInstance)
    : hInstance_(hInstance), hwnd_(nullptr), edit_(nullptr), listbox_(nullptr), combo_mode_(nullptr),
    mode_(Mode::History), sel_(-1), show_hidden_(false), last_input_(L"")
{
    self = this;
    bookmarks_.load();
    //history_.load();
    history_.load_async([this]() {
        PostMessageW(hwnd_, WM_HISTORY_LOADED, 0, 0);
        });
    //history_.load_async([this](const std::vector<std::wstring>& batch) {  // 不要对const 使用 move
    //history_.load_async([this](std::vector<std::wstring>& batch) {
    //    auto* pBatch = new std::vector<std::wstring>(std::move(batch));
    //    PostMessageW(hwnd_, WM_HISTORY_LOADED, 0, reinterpret_cast<LPARAM>(pBatch));
    //    });
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
    LONG exStyle =  WS_EX_TOOLWINDOW | WS_EX_LAYERED;
    // LONG exStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED;
    // WS_EX_APPWINDOW Forces window to show in taskbar/Alt-Tab (useful with popup windows) 
    hwnd_ = CreateWindowExW(exStyle,
        wc.lpszClassName, L"zfred",
        WS_POPUP | WS_CLIPCHILDREN,
        x, y, WND_W, WND_H,
        nullptr, nullptr, hInstance_, nullptr);
    if (!hwnd_) return false;

    BOOL value = TRUE;
    DwmSetWindowAttribute(hwnd_, DWMWA_NCRENDERING_ENABLED, &value, sizeof(value));
    DwmSetWindowAttribute(hwnd_, DWMWA_NCRENDERING_POLICY, &value, sizeof(value));

    // Modern border color (Windows 10+)
    COLORREF borderColor = RGB(180, 180, 180); // Light gray border
    DwmSetWindowAttribute(hwnd_, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));

    //if (IsWindows10OrGreater()) {
    //    DWORD cornerPref = 2; // DWMWCP_ROUND
    //    DwmSetWindowAttribute(hwnd_, 33, &cornerPref, sizeof(cornerPref)); // DWMWA_WINDOW_CORNER_PREFERENCE = 33
    //}
    
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
        PADDING, PADDING, EDIT_W, EDIT_H, hwnd_, (HMENU)EDITOR_ID, hInstance_, nullptr);

    // Use nice size and font (Segoe UI, 18px height)
    HFONT hFont = CreateFontW(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");


    SendMessageW(edit_, WM_SETFONT, (WPARAM)hFont, TRUE);

    //listbox_ = CreateWindowExW(
    //    WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
    //    WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | LBS_DISABLENOSCROLL,
    //    PADDING, PADDING+EDIT_H + CONTROL_MARGIN, LIST_W, 0,
    //    hwnd_, (HMENU)2, hInstance_, nullptr);
    // SendMessageW(listbox_, WM_SETFONT, (WPARAM)hFont, TRUE);

    combo_mode_ = CreateWindowExW(0, L"COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED,
         PADDING + EDIT_W +CONTROL_MARGIN, PADDING, COMBO_W, 0, hwnd_, (HMENU)COMBO_ID, hInstance_, nullptr);  // height param does not take effect when | CBS_OWNERDRAWFIXED, set height by CB_SETITEMHEIGHT
    SendMessage(combo_mode_, CB_SETITEMHEIGHT, -1, 18); // to level with edit control(whose height is 24)  // Static portion
    SendMessage(combo_mode_, CB_SETITEMHEIGHT, 0, 24); // to level with edit control(whose height is 24)  // Dropdown portion
    //SetWindowSubclass(combo_mode_, FlatComboSubclassProc, FLAT_BORDER_SUBCLASS_ID, 0);

    for (int i = 0; i < static_cast<int>(Mode::Count); ++i) {
        SendMessageW(combo_mode_, CB_ADDSTRING, 0, (LPARAM)ModeToString(static_cast<Mode>(i)));
    }
    SendMessageW(combo_mode_, CB_SETCURSEL, 0, 0);

    SetWindowSubclass(edit_, &MainWindow::EditProc, 0, 0);
    return true;
}

void MainWindow::show(bool visible) {
	ShowWindow(hwnd_, visible ? SW_SHOW : SW_HIDE);
	if (visible) {
		SetForegroundWindow(hwnd_);
		SetActiveWindow(hwnd_);
		if (last_input_.empty()) {
			SetWindowTextW(edit_, L"");
			update_listview();
		}
		SetFocus(edit_);
	}
}

void MainWindow::run() {
#ifdef _DEBUG
    RegisterHotKey(hwnd_, 1, MOD_CONTROL | MOD_ALT, VK_SPACE);
#else
    RegisterHotKey(hwnd_, 1, MOD_ALT, VK_SPACE);
#endif // DEBUG

    RegisterHotKey(hwnd_, 2,  MOD_CONTROL| MOD_ALT, 'B');
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    UnregisterHotKey(hwnd_, 1);
    UnregisterHotKey(hwnd_, 2);
}

void MainWindow::save_all() {
    history_.save();
    bookmarks_.save();
}

LRESULT MainWindow::undo_delete_word() {
	const auto undoItem = simpleundo_.pop();
	if (undoItem) {
        const std::wstring& text = ( * undoItem).first;
		const size_t caret = ( * undoItem).second;
		wchar_t buf[512];
		GetWindowTextW(self->edit_, buf, 511);
        std::wstring old_content(buf);
        // insert  text into edit_ in the pos caret 插入 `text` 到 `caret` 位置
        if (caret <= old_content.size()) {
            old_content.insert(caret, text);
        }
        else {
            // 如果caret越界，插到最后
            old_content += text;
        }
        SetWindowTextW(self->edit_, old_content.c_str());
        // 恢复插入点到插入后的位置
        SendMessageW(self->edit_, EM_SETSEL, caret + text.size(), caret + text.size());
        SendMessageW(self->edit_, EM_SCROLLCARET, 0, 0);
	}
	return 0;
}

void MainWindow::update_listview() {
    ListView_SetItemState(hListview_, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    if (mode_ == Mode::History) {
		history_.request_filter(last_input_, [this](){
		//history_.filterModifyItemThread(last_input_, [this](){
			if (mode_ != Mode::History) {
                // don't run when it's not history mode
				return;
			}
			ListView_SetItemCountEx(hListview_, history_.size(), LVSICF_NOINVALIDATEALL);
            updateUICachePage(0);
			// Force update visible rows
			InvalidateRect(hListview_, NULL, TRUE);
			UpdateWindow(hListview_);
			});
	}
	else if (mode_ == Mode::FileBrowser) {
		ListView_SetItemCountEx(hListview_, browser_.results().size(), LVSICF_NOINVALIDATEALL);
		// Force update visible rows
		InvalidateRect(hListview_, NULL, TRUE);
		UpdateWindow(hListview_);
	}else if(mode_ == Mode::Bookmarks){
		ListView_SetItemCountEx(hListview_, bookmarks_.all().size(), LVSICF_NOINVALIDATEALL);
		// Force update visible rows
		InvalidateRect(hListview_, NULL, TRUE);
		UpdateWindow(hListview_);
    }
    else if(mode_ == Mode::Clipboard) {
		clipboard_.filter(last_input_);
        ListView_SetItemCountEx(hListview_, clipboard_.getCount(), LVSICF_NOINVALIDATEALL);
        InvalidateRect(hListview_, NULL, TRUE);
        UpdateWindow(hListview_);
    }
	sel_ = -1;
	//if (sel_ >= 0){
		//ListView_SetItemState(hListview_, sel_, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        //LONG_PTR style = GetWindowLongPtr(hListview_, GWL_STYLE);
        //SetWindowLongPtr(hListview_, GWL_STYLE, style | LVS_SHOWSELALWAYS);
    //}
}
//void MainWindow::update_list() {
//    SendMessageW(listbox_, LB_RESETCONTENT, 0, 0);
//    sel_ = 0;
//    if (mode_ == Mode::Command) {
//        auto matches = commands_.filter(last_input_);
//        for (const auto* cmd : matches) {
//            std::wstring line = L"⚡ " + cmd->keyword + L"    — " + cmd->description;
//#ifdef _DEBUG
//            OutputDebugPrint(line);
//#endif // DEBUG
//
//            SendMessageW(listbox_, LB_ADDSTRING, 0, (LPARAM)line.c_str());
//        }
//    }
//    else if (mode_ == Mode::FileBrowser) {
//        for (const auto& e : browser_.results()) {
//            std::wstring disp;
//            if (e.is_parent) {
//                continue;
//                //disp = L"⤴ ..";
//            }
//            else if (e.is_dir && e.is_hidden) disp = L"[HID][DIR] " + e.label;
//            else if (e.is_dir) disp = L"[DIR] " + e.label;
//            else if (e.is_hidden) disp = L"[HID] " + e.label;
//            else disp = L"   " + e.label;
//            SendMessageW(listbox_, LB_ADDSTRING, 0, (LPARAM)disp.c_str());
//        }
//    }
//    else if (mode_ == Mode::History) {
//        auto matches = history_.filter(last_input_);
//        for (const auto* item : matches) {
//            std::wstring label = (PathIsDirectoryW(item->c_str()) ? L"🗂 " : L"🗎 ") + *item;
//            SendMessageW(listbox_, LB_ADDSTRING, 0, (LPARAM)label.c_str());
//        }
//    }
//    else if (mode_ == Mode::Bookmarks) {
//        for (const auto& b : bookmarks_.all()) {
//            std::wstring label = (PathIsDirectoryW(b.c_str()) ? L"🧡 " : L"♥ ") + b;
//            SendMessageW(listbox_, LB_ADDSTRING, 0, (LPARAM)label.c_str());
//        }
//    }
//    int n = (int)SendMessageW(listbox_, LB_GETCOUNT, 0, 0);
//    sel_ = (n > 0) ? 0 : -1;
//
//	if (n > 0) {
//        // can be history or file/folder list
//		int item_height = (int)SendMessageW(listbox_, LB_GETITEMHEIGHT, 0, 0);
//
//		int visible_items = (n < MAX_ITEMS) ? n : MAX_ITEMS;
//		int list_height = (item_height * visible_items) + LIST_BOTTOM_PADDING;
//		int total_height = WND_H + CONTROL_MARGIN + list_height;
//
//		SetWindowPos(hwnd_, NULL, 0, 0, WND_W, total_height, SWP_NOMOVE | SWP_NOZORDER);
//		SetWindowPos(listbox_, NULL, 0, 0, LIST_W, list_height, SWP_NOMOVE | SWP_NOZORDER);
//
//		ShowWindow(listbox_, SW_SHOW);
//	}
//	else {
//		ShowWindow(listbox_, SW_HIDE);
//		SetWindowPos(hwnd_, NULL, 0, 0, WND_W, WND_H, SWP_NOMOVE | SWP_NOZORDER);
//	}
//    if (sel_ >= 0)
//        SendMessageW(listbox_, LB_SETCURSEL, sel_, 0);
//}

void MainWindow::parse_input(const std::wstring& text) {
    // todo move to update_listview() ?
    last_input_ = text;
    if(text.size() == 0){
        mode_ = Mode::History;
        SendMessageW(combo_mode_, CB_SETCURSEL, static_cast<int>(mode_), 0);
    }else{
        if (text.size() >= 2 && text[1] == L':' && (text[2] == L'/' || text[2] == L'\\')) {
            // Looks like C:\ or C:/ path
            mode_ = Mode::FileBrowser;
            size_t lastsep = text.find_last_of(L"\\/");
            std::wstring dir = (lastsep != std::wstring::npos) ? text.substr(0, lastsep + 1) : text;
            std::wstring pat = (lastsep != std::wstring::npos) ? text.substr(lastsep + 1) : L"";
            OutputDebugPrint("curdir: ", dir, pat, " ", text);
            browser_.set_cwd(dir);
            browser_.update(pat, show_hidden_);
            SendMessageW(combo_mode_, CB_SETCURSEL, static_cast<int>(mode_), 0);
        }
        // else {
        //     mode_ = Mode::History;
        // }
    }
	update_listview();
}

// Activation methods: what to do when Enter/Double-click
// void MainWindow::activate_command(int idx) {
//     auto matches = commands_.filter(last_input_);
//     if (!last_input_.empty() && idx >= 0 && idx < (int)matches.size()) {
//         matches[idx]->action();
//         show(false);
//     }
//     else if (last_input_.empty() && idx >= 0 && idx < (int)history_.size()) {
//         std::wstring sel = history_.all()[idx];
//         if (PathIsDirectoryW(sel.c_str())) {
//             mode_ = Mode::FileBrowser;
//             browser_.set_cwd(sel);
//             browser_.update(L"", show_hidden_);
//             SetWindowTextW(edit_, sel.c_str());
//             update_list();
//         }
//         else {
//             fileactions::launch(sel);
//             show(false);
//         }
//         history_.add(sel);
//     }
//     else {
//         // just run
//         if (!last_input_.empty()) {
//             auto result = (INT_PTR)ShellExecuteW(nullptr, L"open", last_input_.c_str(), nullptr, nullptr, SW_SHOW);
//             if (result <= 32) {
//                 MessageBoxW(hwnd_, (L"'" + last_input_ + L"' was not found in system PATH or could not be executed.").c_str(),
//                     L"Command failed", MB_OK | MB_ICONWARNING);
//             }
//         }
//     }
// }
void MainWindow::activate_filebrowser(int idx) {
    const auto& results = browser_.results();
    if (idx >= 0 && idx < (int)results.size()) {
        //const auto& e = results[idx];
        // copy, cause results may be clear by browser_.update()
        const auto e = results[idx];
        if (e.is_parent || e.is_dir) {
            GuiHelper::openFolder(hwnd_, e.fullpath);
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
		if (idx >= 0 && idx < (int)history_.size()) {
			std::wstring curItem = (history_)[idx];
			if (PathIsDirectoryW(curItem.c_str())) {
                GuiHelper::openFolder(hwnd_, curItem);
			}
			else if(FsUtils::is_file(curItem.c_str())){
				fileactions::launch(curItem);
				show(false);
            }
            else {
				auto result = (INT_PTR)ShellExecuteW(nullptr, L"open", curItem.c_str(), nullptr, nullptr, SW_SHOW);
                if (result > 32) {
					show(false);
					history_.add(curItem);
                }// else unknown command, don't add to history
            }
            return;
        }
        // no one matches to last_input_ from history list
        if (!last_input_.empty()) {
            auto result = (INT_PTR)ShellExecuteW(nullptr, L"open", last_input_.c_str(), nullptr, nullptr, SW_SHOW);
            if (result > 32) {
                show(false);
                history_.add(last_input_);
            }
            //else {
            //    MessageBoxW(hwnd_, (L"'" + last_input_ + L"' was not found in system PATH or could not be executed.").c_str(),
            //        L"Command failed", MB_OK | MB_ICONWARNING);
            //}
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
            update_listview();
        }
        else {
            fileactions::launch(sel);
            show(false);
        }
        history_.add(sel);
    }
}

void MainWindow::activate_clipboard(int idx){
    clipboard_.write(idx);
}
void MainWindow:: autofill_input_by_selection() {
    if (sel_ < 0) return;  // no item in the listview
    std::wstring text;
    auto autoFillDir = [this](std::wstring& text) {
		if (text.back() != L'\\' && text.back() != L'/') text += L'/';
		SetWindowTextW(edit_, text.c_str());
		SendMessageW(edit_, EM_SETSEL, text.size(), text.size());
		mode_ = Mode::FileBrowser;
		SendMessageW(combo_mode_, CB_SETCURSEL, static_cast<int>(mode_), 0);
		browser_.set_cwd(text);
		browser_.update(L"", show_hidden_);
		update_listview();
	};
	if (mode_ == Mode::FileBrowser) {
		const auto& entries = browser_.results();
        if (sel_ >= 0 && sel_ < (int)entries.size()) {
            const auto& e = entries[sel_];
            text = e.fullpath;
            if (e.is_dir || e.is_parent) {
                if (text.back() != L'\\' && text.back() != L'/')
                    text += L'/'; 
            }
        }
    }
    else if (mode_ == Mode::Bookmarks) {
        auto bm = bookmarks_.all();
        if (sel_ >= 0 && sel_ < (int)bm.size()) {
            text = bm[sel_];
			if (PathIsDirectoryW(text.c_str())) {
				return autoFillDir(text);
			}
        }
    }
    else if (mode_ == Mode::History) {
        if (sel_ >= 0) {
			text = history_[sel_];
			if (PathIsDirectoryW(text.c_str())) {
				return autoFillDir(text);
			}
		}
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
                    browser_.set_cwd(e.fullpath + L"/");
                    browser_.update(L"", show_hidden_);
                    update_listview();
                }
            }
        }
    }
}
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
    //POINT pt; GetCursorPos(&pt);
    RECT rc; // bounding rectangle for the item
    if (!ListView_GetItemRect(hListview_, sel_, &rc, LVIR_BOUNDS)) return;

    POINT pt = { rc.left + 4, rc.bottom - 4 }; 
    ClientToScreen(hListview_, &pt);
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
        update_listview();
        break;
    }
    }
    DestroyMenu(hMenu);
}

LRESULT MainWindow::processAltBackspace() {
	wchar_t buf[512];
	GetWindowTextW(self->edit_, buf, 511);
	DWORD sel = (DWORD)SendMessage(self->edit_, EM_GETSEL, NULL, NULL);
	//OutputDebugPrint(LOWORD(sel), HIWORD(sel));
	DWORD endpos = HIWORD(sel);

	const auto [newText, deletedText] = string_util::delete_word_backward(buf, (size_t)(endpos));
	//OutputDebugPrint("deleted text: ", deleted);
	self->simpleundo_.push(deletedText, endpos - deletedText.size());
	SetWindowTextW(self->edit_, newText.c_str());
	size_t newCursorPos = endpos - deletedText.size();
	SendMessageW(self->edit_, EM_SETSEL, newCursorPos, newCursorPos);
    last_input_ = newText;
    update_listview();
	return 0;
}
void CustomBackspaceEdit(HWND hwnd)
{
    // 1. Get selection
    DWORD selStart, selEnd;
    SendMessageW(hwnd, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);

    if (selStart == selEnd)
    {
        // No selection: delete the character before the caret
        if (selStart == 0)
            return; // At start, nothing to delete

        // Get full text to handle surrogate pairs (optional, for Unicode correctness)
        int len = GetWindowTextLengthW(hwnd);
        std::wstring buf(len, 0);
        GetWindowTextW(hwnd, &buf[0], len + 1);

        // Find the previous codepoint boundary
        size_t prev = selStart - 1;
        // Optional: handle UTF-16 surrogate pairs (if needed)
        if (prev > 0 && IS_HIGH_SURROGATE(buf[prev - 1]) && IS_LOW_SURROGATE(buf[prev]))
            prev--;

        // Set the selection to the char before the caret
        SendMessageW(hwnd, EM_SETSEL, prev, selStart);
    }
    // else: text is selected, EM_REPLACESEL will just delete it

    // 2. Delete the selected text (or the single character)
    SendMessageW(hwnd, EM_REPLACESEL, TRUE, (LPARAM)L"");
}

LRESULT MainWindow::processBackspace()
{
	wchar_t buffer[512];
	GetWindowTextW(self->edit_, buffer, 511);
	std::wstring text = buffer;
    // OutputDebugPrint("text:", text);
	if (self->mode_ == Mode::FileBrowser) {
		// cut one path section on each key stroke
		size_t pos = text.length() ? text.length() - 1 : 0;
		// Remove trailing slash/backslash if present
		if (pos && (text[pos] == L'\\' || text[pos] == L'/')) --pos;
		size_t last_sep = text.rfind(L'\\', pos);
		size_t last_slash = text.rfind(L'/', pos);
		size_t cut = (last_sep == std::wstring::npos) ? last_slash : (last_slash == std::wstring::npos ? last_sep : (std::max)(last_sep, last_slash));
		if (cut != std::wstring::npos) {
			text.erase(cut + 1);
			 SetWindowTextW(self->edit_, text.c_str());
			//SetWindowTextW(self->edit_, L"c:/");
		}
		else {
			// If no slash, clear box
			SetWindowTextW(self->edit_, L"");
            text.clear();
		}
		// Move the caret to the end
		int len = GetWindowTextLengthW(self->edit_);
        SendMessageW(self->edit_, EM_SETSEL, len, len);
		self->parse_input(text);

	}
	else {
        CustomBackspaceEdit(self->edit_);
		//last_input_ = text.substr(0, text.size() - 1);
		//SetWindowTextW(self->edit_, last_input_.c_str());
		//int len = GetWindowTextLengthW(self->edit_);
        //SendMessageW(self->edit_, EM_SETSEL, len, len);
        //last_input = 

        int len = GetWindowTextLengthW(self->edit_);
        std::wstring buf(len, 0);
        GetWindowTextW(self->edit_, &buf[0], len + 1);
        last_input_ = std::move(buf);
		self->update_listview();
	}
    return 0;
}

void MainWindow::update_spinner() {
    if (!show_spinner_) {
        // erase spinner, you may SetWindowTextW(spinner_label_, L"");
        SetWindowTextW(edit_, L""); // blank or old value
        return;
    }
    static const wchar_t* frames[] = { L"⠋", L"⠙", L"⠹", L"⠸", L"⠼", L"⠴", L"⠦", L"⠧", L"⠇", L"⠏" };
    spinner_frame_ = (spinner_frame_ + 1) % (sizeof(frames) / sizeof(frames[0]));
    std::wstring old; int len = GetWindowTextLengthW(edit_);
    if (len) {
        old.resize(len);
        GetWindowTextW(edit_, &old[0], len + 1);
    }
    std::wstring spin = frames[spinner_frame_];
    std::wstring display = L"Loading " + spin;
    SetWindowTextW(edit_, display.c_str());
}

void MainWindow::updateUICachePage(int startRow) {
    uiCachePage = history_.get_page(startRow, PAGE_SIZE);
    cachePageStart.store(startRow);
}

LRESULT MainWindow::processAppendHistory() {
    ListView_SetItemCountEx(hListview_, history_.size(), LVSICF_NOINVALIDATEALL);
    // for listbox
    //if (mode_ == Mode::History) {
    //    update_list();
    //}
    return 0;
}

void MainWindow::processListViewContent(LPARAM lParam) {
    auto set_item_label = [](NMLVDISPINFOW* plvdi, const std::wstring& text) {
		std::wstring label = (FsUtils::is_dir(text.c_str()) ? L"🗂 " : L"🗎 ") + text;
        wcsncpy_s(plvdi->item.pszText, plvdi->item.cchTextMax, label.c_str(), _TRUNCATE);
    };

    NMLVDISPINFOW* plvdi = reinterpret_cast<NMLVDISPINFOW*>(lParam);
    int iItem = plvdi->item.iItem;

    // check if text information is requested 
    if (!(plvdi->item.mask & LVIF_TEXT)) return;

    if (mode_ == Mode::History) {
        if (!uiCachePage.empty() && iItem >= cachePageStart && iItem < cachePageStart + PAGE_SIZE) {
            //OutputDebugPrint("hit cache");
			set_item_label(plvdi, uiCachePage[iItem - cachePageStart]);
        }
        else {
			history_.allWith([&](const auto& items) {
				if (iItem < static_cast<int>(items.size())) {
                    int newStart = (iItem / PAGE_SIZE) * PAGE_SIZE;
                    updateUICachePage(newStart);
					//const std::wstring& path = items[iItem];
					//set_item_label(plvdi, path);
    
					set_item_label(plvdi, uiCachePage[iItem - cachePageStart]);
				}
				});
		}

	  //      //std::deque<std::wstring> items = history_.all(); // copy huge data, cause listview populating slow
			//NMLVDISPINFOW* plvdi = (NMLVDISPINFOW*)lParam;
			//int iItem = plvdi->item.iItem;
			//if ((plvdi->item.mask & LVIF_TEXT) && iItem < (int)history_.all().size()) {
	  //          OutputDebugPrint("history size: ", history_.all().size());
			//	const std::wstring& path = history_.all()[iItem];
	  //          std::wstring label = (PathIsDirectoryW(path.c_str()) ? L"🗂 " : L"🗎 ") + path;
			//	wcsncpy_s(plvdi->item.pszText, plvdi->item.cchTextMax, label.c_str(), _TRUNCATE);
			//}
	}
	else if (mode_ == Mode::FileBrowser) {
        const auto& results = browser_.results();
        if (iItem < static_cast<int>(results.size())) {
            const FileEntry& entry = results[iItem];
            set_item_label(plvdi, entry.label);
        }
    }
    else if (mode_ == Mode::Bookmarks) {
        const auto& bookmarks = bookmarks_.all();
        if (iItem < static_cast<int>(bookmarks.size())) {
            const std::wstring& entry = bookmarks[iItem];
            set_item_label(plvdi, entry);
        }
    }
    else if (mode_ == Mode::Clipboard) {
        //ListView_DeleteAllItems(hListview_);
        const auto& items = clipboard_.getItems();
        // auto items = clipboard_.getItems();
        if (iItem < items.size()) {
			set_item_label(plvdi, items[iItem].content);
        }
        //LVITEM lvi = { 0 };
        //for (size_t i = 0; i < items.size(); ++i) {
        //    lvi.mask = LVIF_TEXT;
        //    lvi.iItem = i;
        //    lvi.iSubItem = 0;
        //    lvi.pszText = const_cast<wchar_t*>(std::wstring(items[i].content.begin(), items[i].content.end()).c_str());
        //    ListView_InsertItem(hListview_, &lvi);

        //    lvi.iSubItem = 1;
        //    lvi.pszText = const_cast<wchar_t*>(std::wstring(items[i].timestamp.begin(), items[i].timestamp.end()).c_str());
        //    ListView_SetItemText(hListview_, i, 1, lvi.pszText);
        //}
    }
}

void MainWindow::selectListview(int sel) {
	LVITEM lvi = { 0 };
	lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	lvi.state = 0;
	SendMessageW(self->hListview_, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&lvi);

	// Select new
	lvi.state = LVIS_SELECTED | LVIS_FOCUSED;
	SendMessageW(self->hListview_, LVM_SETITEMSTATE, (WPARAM)sel, (LPARAM)&lvi);

	// Ensure visible
	SendMessageW(self->hListview_, LVM_ENSUREVISIBLE, (WPARAM)sel, (LPARAM)FALSE);
}
LRESULT MainWindow::processContextMenu() {
    if (sel_ < 0) return 0;
	std::wstring path;
	if (mode_ == Mode::FileBrowser) {
		auto e = self->browser_.selected();
		if (e) path = e->fullpath;
	}
	else if (self->mode_ == Mode::History && self->sel_ < (int)self->history_.size()) {
		path = self->history_.all()[self->sel_];
	}
	else if (self->mode_ == Mode::Bookmarks && self->sel_ < (int)self->bookmarks_.all().size()) {
		path = self->bookmarks_.all()[self->sel_];
	}
	if (!path.empty()) self->file_actions_menu(path);
	return 0;
}
void MainWindow::processReturn() {
	if (mode_ == Mode::FileBrowser) activate_filebrowser(sel_);
	else if (mode_ == Mode::History) activate_history(sel_);
	else if (mode_ == Mode::Bookmarks) activate_bookmarks(sel_);
	else if (mode_ == Mode::Clipboard) activate_clipboard(sel_);
}

LRESULT MainWindow::switchMode(WPARAM wParam) {
    if (wParam == 0x31)
        mode_ = Mode::History;
    else if (wParam == 0x32)
        mode_ = Mode::Bookmarks;
    //else if (wParam == VK_F2)
    //    mode_ = Mode::Clipboard;
    else
        return 0;
	SendMessageW(combo_mode_, CB_SETCURSEL, static_cast<int>(mode_), 0);
	update_listview();
	SetFocus(edit_);
    return 0;
}

LRESULT MainWindow::processChar(UINT msg, WPARAM wParam, LPARAM lParam){
    if (wParam == VK_BACK) { // '\b' == 8
        return self->processBackspace();
    }
    LRESULT res = DefSubclassProc(edit_, msg, wParam, lParam);
    // Allow only printable (`wParam >= 32`) to reach your parse logic
    // You may also allow Enter (13), Tab (9), etc., as needed
    if (wParam >= 32)
        {
            wchar_t buffer[512];
            GetWindowTextW(edit_, buffer, 511);
            self->parse_input(buffer);
        }
    return res;
}
LRESULT CALLBACK MainWindow::EditProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    if (!self) return DefSubclassProc(hEdit, msg, wParam, lParam);
    auto updateInput = [&]() {
        wchar_t buffer[512];
        GetWindowTextW(hEdit, buffer, 511);
        self->parse_input(buffer);
    };
    switch (msg) {
    case WM_KEYDOWN:
        if (wParam == VK_RETURN) {
            self->processReturn();
            return 0;
        }
        if ((wParam == 'A' || wParam == 'a') && (GetKeyState(VK_CONTROL) & 0x8000)) {
            SendMessage(hEdit, EM_SETSEL, 0, -1);
            return 0; // swallow the key
        }
        if (((wParam == 'V' || wParam == 'v') && (GetKeyState(VK_CONTROL) & 0x8000)) ||
            ((wParam == 'X' || wParam == 'x') && (GetKeyState(VK_CONTROL) & 0x8000))
            )
        {
            // Ctrl+V: Paste, let control handle first, then parse_input after text changes
            LRESULT res = DefSubclassProc(hEdit, msg, wParam, lParam);
            updateInput();
            return res;
        }
		if (wParam == VK_TAB) {
			if (self->sel_ == -1) {
				self->processListviewNavigation(+1);
			}
			self->autofill_input_by_selection();
			return 0;
		}
		if (wParam == VK_DOWN || (wParam == L'N' && (GetKeyState(VK_CONTROL) & 0x8000))) {
			//int n = (int)SendMessageW(self->listbox_, LB_GETCOUNT, 0, 0);
			//if (n == 0) return 0;
			//self->sel_ = (self->sel_ + 1) % n;
			//SendMessageW(self->listbox_, LB_SETCURSEL, self->sel_, 0);
			//return 0;

            self->processListviewNavigation(+1);
            return 0;
        }
        if (wParam == VK_UP || (wParam == L'P' && (GetKeyState(VK_CONTROL) & 0x8000))) {
            self->processListviewNavigation(-1);
            return 0;
        }
        //else if (wParam == VK_BACK ) { 
        //    return self->processBackspace();
        //}
        else if (wParam == VK_OEM_2 && (GetKeyState(VK_CONTROL) & 0x8000)) {  // ctrl /
            return self->undo_delete_word();
        }
        else if ((wParam == 0x31 || wParam == 0x32 || wParam == VK_F2) && (GetKeyState(VK_CONTROL) & 0x8000)) {
            return self->switchMode(wParam);
        }
        
        else if ((wParam == L'H' || wParam == L'h') && (GetKeyState(VK_CONTROL) & 0x8000)) {
            self->show_hidden_ = !self->show_hidden_;
            self->parse_input(self->last_input_);
            return 0;
        }
        else if ((wParam == L'B' || wParam == L'b') && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+B: toggle bookmark on selection (only browser/history/bookmarks)
            if (self->sel_ < 0) return 0;
            std::wstring path;
            if (self->mode_ == Mode::FileBrowser) {
                auto e = self->browser_.selected();
                if (e) path = e->fullpath;
            }
            else if (self->mode_ == Mode::History && self->sel_ < (int)self->history_.size()) {
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
                self->update_listview();
            }
            return 0;
        }
        else if (wParam == VK_F2 || wParam == VK_APPS) {
            return self->processContextMenu();
        }
        else if (wParam == VK_ESCAPE ) {
            self->show(false);
            SetWindowTextW(hEdit, L"");
            return 0;
        }
        break;
    case WM_PASTE:
    case WM_CUT: // Handle paste/cut from context menu, etc.
    {
        LRESULT res = DefSubclassProc(hEdit, msg, wParam, lParam);
        updateInput();
        return res;
    }
    break;
    case WM_SYSKEYDOWN:
        if (wParam == VK_BACK && (GetKeyState(VK_MENU) & 0x8000)) {
            return self->processAltBackspace();
        }
        break;
	case WM_CHAR: {
        return self->processChar(msg, wParam, lParam);
	}
    }
    return DefSubclassProc(hEdit, msg, wParam, lParam);
}

void MainWindow::processListviewNavigation(int direction) {
    int n = (int)SendMessageW(hListview_, LVM_GETITEMCOUNT, 0, 0);
    if (n == 0) return ;
    if (direction > 0)   // forward (down)
        sel_ = (sel_ + 1) % n;
    else                 // backward (up)
        sel_ = (sel_ > 0) ? sel_ - 1 : n - 1;

    selectListview(sel_);
}

LRESULT MainWindow::processContextMenu(WPARAM wParam, LPARAM lParam) {
        if ((HWND)wParam == hListview_) {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            POINT pt = { x, y };
            ScreenToClient(hListview_, &pt);

            LVHITTESTINFO ht = { 0 };
            ht.pt = pt;
            int iItem = ListView_HitTest(hListview_, &ht);
            if (iItem < 0) return 0;
            std::wstring hittedItem;
            if (mode_ == Mode::FileBrowser) {
                hittedItem = browser_.results()[iItem].fullpath;
            }
            else if (mode_ == Mode::Bookmarks) {
                hittedItem = bookmarks_.all()[iItem];
            }
            else if (mode_ == Mode::History) {
                hittedItem = history_.all()[iItem];
            }
			GuiHelper::ShowShellContextMenu(hwnd_, hListview_, hittedItem, x, y);
        }
        return 0;
}
LRESULT MainWindow::processWMCommand(WPARAM wParam) {
    if (LOWORD(wParam) == LISTVIEW_ID) {
        // list
        if (HIWORD(wParam) == LBN_DBLCLK) {
            // double click in listbox is same as Enter
            if (mode_ == Mode::FileBrowser) activate_filebrowser(sel_);
            else if (mode_ == Mode::History) activate_history(sel_);
            else if (mode_ == Mode::Bookmarks) activate_bookmarks(sel_);
            SetWindowTextW(edit_, L"");
        }
        else if (HIWORD(wParam) == LBN_SELCHANGE){
			sel_ = (int)SendMessageW(listbox_, LB_GETCURSEL, 0, 0);
        }
    }
    else if (LOWORD(wParam) == COMBO_ID ) {// combo
        if (HIWORD(wParam) == CBN_SELCHANGE) {
			int sel = (int)SendMessageW(combo_mode_, CB_GETCURSEL, 0, 0);
            mode_ = static_cast<Mode>(sel);
            update_listview();
			SetFocus(edit_);
		}
	}
	return 0;
}
LRESULT MainWindow::processDrawItem(LPARAM lParam) {
    LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
    if (pdis->CtlID == COMBO_ID && pdis->itemID >= 0) {
        // === Modern colors/font ===
        COLORREF clrBg = (pdis->itemState & ODS_SELECTED) ? RGB(230, 240, 255) : RGB(255, 255, 255); // blue highlight if selected
        COLORREF clrText = (pdis->itemState & ODS_SELECTED) ? RGB(0, 60, 150) : RGB(40, 40, 40);
        HBRUSH hBrush = CreateSolidBrush(clrBg);
        FillRect(pdis->hDC, &pdis->rcItem, hBrush);
        DeleteObject(hBrush);

        // Fetch item text
        wchar_t szText[128] = { 0 };
        SendMessageW(pdis->hwndItem, CB_GETLBTEXT, pdis->itemID, (LPARAM)szText);

        int iconMargin = 4, iconSize = 16;
        RECT rcIcon{ pdis->rcItem.left + iconMargin, pdis->rcItem.top + (pdis->rcItem.bottom - pdis->rcItem.top - iconSize) / 2,
                    pdis->rcItem.left + iconMargin + iconSize, pdis->rcItem.top + (pdis->rcItem.bottom - pdis->rcItem.top + iconSize) / 2 };

        // Pick icon by string
        if (wcscmp(szText, L"History") == 0) {
            // Draw a clock face
            Ellipse(pdis->hDC, rcIcon.left, rcIcon.top, rcIcon.right, rcIcon.bottom);
            MoveToEx(pdis->hDC, rcIcon.left + iconSize / 2, rcIcon.top + iconSize / 2, nullptr); // center
            LineTo(pdis->hDC, rcIcon.left + iconSize / 2, rcIcon.top + 4); // hour
            MoveToEx(pdis->hDC, rcIcon.left + iconSize / 2, rcIcon.top + iconSize / 2, nullptr);
            LineTo(pdis->hDC, rcIcon.left + iconSize - 4, rcIcon.top + iconSize / 2); // minute
        }
        else if (wcscmp(szText, L"FileBrowser") == 0) {
            // Draw a folder
            HBRUSH yBrush = CreateSolidBrush(RGB(255, 215, 80));
            RECT folder = rcIcon; folder.top += 4; folder.left += 2; folder.right -= 2; folder.bottom -= 2;
            FillRect(pdis->hDC, &folder, yBrush);
            DeleteObject(yBrush);
            MoveToEx(pdis->hDC, rcIcon.left, rcIcon.top + 5, nullptr);
            LineTo(pdis->hDC, rcIcon.right, rcIcon.top + 5); // folder top
            MoveToEx(pdis->hDC, rcIcon.left + 4, rcIcon.top, nullptr);
            LineTo(pdis->hDC, rcIcon.left + 7, rcIcon.top + 5); // folder tab
            DeleteObject(yBrush);
        }
        else if (wcscmp(szText, L"Bookmarks") == 0) {
            // Draw a bookmark (flag)
            HBRUSH rBrush = CreateSolidBrush(RGB(220, 64, 90));
            RECT flag = { rcIcon.left + 4, rcIcon.top + 2, rcIcon.left + 10, rcIcon.bottom - 4 };
            FillRect(pdis->hDC, &flag, rBrush);
            POINT tri[3] = { {rcIcon.left + 4, rcIcon.bottom - 4}, {rcIcon.left + 10, rcIcon.bottom - 4}, {rcIcon.left + 7, rcIcon.bottom - 1} };
            HBRUSH bBrush = CreateSolidBrush(RGB(90, 64, 220));
            SelectObject(pdis->hDC, bBrush);
            Polygon(pdis->hDC, tri, 3);
            DeleteObject(bBrush);
            DeleteObject(rBrush);
        }
        else if (wcscmp(szText, L"Clipboard") == 0) {
            // Draw a clipboard
            HBRUSH bBrush = CreateSolidBrush(RGB(180, 200, 255));
            RECT pad = rcIcon; pad.top += 3; pad.left += 4; pad.right -= 4; pad.bottom -= 2;
            FillRect(pdis->hDC, &pad, bBrush);
            DeleteObject(bBrush);
            Rectangle(pdis->hDC, rcIcon.left + 4, rcIcon.top + 1, rcIcon.right - 4, rcIcon.top + 5); // clip head
        }
        else {
            // default: draw colored dot
            HBRUSH hIconBrush = CreateSolidBrush(RGB(80, 150, 255));
            SelectObject(pdis->hDC, hIconBrush);
            Ellipse(pdis->hDC, rcIcon.left, rcIcon.top, rcIcon.right, rcIcon.bottom);
            DeleteObject(hIconBrush);
        }

        // Draw text (Segoe UI if available)
        HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(pdis->hDC, hFont);

        RECT rcText = pdis->rcItem;
        rcText.left += iconMargin + iconMargin + iconSize; // offset after icon
        SetTextColor(pdis->hDC, clrText);
        SetBkMode(pdis->hDC, TRANSPARENT);
        DrawTextW(pdis->hDC, szText, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        SelectObject(pdis->hDC, hOldFont);
        DeleteObject(hFont);

        // Draw flat border for drop-down
        //if (pdis->itemAction == ODA_FOCUS) {
        //    RECT rc = pdis->rcItem;
        //    rc.left += 1; rc.top += 1;
        //    rc.right -= 1; rc.bottom -= 1;
        //    FrameRect(pdis->hDC, &rc, (HBRUSH)GetStockObject(GRAY_BRUSH));
        //}
    }
    return 0;
}

LRESULT MainWindow::processHotkey(WPARAM wParam) {
        if (wParam == 1)
            show(!IsWindowVisible(hwnd_));
        else if (wParam == 2) {
            show(true);
            mode_ = Mode::Clipboard;
            SendMessageW(combo_mode_, CB_SETCURSEL, (WPARAM)mode_, 0);
            update_listview();
            SetFocus(edit_);
        }
        return 0;
}
LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (!self) return DefWindowProcW(hwnd, msg, wParam, lParam);
	switch (msg) {
        case WM_CREATE:{
            AddClipboardFormatListener(hwnd);
            gEditBrush = CreateSolidBrush(EDIT_BK_COLOR);
            CoInitialize(NULL); // Must call this to use shell COM interfaces!
            self->hListview_ = CreateWindowW(WC_LISTVIEWW, L"",
                                       WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL | LVS_NOCOLUMNHEADER | LVS_SHOWSELALWAYS,
                                       PADDING, PADDING + EDIT_H + CONTROL_MARGIN, 0, 0,
                                       hwnd, (HMENU)LISTVIEW_ID, GetModuleHandle(nullptr), nullptr);
            ListView_SetBkColor(self->hListview_, LISTVIEW_BK_COLOR);
            ListView_SetTextBkColor(self->hListview_, LISTVIEW_BK_COLOR);
            LVCOLUMNW col = { 0 };
            col.mask = LVCF_TEXT | LVCF_WIDTH;
            col.pszText = (LPWSTR)L"";
            col.cx = LIST_W;
            ListView_InsertColumn(self->hListview_, 0, &col);
            SetWindowSubclass(self->hListview_, &MainWindow::ListviewProc, 0, 0);
			break;
        }
    case WM_HOTKEY:
        self->processHotkey(wParam);
        return 0;
    case WM_COMMAND:
        return self->processWMCommand(wParam);
    case WM_CONTEXTMENU:
        return self->processContextMenu(wParam, lParam);

    //case WM_ACTIVATE:
    //    if (wParam == WA_INACTIVE)
    //        self->show(false);
    //    break;
    case WM_CTLCOLOREDIT:
    {
        if ((HWND)lParam == self->edit_) {
			//return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
            HDC hdcEdit = (HDC)wParam;
            //SetBkMode(hdcEdit, OPAQUE);
            SetBkColor(hdcEdit, EDIT_BK_COLOR);
            return (LRESULT)gEditBrush;
        }
        else {
			HDC hdc = (HDC)wParam;
			SetBkColor(hdc, RGB(255, 255, 255));
			SetTextColor(hdc, RGB(25, 25, 25));
			static HBRUSH hbr = CreateSolidBrush(RGB(255, 255, 255));
			return (INT_PTR)hbr;
        }
        // else: default color for other edits
        //return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    }
    case WM_CLIPBOARDUPDATE:
        if (OpenClipboard(hwnd)) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData) {
                wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
                if (pszText) {
                    std::wstring strText(pszText);
                    GlobalUnlock(hData);
                    self->clipboard_.add(strText);
                    self->update_listview();
                }
            }
            CloseClipboard();
        }
        break;
    case WM_DESTROY:
        RemoveClipboardFormatListener(hwnd);
        self->save_all();
        CoUninitialize();
        DeleteObject(gEditBrush);
        PostQuitMessage(0); break;
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, RGB(245, 245, 245));   // Soft gray
        SetTextColor(hdc, RGB(25, 25, 25));   // Near black
        static HBRUSH hbr = CreateSolidBrush(RGB(245, 245, 245));
        return (INT_PTR)hbr;
    }

    case WM_LBUTTONDOWN:
        // Make client area work like a title bar for moving window
        ReleaseCapture();
        SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return 0;
    case WM_HISTORY_LOADED:
    {
        //auto batch = (std::vector<std::wstring>*)lParam;
        //for (const std::wstring& h : *batch) {
        //    std::wstring label = (PathIsDirectoryW(h.c_str()) ? L"🗂 " : L"🗎 ") + h;
        //    SendMessageW(self->listbox_, LB_ADDSTRING, 0, (LPARAM)label.c_str());
        //}
        //delete batch;
        return self->processAppendHistory();
    }
    case WM_DEBOUNCED_UPDATE_LIST:
        self->queued_update_ = false;
        // When done loading, stop showing spinner:
        if (self->history_.loaded_done()) { 
            self->show_spinner_ = false;
            KillTimer(hwnd, SPINNER_TIMER_ID);
            self->update_spinner();
        }
        return 0;

    case WM_NOTIFY:
        if ((HWND)((LPNMHDR)lParam)->hwndFrom == self->hListview_) {
            if (((NMHDR*)lParam)->code == LVN_GETDISPINFOW) {
                self->processListViewContent(lParam);
            }else if(self->mode_ == Mode::Clipboard && ((NMHDR*)lParam)->code == NM_CUSTOMDRAW){

                LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)lParam;
                switch (lvcd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:
                    return CDRF_NOTIFYITEMDRAW;
                case CDDS_ITEMPREPAINT:
                    return CDRF_NOTIFYSUBITEMDRAW;
                case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
                    int itemIdx = (int)lvcd->nmcd.dwItemSpec;
                    int subItem = lvcd->iSubItem; // If you use subitems; adjust as needed

                    // Get item:
                    const auto& item = self->clipboard_.getItems()[itemIdx];
                    RECT rc;
                    ListView_GetSubItemRect(lvcd->nmcd.hdr.hwndFrom, itemIdx, subItem, LVIR_LABEL, &rc);

                    int x = rc.left;
                    int y = rc.top;
                    HDC hdc = lvcd->nmcd.hdc;

                    // Prepare font and colors:
                    int oldBkMode = SetBkMode(hdc, TRANSPARENT);
                    COLORREF defaultColor = GetTextColor(hdc);

                    // Draw text char by char, set color for matches
                    for (size_t i = 0; i < item.content.size(); ++i) {
                        std::wstring chr(1, item.content[i]);
                        SIZE sz;
                        GetTextExtentPoint32W(hdc, chr.c_str(), 1, &sz);
                        if (item.highlight_mask.size() > i-1 && item.highlight_mask[i]) {
                            SetTextColor(hdc, HEXTOCOLORREF(0x3370FF)); // Red highlight
                        } else {
                            SetTextColor(hdc, defaultColor);
                        }
                        TextOutW(hdc, x, y, chr.c_str(), 1);
                        x += sz.cx;
                    }
                    SetTextColor(hdc, defaultColor);
                    SetBkMode(hdc, oldBkMode);

                    return CDRF_SKIPDEFAULT; // tells ListView not to draw text itself
                }
                }
            }
        }
        break;
    case WM_SIZE:{
        int width = LOWORD(lParam), height = HIWORD(lParam);
        if (self->hListview_) {
            MoveWindow(self->hListview_, PADDING, PADDING + EDIT_H + CONTROL_MARGIN, width-PADDING, height-EDIT_H-PADDING, TRUE);
        }
        break;
    }
    case WM_TIMER:
        if (wParam == SPINNER_TIMER_ID) {
            self->update_spinner();
        }
		return 0;
    case WM_DRAWITEM: {
        return self ->processDrawItem(lParam);
    }
    //case WM_MEASUREITEM: { // replace by SendMessage(hCombo, CB_SETITEMHEIGHT, 0, 24);
    //    LPMEASUREITEMSTRUCT pmis = (LPMEASUREITEMSTRUCT)lParam;
    //    if (pmis->CtlID == COMBO_ID) {
    //        pmis->itemHeight = 24;
    //        return TRUE;
    //    }
    //    break;
    //}
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT r;
        GetClientRect(hwnd, &r);
        // Draw border
        HBRUSH br = CreateSolidBrush(RGB(180, 180, 180)); // light gray
        FrameRect(hdc, &r, br);
        DeleteObject(br);
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            self->show(false);
        }
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK MainWindow::ListviewProc(HWND hListview, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
    if (!self) return DefSubclassProc(hListview, msg, wParam, lParam);
    if (msg == WM_KEYDOWN) {
        if (wParam == VK_TAB) {
            SetFocus(self->edit_);
            return 0;
        }
        else if (wParam == VK_RETURN) {
            int sel = ListView_GetNextItem(hListview, -1, LVNI_SELECTED);
            if (sel >= 0) {
                self->processReturn();
            }
            return 0;
        }
		else if (wParam == VK_DOWN || (wParam == L'N' && (GetKeyState(VK_CONTROL) & 0x8000))) {
            self->processListviewNavigation(+1);
            return 0;
        }
        else if (wParam == VK_UP || (wParam == L'P' && (GetKeyState(VK_CONTROL) & 0x8000))) {
            self->processListviewNavigation(-1);
            return 0;
        }
        else if (wParam == VK_BACK ) {
            return self->processBackspace();
        }
        else if (wParam == VK_ESCAPE ) {
            self->show(false);
            return 0;
        }

    }
    return DefSubclassProc(hListview, msg, wParam, lParam);
}

LRESULT CALLBACK MainWindow::FlatComboSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}
