#pragma once
#include <windows.h>
#include <thread>
#include "utils/dbsqlite.h"

class ClipboardManager {
public:
    ClipboardManager(HWND hwnd, Database* db);
    void Start();
    void Stop();
private:
    HWND hwnd_;
    Database* db_;
    std::thread thread_;
    bool running_;
    void Monitor();
};