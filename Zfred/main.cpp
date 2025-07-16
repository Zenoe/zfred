#include "mainwindow.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    MainWindow app(hInst);
    if (!app.create())
        return 1;
    app.show(false); // Start hidden
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