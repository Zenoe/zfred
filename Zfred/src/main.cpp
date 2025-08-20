#include "mainwindow.h"
#include <commctrl.h>

//#include <gdiplus.h>
//using namespace Gdiplus;
//ULONG_PTR gdiplusToken;

// To make a ComboBox control appear flat. Using Visual Styles
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

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

#ifdef _DEBUG
    //AllocConsole();
#endif // _DEBUG

    //ICC_STANDARD_CLASSES
    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_LISTVIEW_CLASSES };
    //icex.dwICC = ICC_STANDARD_CLASSES;  // dnon't know what utility of the code
    InitCommonControlsEx(&icex);

    //GdiplusStartupInput gdiplusStartupInput;
    //GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    MainWindow app(hInstance);
    //ClipboardManager clip(app.GetHwnd(), &db);
    //clip.Start();

    if (!app.create())
        return 1;
    app.show(true); // Start hidden
    app.run();

#ifdef _DEBUG
    //FreeConsole();
#endif // _DEBUG
    //GdiplusShutdown(gdiplusToken);
    //clip.Stop();
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
