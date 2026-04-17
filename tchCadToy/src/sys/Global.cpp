// 对应头文件

// C++ 标准库
#include <filesystem>

// 第三方库
#include <ImFileDialog.h>

// 项目头文件
#include "SysConfig.h"
#include "Logger.h"
#include "Global.h"
#include "GlobalUtils.h"
#include "ImFileDialogConfigManager.h"


using namespace tch;

// check which OS current is
void checkOS()
{
    globalLogger().info(std::format("system: {}", SYSTEM_NAME));
}

// check the endian of the system
void checkSystemEndian()
{
    uint32_t x = 0x11223344;
    char* p = reinterpret_cast<char*>(&x);
    if (*p == 0x11)
    {
        g_bBigEndian = true;
    }
    else
    {
        g_bBigEndian = false;
    }
    globalLogger().info(std::format("The endian of system: {}", g_bBigEndian ? "big endian" : "little endian"));
}

// build current working directory from exe path
void buildCwd(const char* exePath)
{
    g_pathCwd = std::filesystem::absolute(std::filesystem::path(exePath));
    g_pathCwd.remove_filename();
    globalLogger().info(std::format("cwd: {}", PathUtils::toString(g_pathCwd)));
}

// create a directory p if it does not exist
void createDirIfNotExist(const std::filesystem::path& p)
{
    try
    {
        std::error_code ec;
        if (std::filesystem::exists(p, ec))
        {
            if (std::filesystem::is_directory(p, ec))
            {
                return;
            }
            globalLogger().warning(std::format("file {} exists, but not a directory, we will delete it and create a same name directory!", PathUtils::toString(p)));
            std::filesystem::remove(p, ec);
        }
        if (!std::filesystem::create_directories(p, ec))
        {
            globalLogger().warning(std::format("create directory failed, error code: {}, {}!", ec.value(), ec.message()));
        }
    }
    catch(const std::filesystem::filesystem_error& e)
    {
        globalLogger().warning(std::format("filesystem_error exception caught: \nwhat: {}\npath1: {}\npath2: {}\n", e.what(), PathUtils::toString(e.path1()), PathUtils::toString(e.path2())));
    }
}

// create important resource paths
void checkAndCreateImportantDirs()
{
    createDirIfNotExist(g_pathCwd / "config");
    createDirIfNotExist(g_pathCwd / "fonts");
    createDirIfNotExist(g_pathCwd / "res");
}

// initialize ImFileDialog texture callbacks
void initializeImFileDialog()
{
    ifd::FileDialog::getInstance().createTexture = [](const uint8_t* data, int w, int h, ifd::Format fmt) -> void* {
        GLuint tex;
    
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        if (fmt == ifd::Format::BGRA) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
        } else if (fmt == ifd::Format::RGBA) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
    
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    
        return reinterpret_cast<void*>(static_cast<uintptr_t>(tex));
    };
    ifd::FileDialog::getInstance().deleteTexture = [](void* tex) {
        GLuint texID = static_cast<GLuint>(reinterpret_cast<uintptr_t>(tex));
        glDeleteTextures(1, &texID);
    };
    
    // 初始化ImFileDialog配置管理器
    ImFileDialogConfigManager::getInstance().initialize();
}
