#include "sysutil.h"

#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <vector>
#include <string>
namespace sys_util {
    std::vector<std::wstring> loadSystemRecent() {
        //std::vector<CommandItem> hist;
        std::vector<std::wstring> hist;
        // System recent files
        PWSTR recent_path = nullptr;
        HRESULT hr = SHGetKnownFolderPath(FOLDERID_Recent, KF_FLAG_DEFAULT, nullptr, &recent_path);
        if (SUCCEEDED(hr)) {
            for (const auto& entry : std::filesystem::directory_iterator(recent_path)) {
                if (entry.is_regular_file()) {
                    std::wstring path = entry.path().wstring();
                    //std::wstring filename = entry.path().filename().wstring();
                    hist.push_back(path);
                }
                else {
                    OutputDebugString(entry.path().wstring().c_str());
                }
            }
            CoTaskMemFree(recent_path);
        }
        return hist;
    }

    std::vector<std::wstring> GetQuickAccessItems()
    {
        std::vector<std::wstring> result;

        HRESULT hr = CoInitialize(NULL);
        bool bCoInit = SUCCEEDED(hr);

        IShellItem* psiQuickAccess = nullptr;
        hr = SHCreateItemFromParsingName(
            L"shell:::{679f85cb-0220-4080-b29b-5540cc05aab6}",
            NULL, IID_PPV_ARGS(&psiQuickAccess));
        if (SUCCEEDED(hr) && psiQuickAccess) {
            IEnumShellItems* pEnum = nullptr;
            hr = psiQuickAccess->BindToHandler(
                NULL, BHID_EnumItems, IID_PPV_ARGS(&pEnum));
            if (SUCCEEDED(hr) && pEnum) {
                IShellItem* psiItem = nullptr;
                while (pEnum->Next(1, &psiItem, NULL) == S_OK) {
                    LPWSTR displayName = nullptr;
                    if (SUCCEEDED(psiItem->GetDisplayName(SIGDN_FILESYSPATH, &displayName)) && displayName) {
                        result.emplace_back(displayName);
                        CoTaskMemFree(displayName);
                    }
                    psiItem->Release();
                }
                pEnum->Release();
            }
            psiQuickAccess->Release();
        }

        if (bCoInit) CoUninitialize();
        return result;
    }

    bool Write2Clipboard(const std::wstring& text){
        if(text.empty()) return false;
        if (!OpenClipboard(nullptr)) return false;
        EmptyClipboard();

        // 分配全局内存（包含结尾的 L'\0'）
        size_t data_size = (text.size() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, data_size);
        if (!hMem) {
            CloseClipboard();
            return false;
        }

        // 将内容拷贝到分配的内存
        void* pMem = GlobalLock(hMem);
        memcpy(pMem, text.c_str(), data_size);
        GlobalUnlock(hMem);

        // 设置剪贴板内容为 Unicode 文本
        SetClipboardData(CF_UNICODETEXT, hMem);

        // 剪贴板现在拥有数据的所有权，不需要手动 GlobalFree(hMem)
        CloseClipboard();
    }
}
