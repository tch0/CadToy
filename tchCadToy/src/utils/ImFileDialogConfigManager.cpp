// 对应头文件
#include "ImFileDialogConfigManager.h"

// C++ 标准库
#include <filesystem>

// 第三方库
#include <ImFileDialog.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

// 项目头文件
#include "Global.h"
#include "GlobalUtils.h"
#include "Logger.h"
#include "PlatformUtils.h"


namespace tch {

ImFileDialogConfigManager& ImFileDialogConfigManager::getInstance() {
    static ImFileDialogConfigManager instance;
    return instance;
}

ImFileDialogConfigManager::ImFileDialogConfigManager() {
}

ImFileDialogConfigManager::~ImFileDialogConfigManager() {
    if (!m_configFilePath.empty()) {
        saveToFile();
    }
}

void ImFileDialogConfigManager::initialize() {
    PlatformUtils::Path configPath = PlatformUtils::Path(g_pathCwd) / "config" / "ImFileDialogConfig.json";
    m_configFilePath = configPath.string();
    
    loadFromFile();
}

void ImFileDialogConfigManager::ensureConfigDirExists() {
    PlatformUtils::Path configDir = PlatformUtils::Path(g_pathCwd) / "config";
    if (!configDir.exists()) {
        std::filesystem::create_directories(configDir.local());
        LOG_INFO("ImFileDialogConfigManager: Created config directory: {}", configDir.string());
    }
}

void ImFileDialogConfigManager::loadFromFile() {
    std::string content;
    if (!Utils::readTextFile(m_configFilePath, content)) {
        LOG_INFO("ImFileDialogConfigManager: Config file not found, will create on save: {}", m_configFilePath);
        return;
    }
    
    rapidjson::Document doc;
    if (doc.Parse(content.c_str()).HasParseError() || !doc.IsObject()) {
        LOG_ERROR("ImFileDialogConfigManager: Failed to parse config file: {}", m_configFilePath);
        return;
    }
    
    if (!doc.HasMember("favorites") || !doc["favorites"].IsArray()) {
        return;
    }
    
    const rapidjson::Value& favorites = doc["favorites"];
    for (rapidjson::SizeType i = 0; i < favorites.Size(); i++) {
        if (favorites[i].IsString()) {
            ifd::FileDialog::getInstance().addFavorite(favorites[i].GetString());
        }
    }
    
    LOG_INFO("ImFileDialogConfigManager: Loaded {} favorites from {}", favorites.Size(), m_configFilePath);
}

void ImFileDialogConfigManager::saveToFile() {
    if (m_configFilePath.empty()) {
        return;
    }
    
    const auto& u8Favorites = ifd::FileDialog::getInstance().getFavorites();
    if (u8Favorites.empty()) {
        return;
    }
    
    ensureConfigDirExists();
    
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    
    writer.StartObject();
    writer.Key("favorites");
    writer.StartArray();
    for (const auto& u8Path : u8Favorites) {
        writer.String(ifd::u8_to_string(u8Path).c_str());
    }
    writer.EndArray();
    writer.EndObject();
    
    if (!Utils::writeTextFile(m_configFilePath, buffer.GetString())) {
        LOG_ERROR("ImFileDialogConfigManager: Failed to write config file: {}", m_configFilePath);
        return;
    }
    
    LOG_INFO("ImFileDialogConfigManager: Saved {} favorites to {}", u8Favorites.size(), m_configFilePath);
}

} // namespace tch
