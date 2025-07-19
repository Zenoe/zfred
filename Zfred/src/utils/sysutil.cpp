#include "sysutil.h"


namespace sys_util {
    std::vector<CommandItem> loadSystemRecent() {
        std::vector<CommandItem> hist;
        // System recent files
        PWSTR recent_path = nullptr;
        HRESULT hr = SHGetKnownFolderPath(FOLDERID_Recent, KF_FLAG_DEFAULT, nullptr, &recent_path);
        if (SUCCEEDED(hr)) {
            for (const auto& entry : std::filesystem::directory_iterator(recent_path)) {
                if (entry.is_regular_file()) {
                    std::wstring path = entry.path();
                    std::wstring filename = entry.path().filename().wstring();
                    hist.push_back(CommandItem{
                        filename,
                        path,
                        [path] { fileactions::launch(path); },
                        false
                        });
                }
            }
            CoTaskMemFree(recent_path);
        }
        return hist;
    }
}
