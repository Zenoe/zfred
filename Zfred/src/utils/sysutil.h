#include <shlobj.h> // For SHGetKnownFolderPath
#include <filesystem>
#include "fileactions.h"

namespace sys_util {
	std::vector<std::wstring> loadSystemRecent();
	std::vector<std::wstring> GetQuickAccessItems();
	bool Write2Clipboard(const std::wstring& );
}
