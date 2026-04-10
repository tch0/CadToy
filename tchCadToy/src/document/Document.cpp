// 对应头文件
#include "Document.h"

// C++ 标准库
#include <algorithm>
#include <filesystem>
#include <fstream>

// 第三方库
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

// 项目头文件
#include "Logger.h"


namespace tch {

// 默认构造函数：创建空文档，不构造 Database
Document::Document() :
    m_fileName("unnamed-empty"),
    m_fileExtension(".cad.json"),
    m_modified(false),
    m_saved(false),
    m_showGrid(true),
    m_showAxes(true) {
    // m_database 不构造，保持 nullptr
}

// 构造函数：新建文档，使用指定文件名
Document::Document(const std::string& fileName) :
    m_fileName(fileName),
    m_fileExtension(".cad.json"),
    m_modified(false),
    m_saved(false),
    m_showGrid(true),
    m_showAxes(true),
    m_database(std::make_unique<Database>()) {
}

// 从路径解析文件名和后缀
void Document::parseFilePath(const std::string& path) {
    m_fullPath = path;
    
    std::filesystem::path filePath(path);
    std::string filename = filePath.filename().string();
    
    size_t dotPos = filename.rfind('.');
    if (dotPos != std::string::npos) {
        m_fileName = filename.substr(0, dotPos);
        m_fileExtension = filename.substr(dotPos);
    } else {
        m_fileName = filename;
    }
}

// 标记文档为已修改
void Document::markModified(bool isModified) {
    m_modified = isModified;
    if (isModified) {
        m_saved = false;
    }
}

// 标记文档为已保存
void Document::markSaved(bool isSaved) {
    m_saved = isSaved;
    if (isSaved) {
        m_modified = false;
    }
}

// 添加行到命令行历史
void Document::addToCommandLineHistory(const std::string& content) {
    m_commandLineHistory.push_back(content);
}

// 添加命令到执行历史
void Document::addToCommandExecutionHistory(const std::string& content) {
    // 命令执行历史不重复，如果已经存在，那么先删除之后再添加到末尾
    auto it = std::find(m_commandExecutionHistory.begin(), m_commandExecutionHistory.end(), content);
    
    if (it != m_commandExecutionHistory.end()) {
        m_commandExecutionHistory.erase(it);
    }
    
    m_commandExecutionHistory.push_back(content);
}

// ============================================================================
// 数据库相关方法
// ============================================================================

bool Document::loadFromFile(const std::string& filePath) {
    if (filePath.empty()) {
        LOG_ERROR("File path is empty");
        return false;
    }
    
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open file: {}", filePath);
            return false;
        }
        
        std::string jsonStr((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // 解析 JSON
        rapidjson::Document doc;
        doc.Parse(jsonStr.c_str());
        
        if (doc.HasParseError()) {
            LOG_ERROR("Failed to parse JSON from file: {}", filePath);
            return false;
        }
        
        // 加载到数据库
        if (!m_database->loadFromJson(doc)) {
            LOG_ERROR("Failed to load database from JSON: {}", filePath);
            return false;
        }
        
        // 解析路径并标记为已保存
        parseFilePath(filePath);
        markSaved(true);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception loading file: {} - {}", filePath, e.what());
        return false;
    }
}

bool Document::loadFromFile() {
    if (m_fullPath.empty()) {
        LOG_ERROR("No file path set for document");
        return false;
    }
    return loadFromFile(m_fullPath);
}

bool Document::saveToFile(const std::string& filePath) {
    if (filePath.empty()) {
        LOG_ERROR("File path is empty");
        return false;
    }
    
    try {
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        
        m_database->saveToJson(writer);
        
        std::ofstream outFile(filePath);
        if (!outFile.is_open()) {
            LOG_ERROR("Failed to open file for writing: {}", filePath);
            return false;
        }
        
        outFile << buffer.GetString();
        outFile.close();
        
        // 解析路径并标记为已保存
        parseFilePath(filePath);
        markSaved(true);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception saving file: {} - {}", filePath, e.what());
        return false;
    }
}

bool Document::saveToFile() {
    if (m_fullPath.empty()) {
        LOG_ERROR("No file path set for document");
        return false;
    }
    return saveToFile(m_fullPath);
}

void Document::markDatabaseModified() {
    markModified(true);
}

} // namespace tch
