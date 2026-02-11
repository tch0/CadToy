#include "SysConfig.h"
#include "debug/Logger.h"
#include "sys/Global.h"

namespace fs = std::filesystem;

// check which OS current is
void checkOS()
{
    globalLogger().info(std::format("system: {}", SYSTEM_NAME));
}

// build current working directory from exe path
void buildCwd(const char* exePath)
{
    g_pathCwd = fs::absolute(fs::path(exePath));
    g_pathCwd.remove_filename();
    globalLogger().info(std::format("cwd: {}", g_pathCwd.string()));
}

// create a directory p if it does not exist
void createDirIfNotExist(const fs::path& p)
{
    try
    {
        std::error_code ec;
        if (fs::exists(p, ec))
        {
            if (fs::is_directory(p, ec))
            {
                return;
            }
            globalLogger().warning(std::format("file {} exists, but not a directory, we will delete it and create a same name directory!", (p).string()));
            fs::remove(p, ec);
        }
        if (!fs::create_directories(p, ec))
        {
            globalLogger().warning(std::format("create directory failed, error code: {}, {}!", ec.value(), ec.message()));
        }
    }
    catch(const fs::filesystem_error& e)
    {
        globalLogger().warning(std::format("filesystem_error exception caught: \nwhat: {}\npath1: {}\npath2: {}\n", e.what(), e.path1().string(), e.path2().string()));
    }
}

// create important resource paths
void checkAndCreateImportantDirs()
{
}
