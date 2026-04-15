#pragma once

// C++ 标准库
#include <string>

// 第三方库

// 项目头文件

namespace tch {

class ImFileDialogConfigManager {
public:
    static ImFileDialogConfigManager& getInstance();
    
    ImFileDialogConfigManager(const ImFileDialogConfigManager&) = delete;
    ImFileDialogConfigManager& operator=(const ImFileDialogConfigManager&) = delete;
    
    // 初始化，加载配置
    void initialize();

private:
    ImFileDialogConfigManager();
    ~ImFileDialogConfigManager();
    
    // 从文件加载收藏夹配置
    void loadFromFile();
    
    // 保存收藏夹配置到文件
    void saveToFile();
    
    // 确保配置目录存在
    void ensureConfigDirExists();
    
    std::string m_configFilePath;
};

} // namespace tch
