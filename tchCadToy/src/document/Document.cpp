// 对应头文件
#include "Document.h"

// C++ 标准库
#include <algorithm>
#include <filesystem>

// 第三方库
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

// 项目头文件
#include "Logger.h"
#include "GlobalUtils.h"


namespace tch {

// 默认构造函数：创建空文档，不构造 Database
Document::Document() :
    m_fileName("unnamed-empty"),
    m_fileExtension(".cad.json"),
    m_modified(false),
    m_saved(false),
    m_showGrid(true),
    m_showAxes(true),
    m_lastPoint(0, 0, 0) {
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
    m_lastPoint(0, 0, 0),
    m_database(std::make_unique<Database>()),
    m_graphicsDataCache(std::make_unique<GraphicsDataCache>()) {
    // 关联图形数据缓存到数据库（双向关联）
    m_graphicsDataCache->setDatabase(m_database.get());
    m_database->setGraphicsDataCache(m_graphicsDataCache.get());
    // 关联数据库到 Undo 栈
    m_undoStack.setDatabase(m_database.get());
}

// 从路径解析文件名和后缀
void Document::parseFilePath(const std::filesystem::path& path) {
    m_filePath = path;
    
    std::string filename = PathUtils::toString(path.filename());
    
    // 特殊处理 .cad.json 后缀
    const std::string CAD_JSON_EXT = ".cad.json";
    if (filename.size() >= CAD_JSON_EXT.size() &&
        filename.substr(filename.size() - CAD_JSON_EXT.size()) == CAD_JSON_EXT) {
        // 文件名是 .cad.json 格式
        m_fileName = filename.substr(0, filename.size() - CAD_JSON_EXT.size());
        m_fileExtension = CAD_JSON_EXT;
    } else {
        // 默认逻辑：查找最后一个点号
        size_t dotPos = filename.rfind('.');
        if (dotPos != std::string::npos) {
            m_fileName = filename.substr(0, dotPos);
            m_fileExtension = filename.substr(dotPos);
        } else {
            m_fileName = filename;
            m_fileExtension = "";
        }
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

bool Document::loadFromFile(const std::filesystem::path& filePath) {
    if (filePath.empty()) {
        LOG_ERROR("File path is empty");
        return false;
    }
    
    try {
        // 使用封装接口读取文件
        std::string jsonStr;
        if (!Utils::readTextFile(filePath, jsonStr)) {
            LOG_ERROR("Failed to read file: {}", PathUtils::toString(filePath));
            return false;
        }
        
        // 解析 JSON
        rapidjson::Document doc;
        doc.Parse(jsonStr.c_str());
        
        if (doc.HasParseError()) {
            LOG_ERROR("Failed to parse JSON from file: {}", PathUtils::toString(filePath));
            return false;
        }
        
        // 加载到数据库
        if (!m_database->loadFromJson(doc)) {
            LOG_ERROR("Failed to load database from JSON: {}", PathUtils::toString(filePath));
            return false;
        }
        
        // 解析路径并标记为已保存
        parseFilePath(filePath);
        markSaved(true);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception loading file: {} - {}", PathUtils::toString(filePath), e.what());
        return false;
    }
}

bool Document::loadFromFile() {
    if (m_filePath.empty()) {
        LOG_ERROR("No file path set for document");
        return false;
    }
    return loadFromFile(m_filePath);
}

bool Document::saveToFile(const std::filesystem::path& filePath) {
    if (filePath.empty()) {
        LOG_ERROR("File path is empty");
        return false;
    }
    
    try {
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        
        m_database->saveToJson(writer);
        
        // 使用封装接口写入文件
        if (!Utils::writeTextFile(filePath, buffer.GetString())) {
            LOG_ERROR("Failed to write file: {}", PathUtils::toString(filePath));
            return false;
        }
        
        // 解析路径并标记为已保存
        parseFilePath(filePath);
        markSaved(true);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception saving file: {} - {}", PathUtils::toString(filePath), e.what());
        return false;
    }
}

bool Document::saveToFile() {
    if (m_filePath.empty()) {
        LOG_ERROR("No file path set for document");
        return false;
    }
    return saveToFile(m_filePath);
}

} // namespace tch
