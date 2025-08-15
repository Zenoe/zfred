// fileactions.cpp
#include "fileactions.h"
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

void fileactions::launch(const std::wstring& filepath) {
    ShellExecuteW(nullptr, L"open", filepath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
void fileactions::open_location(const std::wstring& filepath) {
    PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(filepath.c_str());
    if (pidl) {
        SHOpenFolderAndSelectItems(pidl, 0, NULL, 0);
        ILFree(pidl);
    }
}
bool fileactions::delete_file(const std::wstring& filepath) {
    return DeleteFileW(filepath.c_str()) != 0;
}
bool fileactions::move_to_trash(const std::wstring& filepath) {
    SHFILEOPSTRUCTW op = { 0 };
    std::wstring p = filepath + L'\0';
    op.wFunc = FO_DELETE;
    op.pFrom = p.c_str();
    op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI;
    return SHFileOperationW(&op) == 0;
}
bool fileactions::rename_file(const std::wstring& filepath, const std::wstring& newname) {
    return MoveFileW(filepath.c_str(), newname.c_str());
}
void fileactions::copy_path_to_clipboard(const std::wstring& filepath) {
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        size_t sz = (filepath.size() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sz);
        if (!hMem) {
            CloseClipboard();
            return;
        }
        void* ptr = GlobalLock(hMem);
        if (!ptr) {
            GlobalFree(hMem);
            CloseClipboard();
            return;
        }

        memcpy(ptr, filepath.c_str(), sz);
        GlobalUnlock(hMem);
        SetClipboardData(CF_UNICODETEXT, hMem);
        CloseClipboard();
    }
}