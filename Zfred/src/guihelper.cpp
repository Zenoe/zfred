#include "guihelper.h"
#include <shlobj.h>
#include <filesystem>
#include "FsUtils.h"

#include "debugtool.h"
using namespace Gdiplus;

#define ID_OPENFILEPATH   9001
#define ID_COPYFILEPATH   9002

//#pragma comment(lib, "gdiplus.lib")
namespace fs = std::filesystem;


void invoke_folder_verb(HWND hwnd, const std::wstring& folder_path, LPCWSTR wverb, LPCSTR verb) {
    PIDLIST_ABSOLUTE pidlFolder = nullptr;
    if (SUCCEEDED(SHParseDisplayName(folder_path.c_str(), nullptr, &pidlFolder, 0, nullptr))) {
        IShellFolder* psfParent = nullptr;
        LPCITEMIDLIST pidlChild = nullptr;
        if (SUCCEEDED(SHBindToParent(pidlFolder, IID_IShellFolder, (void**)&psfParent, &pidlChild))) {
            IContextMenu* pcmFolder = nullptr;
            if (SUCCEEDED(psfParent->GetUIObjectOf(hwnd, 1, &pidlChild, IID_IContextMenu, nullptr, (void**)&pcmFolder))) {
                HMENU hMenu = CreatePopupMenu();
                if (hMenu) {
                    if (SUCCEEDED(pcmFolder->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_NORMAL))) {
                        CMINVOKECOMMANDINFOEX cmi = { 0 };
                        cmi.cbSize = sizeof(cmi);
                        cmi.fMask = CMIC_MASK_UNICODE;
                        cmi.hwnd = hwnd;
                        //cmi.lpVerb = "QTTabBar.openInView";
                        //cmi.lpVerbW = L"QTTabBar.openInView";
                        // both narrow/wide verb are needed to be set
                        cmi.lpVerbW = wverb;
                        cmi.lpVerb = verb;
                        cmi.nShow = SW_SHOWNORMAL;
                        cmi.lpDirectoryW = folder_path.c_str();
                        pcmFolder->InvokeCommand((LPCMINVOKECOMMANDINFO)&cmi);
                    }
                    DestroyMenu(hMenu);
                }
                pcmFolder->Release();
            }
            psfParent->Release();
        }
        CoTaskMemFree(pidlFolder);
    }
}

void GuiHelper::openFolder(HWND hwndParent, const std::wstring& folder_path){
    bool qttab = true;
    // todo
    if (qttab)
        invoke_folder_verb(hwndParent, folder_path, L"QTTabBar.openInView", "QTTabBar.openInView");
    else
        invoke_folder_verb(hwndParent, folder_path, L"open", "open");
}
void GuiHelper::ShowShellContextMenu(HWND hwndParent, HWND hListView, std::wstring& path, int x, int y)
{
    fs::path p(path);
    p.make_preferred();
    std::wstring converted_path = p.wstring();
    LPITEMIDLIST pidlItem = NULL;
    HRESULT hr = SHParseDisplayName(converted_path.c_str(), NULL, &pidlItem, 0, NULL);
    if (FAILED(hr) || !pidlItem) return;
    LPITEMIDLIST pidlParent = ILClone(pidlItem);
    ILRemoveLastID(pidlParent);

    IShellFolder *psfFolder = NULL;
    hr = SHBindToObject(NULL, pidlParent, NULL, IID_IShellFolder, (void**)&psfFolder);
    if (FAILED(hr) || !psfFolder) { CoTaskMemFree(pidlParent); CoTaskMemFree(pidlItem); return; }
    LPCITEMIDLIST pidlRel = ILFindLastID(pidlItem);

    IContextMenu *pcm = NULL;
    hr = psfFolder->GetUIObjectOf(hwndParent, 1, &pidlRel, IID_IContextMenu, NULL, (void**)&pcm);
    if (SUCCEEDED(hr) && pcm)
      {
        HMENU hMenu = CreatePopupMenu();

        InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_OPENFILEPATH, L"Open filepath");
        InsertMenu(hMenu, 1, MF_BYPOSITION | MF_STRING, ID_COPYFILEPATH, L"Copy filepath");
        InsertMenu(hMenu, 2, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
        pcm->QueryContextMenu(hMenu, 3, 1, 0x7FFF, CMF_EXPLORE);

        bool qttab = true; // temporately set true
        int menuCount = GetMenuItemCount(hMenu);
        for (int i = 3; i < menuCount; ++i) {
            UINT cmdId = GetMenuItemID(hMenu, i);
            if (cmdId == -1) continue; // Separator
            WCHAR verbW[256] = { 0 };
            // todo when click a file ,there's no QTTabBar.openInView
			if (SUCCEEDED(pcm->GetCommandString(cmdId, GCS_VERBW, NULL, (char*)verbW, ARRAYSIZE(verbW)))) {
				OutputDebugPrint("----------verb: ", verbW);
				// check if QTTabBar exist
				if (lstrcmpW(verbW, L"QTTabBar.openInView") == 0) {
					qttab = true;
					break;
				}
			}
		}
		int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN, x, y, 0, hwndParent, NULL);
		if (cmd)
		{
			if (cmd == ID_OPENFILEPATH) {
				std::wstring dir = FsUtils::is_dir(converted_path) ? converted_path : FsUtils::get_parent_path(converted_path);
                openFolder(hwndParent, dir);
				// want the context menu for a file, but to act as if it's on the parent folder (for example, show the context menu for the folder, not the file), you need to obtain the PIDL for the folder (not the file), and use it when assembling the context menu.

    //            // Delegate "open" to shell verb
    //            CMINVOKECOMMANDINFOEX cmi = { 0 };
    //            std::wstring dir = converted_path;
    //            if (!FsUtils::is_dir(converted_path)) {
    //                // not dir ,get parent dir
    //                dir = FsUtils::get_parent_path(converted_path);
				//	//std::wstring cmd = L"/select,\"" + dir + L"\"";
				//	//ShellExecute(NULL, L"open", L"explorer.exe", cmd.c_str(), NULL, SW_SHOWNORMAL);
    //            }
    //            //else {
				//	cmi.cbSize = sizeof(cmi);
				//	cmi.fMask = CMIC_MASK_UNICODE;
				//	cmi.hwnd = hwndParent;
				//	cmi.lpVerb = "QTTabBar.openInView";
				//	cmi.lpVerbW = L"QTTabBar.openInView";
				//	cmi.lpDirectoryW = dir.c_str();
				//	cmi.lpDirectory = NULL;
				//	cmi.nShow = SW_SHOWNORMAL;
				//	pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&cmi);
				////}
			}
			else if (cmd == ID_COPYFILEPATH) {
				if (OpenClipboard(hwndParent)) {
					EmptyClipboard();
					size_t sz = (converted_path.length() + 1) * sizeof(wchar_t);
					HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sz);
					if (hMem) {
						wchar_t* pMem = (wchar_t*)GlobalLock(hMem);
						if (pMem) {
							memcpy(pMem, converted_path.c_str(), sz);
							GlobalUnlock(hMem);
							SetClipboardData(CF_UNICODETEXT, hMem);
						}
					}
					CloseClipboard();
				}
			}
			else {
				CMINVOKECOMMANDINFOEX cmi = { 0 };
				cmi.cbSize = sizeof(cmi);
				cmi.fMask = CMIC_MASK_UNICODE;
                cmi.hwnd = hwndParent;
                cmi.lpVerb = MAKEINTRESOURCEA(cmd - 1);
                if (FsUtils::is_dir(converted_path)) {
                    // only when openning a dir should this property be set
                    // This property should only be set when opening a directory
                    std::wstring tmp = FsUtils::get_parent_path(converted_path);
                    cmi.lpDirectoryW = tmp.c_str();
                }
                cmi.lpDirectory = NULL;
                cmi.nShow = SW_SHOWNORMAL;

                pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&cmi);
            }
        }
        DestroyMenu(hMenu);
        pcm->Release();
    }
    psfFolder->Release();
    CoTaskMemFree(pidlParent);
    CoTaskMemFree(pidlItem);
}
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

// Split into at most two lines by '\n'
std::vector<std::wstring> GuiHelper::ExtractTwoLines(const std::wstring& input) {
    std::vector<std::wstring> lines;
    std::wstring currentLine;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == L'\n') {
            lines.push_back(currentLine);
            currentLine.clear();
            if (lines.size() == 2) break; // Maximum two lines
        } else {
            currentLine += input[i];
        }
    }
    if (!currentLine.empty() && lines.size() < 2)
        lines.push_back(currentLine);
    return lines;
}

// Combine two lines with the '↓' character
std::wstring GuiHelper::CombineLinesWithArrow(const std::vector<std::wstring>& lines) {
    if (lines.empty()) return L"";
    if (lines.size() == 1) return lines[0];
    return lines[0] + L'↓' + lines[1];
}

// Truncate string to fit pixel width, add ellipsis '...'
std::wstring GuiHelper::TruncateWithEllipsis(HDC hdc, const std::wstring& text, int maxWidth) {
    SIZE textSize;
    GetTextExtentPoint32W(hdc, text.c_str(), text.length(), &textSize);
    if (textSize.cx <= maxWidth)
        return text;

    std::wstring temp;
    for (size_t i = 0; i < text.length(); ++i) {
        std::wstring test = temp + text[i] + L"...";
        GetTextExtentPoint32W(hdc, test.c_str(), test.length(), &textSize);
        if (textSize.cx > maxWidth) {
            break;
        }
        temp += text[i];
    }
    return temp + L"...";
}

// Draw text with highlight mask
void GuiHelper::DrawHighlightedText(HDC hdc, int x, int y, const std::wstring& text,
                         const std::vector<bool>& highlight_mask,
                         COLORREF textColor, COLORREF highlightColor)
{
    for (size_t charIdx = 0; charIdx < text.length(); ++charIdx) {
        size_t originalPos = charIdx; // Simple mapping (see note in your logic)
        bool isHighlighted = (originalPos < highlight_mask.size() && highlight_mask[originalPos]);
        SetTextColor(hdc, isHighlighted ? highlightColor : textColor);
        wchar_t chr = text[charIdx];
        TextOutW(hdc, x, y, &chr, 1);
        SIZE charSize;
        GetTextExtentPoint32W(hdc, &chr, 1, &charSize);
        x += charSize.cx;
    }
}
