#include <shlobj.h> // For SHGetKnownFolderPath
#include <filesystem>
#include "history.h"
#include "fileactions.h"
#include "commandlibrary.h"

namespace sys_util {
	std::vector<CommandItem> loadSystemRecent();
}