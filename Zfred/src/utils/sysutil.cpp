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
}
