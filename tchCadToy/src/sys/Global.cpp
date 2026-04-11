// 对应头文件

// C++ 标准库
#include <filesystem>

// 第三方库
#include <ImFileDialog.h>

// 项目头文件
#include "SysConfig.h"
#include "Logger.h"
#include "Global.h"
#include "PlatformUtils.h"

using namespace tch;
namespace fs = std::filesystem;

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
    fs::path cwdPath = fs::absolute(fs::path(exePath));
    cwdPath.remove_filename();
    // 转换为 UTF-8 存储
    g_pathCwd = PlatformUtils::Path::fromLocal(cwdPath.string()).string();
    globalLogger().info(std::format("cwd: {}", g_pathCwd));
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
    
        return (void*)tex;
    };
    ifd::FileDialog::getInstance().deleteTexture = [](void* tex) {
        GLuint texID = (GLuint)tex;
        glDeleteTextures(1, &texID);
    };
}
