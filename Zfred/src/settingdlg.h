#pragma once
#include <windows.h>

class SettingsDialog {
public:
    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
};