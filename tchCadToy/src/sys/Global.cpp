#include <SysConfig.h>
#include <Logger.h>
#include <Global.h>

namespace fs = std::filesystem;


// =========================================================================================================
// ------------------------------------- commands
// =========================================================================================================
std::map<std::string, std::pair<Command*, int>>& getCommandsMap()
{
    static std::map<std::string, std::pair<Command*, int>> s_CommandsMap;
    return s_CommandsMap;
}

// =========================================================================================================
// ------------------------------------- global functions
// =========================================================================================================

// check which OS current is
void checkOS()
{
    globalLogger().info(std::format("system: {}", SYSTEM_NAME));
}

// build current working directory from exe path
void buildCwd(const char* exePath)
{
    g_PathCwd = fs::absolute(fs::path(exePath));
    g_PathCwd.remove_filename();
    globalLogger().info(std::format("cwd: {}", g_PathCwd.string()));
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