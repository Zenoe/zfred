#pragma once
#include <Windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>

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

    // Split into at most two lines by '\n'
    static std::vector<std::wstring> ExtractTwoLines(const std::wstring& input);

    // Combine two lines with the '↓' character
    static std::wstring CombineLinesWithArrow(const std::vector<std::wstring>& lines);

    // Truncate string to fit pixel width, add ellipsis '...'
    static std::wstring TruncateWithEllipsis(HDC hdc, const std::wstring& text, int maxWidth);

    // Draw text with highlight mask
    static void DrawHighlightedText(HDC hdc, int x, int y, const std::wstring& text,
                                    const std::vector<bool>& highlight_mask,
                                    COLORREF textColor, COLORREF highlightColor);
    //static HBITMAP CreateRoundRectBitmap(int width, int height, int radius, Gdiplus::Color cFill);
    //static void ApplyLayered(HWND hwnd, int w, int h, int radius, Gdiplus::Color c);
    //static void DrawPartialRoundRect(HDC hdc, int x, int y, int w, int h, int radius, bool tl, bool tr, bool bl, bool br, Gdiplus::Color color);
    
};
