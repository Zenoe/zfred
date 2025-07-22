#include <shlobj.h> // For SHGetKnownFolderPath
#include <filesystem>
#include "fileactions.h"

namespace sys_util {
	std::vector<std::wstring> loadSystemRecent();
}