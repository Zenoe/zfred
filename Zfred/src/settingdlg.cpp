#include "settingdlg.h"
#include "resource.h"

INT_PTR CALLBACK SettingsDialog::DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG:
        SetDlgItemInt(hDlg, IDC_ITEMLIMIT, 100, FALSE);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            // Save settings here if needed
            EndDialog(hDlg, TRUE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}