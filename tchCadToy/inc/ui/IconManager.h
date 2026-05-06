#pragma once

// C++ 标准库
#include <map>

// 第三方库
#include <glad/gl.h>
#include <imgui.h>

// 项目头文件
#include "IconDefines.h"

namespace tch {

// ================================================================================================
// 图标管理器类（单例）
// 负责加载、存储和提供 PNG 图标纹理
// ================================================================================================
class IconManager {
public:
    // 获取单例实例
    static IconManager& getInstance();

    // 禁止拷贝
    IconManager(const IconManager&) = delete;
    IconManager& operator=(const IconManager&) = delete;

    // ================================================================================================
    // 生命周期
    // ================================================================================================

    // 初始化图标管理器，加载所有在 g_iconPaths 中定义的图标
    void initialize();

    // 释放所有已分配的图形资源
    void cleanup();

    // ================================================================================================
    // 查询接口
    // ================================================================================================

    // 获取指定图标的 ImGui 纹理 ID，如果未加载则返回 0
    ImTextureID getIcon(IconID id) const;

    // 检查是否已成功初始化
    bool isInitialized() const { return m_initialized; }

private:
    // 私有构造函数
    IconManager() = default;

    // 析构时自动清理
    ~IconManager() { cleanup(); }

    // 加载单个图标
    bool loadIcon(IconID id, const std::string& relPath);

    // ================================================================================================
    // 成员变量
    // ================================================================================================
    std::map<IconID, GLuint> m_textures;    // 图标纹理映射表
    bool m_initialized = false;             // 初始化标志
};

} // namespace tch
