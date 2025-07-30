#include "mainwindow.h"
#include <commctrl.h>

//#include <gdiplus.h>
//using namespace Gdiplus;
//ULONG_PTR gdiplusToken;

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow){
    #ifndef _DEBUG
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"zfred_SINGLETON_MUTEX");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Optionally: find the existing window and bring it to foreground, then exit
        HWND hwnd = FindWindowW(L"zfredwnd", NULL);
        if (hwnd) {
            ShowWindow(hwnd, SW_SHOWNORMAL);
            SetForegroundWindow(hwnd);
        }
        return 0; // exit, already running!
    }
    #endif

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icex);

    //GdiplusStartupInput gdiplusStartupInput;
    //GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    MainWindow app(hInstance);
    if (!app.create())
        return 1;
    app.show(true); // Start hidden
    app.run();

    //GdiplusShutdown(gdiplusToken);
    return 0;
}
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
//    CommandLibrary commands;
//    MainWindow window(hInstance, commands);
//
//    if (!window.create())
//        return 1;
//
//    window.show(false); // Initially hidden
//    window.runMessageLoop();
//    return 0;
//}
