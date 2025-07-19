#pragma once
#include <Windows.h>
#include <gdiplus.h>

class GuiHelper {
public:
    static void SetWindowRoundRgn(HWND hwnd, int width, int height, int radius);

    //static HBITMAP CreateRoundRectBitmap(int width, int height, int radius, Gdiplus::Color cFill);
    //static void ApplyLayered(HWND hwnd, int w, int h, int radius, Gdiplus::Color c);
    //static void DrawPartialRoundRect(HDC hdc, int x, int y, int w, int h, int radius, bool tl, bool tr, bool bl, bool br, Gdiplus::Color color);
    
};