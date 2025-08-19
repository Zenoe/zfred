#pragma once
#include <Windows.h>
#include <gdiplus.h>
#include <string>

//0xF0F4FD  light blue (bg)
//0x3370FF  blue(text)
//0xEDFBF3  light green
// 0x039855 green

#define HEXTOCOLORREF(x) RGB(((x) >> 16) & 0xFF, ((x) >> 8) & 0xFF, (x) & 0xFF)

class GuiHelper {
public:
    static void openFolder(HWND hwndParent,const std::wstring& folder_path);
    static void ShowShellContextMenu(HWND hwndParent, HWND hListView, std::wstring& path, int x, int y);
    static void SetWindowRoundRgn(HWND hwnd, int width, int height, int radius);

    //static HBITMAP CreateRoundRectBitmap(int width, int height, int radius, Gdiplus::Color cFill);
    //static void ApplyLayered(HWND hwnd, int w, int h, int radius, Gdiplus::Color c);
    //static void DrawPartialRoundRect(HDC hdc, int x, int y, int w, int h, int radius, bool tl, bool tr, bool bl, bool br, Gdiplus::Color color);
    
};
