#pragma once
#include <string>
#include <unordered_map>
#include <memory>

namespace tch {

/**
 * 本地化管理器
 * 负责加载和管理不同语言的资源，提供获取本地化文本的接口，支持语言热切换
 */
class LocalizationManager {
private:
    // 静态实例
    static std::unique_ptr<LocalizationManager> s_instance;
    
    // 当前语言
    std::string m_currentLanguage;
    
    // 语言资源存储
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_languages;
    
    // 私有构造函数
    LocalizationManager();
    
    // 加载语言资源文件
    bool loadLanguage(const std::string& langCode, const std::string& filePath);

public:
    // 获取实例
    static LocalizationManager& getInstance();
    
    // 初始化
    bool initialize();
    
    // 清理
    void cleanup();
    
    // 设置当前语言
    void setLanguage(const std::string& langCode);
    
    // 获取当前语言
    std::string getCurrentLanguage();
    
    // 获取本地化文本
    std::string get(const std::string& key);
    
    // 获取所有可用语言
    std::vector<std::string> getAvailableLanguages();
};

} // namespace tch
