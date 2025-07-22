#include "sysutil.h"

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
            }
            CoTaskMemFree(recent_path);
        }
        return hist;
    }
}
