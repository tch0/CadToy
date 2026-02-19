#include "utils/LocalizationManager.h"
#include "sys/Global.h"
#include "debug/Logger.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include <fstream>
#include <sstream>
#include <vector>

namespace tch {

// 静态实例初始化
std::unique_ptr<LocalizationManager> LocalizationManager::s_instance = nullptr;

// 构造函数
LocalizationManager::LocalizationManager() {
    m_currentLanguage = "en";
}

// 获取实例
LocalizationManager& LocalizationManager::getInstance() {
    if (!s_instance) {
        s_instance.reset(new LocalizationManager());
    }
    return *s_instance.get();
}

// 初始化
bool LocalizationManager::initialize() {
    // 构建语言资源文件路径
    std::filesystem::path langDir = g_pathCwd / "res" / "lang";
    
    // 检查语言资源目录是否存在
    if (!std::filesystem::exists(langDir)) {
        LOG_WARNING("Language directory not found: {}", langDir.string());
        return false;
    }
    
    // 加载英语资源
    std::filesystem::path enPath = langDir / "en.json";
    if (!loadLanguage("en", enPath.string())) {
        LOG_WARNING("Failed to load English language file: {}", enPath.string());
    }
    
    // 加载中文资源
    std::filesystem::path zhPath = langDir / "zh.json";
    if (!loadLanguage("zh", zhPath.string())) {
        LOG_WARNING("Failed to load Chinese language file: {}", zhPath.string());
    }
    
    // 检查是否成功加载了至少一种语言
    if (m_languages.empty()) {
        LOG_ERROR("No language files loaded!");
        return false;
    }
    
    LOG_INFO("LocalizationManager initialized successfully. Loaded {} languages.", m_languages.size());
    return true;
}

// 清理
void LocalizationManager::cleanup() {
    m_languages.clear();
    s_instance.reset();
}

// 加载语言资源文件
bool LocalizationManager::loadLanguage(const std::string& langCode, const std::string& filePath) {
    // 检查文件是否存在
    if (!std::filesystem::exists(filePath)) {
        return false;
    }
    
    // 打开文件
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    // 读取文件内容
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    // 解析JSON
    rapidjson::Document doc;
    rapidjson::ParseResult result = doc.Parse(content.c_str());
    if (!result) {
        LOG_ERROR("Failed to parse language file: {}", filePath);
        return false;
    }
    
    // 检查是否为对象
    if (!doc.IsObject()) {
        LOG_ERROR("Language file is not a valid JSON object: {}", filePath);
        return false;
    }
    
    // 提取键值对
    std::unordered_map<std::string, std::string> langMap;
    for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
        if (it->name.IsString() && it->value.IsString()) {
            std::string key = it->name.GetString();
            std::string value = it->value.GetString();
            langMap[key] = value;
        }
    }
    
    // 添加到语言映射
    m_languages[langCode] = langMap;
    LOG_INFO("Loaded language: {} with {} entries", langCode, langMap.size());
    return true;
}

// 设置当前语言
void LocalizationManager::setLanguage(const std::string& langCode) {
    if (m_languages.find(langCode) != m_languages.end()) {
        m_currentLanguage = langCode;
        LOG_INFO("Switched to language: {}", langCode);
    } else {
        LOG_WARNING("Language not found: {}", langCode);
    }
}

// 获取当前语言
std::string LocalizationManager::getCurrentLanguage() {
    return m_currentLanguage;
}

// 获取本地化文本
std::string LocalizationManager::get(const std::string& key) {
    // 检查当前语言是否存在
    if (m_languages.find(m_currentLanguage) == m_languages.end()) {
        return key;
    }
    
    // 检查键是否存在
    const auto& langMap = m_languages[m_currentLanguage];
    if (langMap.find(key) == langMap.end()) {
        return key;
    }
    
    // 返回本地化文本
    return langMap.at(key);
}

// 获取所有可用语言
std::vector<std::string> LocalizationManager::getAvailableLanguages() {
    std::vector<std::string> languages;
    for (const auto& pair : m_languages) {
        languages.push_back(pair.first);
    }
    return languages;
}

} // namespace tch
