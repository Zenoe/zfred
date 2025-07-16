#include "mainwindow.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
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
    MainWindow app(hInst);
    if (!app.create())
        return 1;
    app.show(true); // Start hidden
    app.run();
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