// 对应头文件
#include "IconManager.h"

// C++ 标准库
#include <filesystem>

// 第三方库
#include <stb_image.h>

// 项目头文件
#include "Global.h"
#include "Logger.h"

namespace tch {

// ================================================================================================
// 单例实现
// ================================================================================================

IconManager& IconManager::getInstance() {
    static IconManager instance;
    return instance;
}

// ================================================================================================
// 生命周期
// ================================================================================================

void IconManager::initialize() {
    if (m_initialized) {
        cleanup();
    }

    m_initialized = true;

    size_t loadedCount = 0;
    for (const auto& [id, relPath] : g_iconPaths) {
        if (loadIcon(id, relPath)) {
            ++loadedCount;
        }
    }

    LOG_INFO("IconManager: Loaded {}/{} icons", loadedCount, g_iconPaths.size());
}

void IconManager::cleanup() {
    for (auto& [id, tex] : m_textures) {
        if (tex != 0) {
            glDeleteTextures(1, &tex);
        }
    }
    m_textures.clear();
    m_initialized = false;
}

// ================================================================================================
// 查询接口
// ================================================================================================

ImTextureID IconManager::getIcon(IconID id) const {
    auto it = m_textures.find(id);
    if (it != m_textures.end() && it->second != 0) {
        return static_cast<ImTextureID>(it->second);
    }
    return 0;
}

// ================================================================================================
// 内部方法
// ================================================================================================

bool IconManager::loadIcon(IconID id, const std::string& relPath) {
    // 构建完整路径
    std::filesystem::path fullPath = g_pathCwd / relPath;
    std::string pathStr = fullPath.string();

    // 加载图片数据
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(pathStr.c_str(), &width, &height, &channels, 4); // 强制 RGBA
    if (!data) {
        LOG_WARNING("IconManager: Failed to load icon: {}", pathStr);
        return false;
    }

    // 创建 OpenGL 纹理
    GLuint tex = 0;
    glGenTextures(1, &tex);
    if (tex == 0) {
        LOG_WARNING("IconManager: Failed to create texture for: {}", pathStr);
        stbi_image_free(data);
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    // 释放图片数据
    stbi_image_free(data);

    // 保存纹理 ID
    m_textures[id] = tex;

    return true;
}

} // namespace tch
