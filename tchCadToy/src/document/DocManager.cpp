// 对应头文件
#include "DocManager.h"

// C++ 标准库
#include <algorithm>

// 第三方库

// 项目头文件
#include "Document.h"
#include "Logger.h"


namespace tch {

// 静态成员初始化
std::vector<Document> DocManager::s_documents;
std::size_t DocManager::s_currentDocIndex = 0;
std::size_t DocManager::s_docCounter = 0;
std::vector<std::string> DocManager::s_recentFiles;
const std::size_t DocManager::InvalidDocIndex = static_cast<std::size_t>(-1);

// 初始化文档管理器
void DocManager::initialize() {
    s_documents.clear();
    s_recentFiles.clear();
    s_docCounter = 0;
    
    // 创建一个默认的未命名文档
    s_currentDocIndex = createNewDocument();
}

// 生成默认文件名（不含后缀，如 "unnamed-0"）
std::string DocManager::generateDefaultFileName() {
    return "unnamed-" + std::to_string(s_docCounter++);
}

// 创建新文档，返回文档索引
std::size_t DocManager::createNewDocument() {
    Document newDocument(generateDefaultFileName());
    // Database 已在 Document 构造函数中初始化，包含默认 "0" 图层
    
    s_documents.push_back(std::move(newDocument));
    return s_documents.size() - 1;
}

// 打开文件，返回文档索引
std::size_t DocManager::openFile(const std::string& filePath) {
    try {
        // 创建文档（带 Database），文件名临时为 "unnamed"，后续加载后会覆盖
        Document newDocument("unnamed");
        
        // 尝试加载文件
        if (!newDocument.loadFromFile(filePath)) {
            LOG_ERROR("Failed to load file: {}", filePath);
            return -1;  // 加载失败，返回无效索引
        }
        
        // 加载成功
        s_documents.push_back(std::move(newDocument));
        s_currentDocIndex = s_documents.size() - 1;
        addToRecentFiles(filePath);
        
        return s_currentDocIndex;
    } catch (const std::exception& e) {
        LOG_ERROR("Error opening file: {}", e.what());
        return -1;
    }
}

// 保存文档
bool DocManager::saveFile(std::size_t index) {
    if (index >= s_documents.size()) {
        return false;
    }
    
    Document& document = s_documents[index];
    if (document.getFullPath().empty()) {
        // 没有路径，需要另存为
        return false;
    }
    
    if (!document.saveToFile()) {
        LOG_ERROR("Failed to save document: {}", document.getFullPath());
        return false;
    }
    
    // 添加到最近文件
    addToRecentFiles(document.getFullPath());
    return true;
}

// 另存为
bool DocManager::saveFileAs(std::size_t index, const std::string& filePath) {
    if (index >= s_documents.size()) {
        return false;
    }
    
    if (!s_documents[index].saveToFile(filePath)) {
        LOG_ERROR("Failed to save document as: {}", filePath);
        return false;
    }
    
    // 添加到最近文件
    addToRecentFiles(filePath);
    return true;
}

// 关闭文档
bool DocManager::closeDocument(std::size_t index) {
    if (index >= s_documents.size()) {
        return false;
    }
    
    if (s_documents.size() == 1) {
        // 如果只剩最后一个文档，关闭后创建新文档
        s_documents.clear();
        createNewDocument();
    }
    else {
        s_documents.erase(s_documents.begin() + index);
        if (s_currentDocIndex >= index) {
            s_currentDocIndex = std::max(static_cast<std::size_t>(0), s_currentDocIndex - 1);
        }
    }
    
    return true;
}

// 设置当前文档索引
void DocManager::setCurrentDocumentIndex(std::size_t index) {
    if (index < s_documents.size()) {
        s_currentDocIndex = index;
    }
}

// 获取当前文档索引
std::size_t DocManager::getCurrentDocumentIndex() {
    return s_currentDocIndex;
}

// 获取文档数量
std::size_t DocManager::getDocumentCount() {
    return s_documents.size();
}

// 获取当前文档
Document& DocManager::getCurrentDocument() {
    static Document emptyDocument;
    if (s_currentDocIndex < s_documents.size()) {
        return s_documents[s_currentDocIndex];
    }
    return emptyDocument;
}

// 获取指定索引的文档
Document& DocManager::getDocument(std::size_t index) {
    static Document emptyDocument;
    if (index < s_documents.size()) {
        return s_documents[index];
    }
    return emptyDocument;
}

// 标记文档为已修改
void DocManager::markDocumentModified(std::size_t index, bool modified) {
    if (index < s_documents.size()) {
        s_documents[index].markModified(modified);
    }
}

// 获取文件名
const std::string& DocManager::getFileName(std::size_t index) {
    static std::string emptyString;
    if (index < s_documents.size()) {
        return s_documents[index].getFileName();
    }
    return emptyString;
}

// 获取文件后缀
const std::string& DocManager::getFileExtension(std::size_t index) {
    static std::string emptyString;
    if (index < s_documents.size()) {
        return s_documents[index].getFileExtension();
    }
    return emptyString;
}

// 获取完整文件名（含后缀）
std::string DocManager::getFullFileName(std::size_t index) {
    static std::string emptyString;
    if (index < s_documents.size()) {
        return s_documents[index].getFullFileName();
    }
    return emptyString;
}

// 检查文档是否被修改
bool DocManager::isDocumentModified(std::size_t index) {
    if (index < s_documents.size()) {
        return s_documents[index].isModified();
    }
    return false;
}

// 检查文档是否已保存
bool DocManager::isDocumentSaved(std::size_t index) {
    if (index < s_documents.size()) {
        return s_documents[index].isSaved();
    }
    return false;
}

// 获取文件路径
const std::string& DocManager::getFilePath(std::size_t index) {
    static std::string emptyString;
    if (index < s_documents.size()) {
        return s_documents[index].getFullPath();
    }
    return emptyString;
}

// 获取最近打开的文件
const std::vector<std::string>& DocManager::getRecentFiles() {
    return s_recentFiles;
}

// 添加到最近文件
void DocManager::addToRecentFiles(const std::string& filePath) {
    // 移除已存在的路径
    auto it = std::find(s_recentFiles.begin(), s_recentFiles.end(), filePath);
    if (it != s_recentFiles.end()) {
        s_recentFiles.erase(it);
    }
    
    // 添加到开头
    s_recentFiles.insert(s_recentFiles.begin(), filePath);
    
    // 限制最近文件数量为10
    if (s_recentFiles.size() > 10) {
        s_recentFiles.resize(10);
    }
}

// 获取当前文档的命令历史
const std::vector<std::string>& DocManager::getCurrentDocumentCommandLineHistory() {
    static std::vector<std::string> emptyHistory;
    if (s_currentDocIndex < s_documents.size()) {
        return s_documents[s_currentDocIndex].getCommandLineHistory();
    }
    return emptyHistory;
}

// 向当前文档添加命令历史
void DocManager::addToCurrentDocumentCommandLineHistory(const std::string& content) {
    if (s_currentDocIndex < s_documents.size()) {
        s_documents[s_currentDocIndex].addToCommandLineHistory(content);
    }
}

// 清除当前文档的命令历史
void DocManager::clearCurrentDocumentCommandLineHistory() {
    if (s_currentDocIndex < s_documents.size()) {
        s_documents[s_currentDocIndex].clearCommandLineHistory();
    }
}

} // namespace tch
