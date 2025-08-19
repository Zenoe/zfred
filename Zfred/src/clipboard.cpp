#include "clipboard.h"
#include <atomic>
#include <chrono>
#include <string>
#include <iostream>

ClipboardManager::ClipboardManager(HWND hwnd, Database* db) : hwnd_(hwnd), db_(db), running_(false) {}

void ClipboardManager::Start() {
    running_ = true;
    thread_ = std::thread(&ClipboardManager::Monitor, this);
}

void ClipboardManager::Stop() {
    running_ = false;
    if (thread_.joinable())
        thread_.join();
}

void ClipboardManager::Monitor() {
    AddClipboardFormatListener(hwnd_);
    MSG msg;
    while (running_) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_CLIPBOARDUPDATE) {// should be handle in UI thread. not work in this thread
                if (OpenClipboard(hwnd_)) {
                    HANDLE hData = GetClipboardData(CF_TEXT);
                    if (hData) {
                        wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
                        if (pszText) {
                            std::wstring strText(pszText);
                            GlobalUnlock(hData);
                            db_->addItem(strText);
                            PostMessage(hwnd_, WM_USER + 1, 0, 0); // Notify UI
                        }
                    }
                    CloseClipboard();
                }
            } else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    RemoveClipboardFormatListener(hwnd_);
}