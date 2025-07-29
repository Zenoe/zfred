#include "guihelper.h"
using namespace Gdiplus;

//#pragma comment(lib, "gdiplus.lib")

// this would make the corner jagged
void GuiHelper::SetWindowRoundRgn(HWND hwnd, int width, int height, int radius)
{
    HRGN hRgn = CreateRoundRectRgn(0, 0, width, height, radius, radius);
    SetWindowRgn(hwnd, hRgn, TRUE);
}

/*
HBITMAP GuiHelper::CreateRoundRectBitmap(int width, int height, int radius, Color cFill)
{
    Bitmap bmp(width, height, PixelFormat32bppARGB);
    Graphics g(&bmp);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    GraphicsPath path;
    path.AddArc(0, 0, 2 * radius, 2 * radius, 180, 90);
    path.AddArc(width - 2 * radius - 1, 0, 2 * radius, 2 * radius, 270, 90);
    path.AddArc(width - 2 * radius - 1, height - 2 * radius - 1, 2 * radius, 2 * radius, 0, 90);
    path.AddArc(0, height - 2 * radius - 1, 2 * radius, 2 * radius, 90, 90);
    path.CloseFigure();

    SolidBrush fill(cFill);
    g.FillPath(&fill, &path);
    g.Flush();

    HBITMAP hBmp = NULL;
    bmp.GetHBITMAP(Color(0, 0, 0, 0), &hBmp);
    return hBmp;
}

// Apply GDI+ alpha bitmap as window background
void GuiHelper::ApplyLayered(HWND hwnd, int w, int h, int radius, Color c)
{
    POINT ptZero = { 0, 0 };
    SIZE sz = { w, h };
    HDC hdcScreen = GetDC(0);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    HBITMAP hBmp = CreateRoundRectBitmap(w, h, radius, c);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    UpdateLayeredWindow(hwnd, hdcScreen, NULL, &sz, hdcMem, &ptZero, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(0, hdcScreen);
}

// Draws a rounded rect only on some corners (control over this area will look smooth)
void GuiHelper::DrawPartialRoundRect(HDC hdc, int x, int y, int w, int h, int radius, bool tl, bool tr, bool bl, bool br, Color color)
{
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    GraphicsPath path;
    // Build the path manually for each corner
    if (tl)
        path.AddArc(x, y, 2 * radius, 2 * radius, 180, 90);
    else
        path.AddLine(x, y, x + radius, y);

    if (tr)
        path.AddArc(x + w - 2 * radius - 1, y, 2 * radius, 2 * radius, 270, 90);
    else
        path.AddLine(x + w - radius, y, x + w, y);

    if (br)
        path.AddArc(x + w - 2 * radius - 1, y + h - 2 * radius - 1, 2 * radius, 2 * radius, 0, 90);
    else
        path.AddLine(x + w, y + h - radius, x + w, y + h);

    if (bl)
        path.AddArc(x, y + h - 2 * radius - 1, 2 * radius, 2 * radius, 90, 90);
    else
        path.AddLine(x + radius, y + h, x, y + h);

    path.CloseFigure();

    SolidBrush fill(color);
    g.FillPath(&fill, &path);
}
*/