#include "document/DocManager.h"
#include "document/Document.h"
#include "render/Renderer.h"
#include "imgui.h"
#include "utils/LocalizationManager.h"
#include "debug/Logger.h"
#include <fstream>
#include <algorithm>

namespace tch {

// 静态成员初始化
std::vector<Document> DocManager::s_documents;
std::size_t DocManager::s_currentDocIndex = 0;
std::size_t DocManager::s_docCounter = 0;
std::vector<std::string> DocManager::s_recentFiles;

// 初始化文档管理器
void DocManager::initialize() {
    s_documents.clear();
    s_recentFiles.clear();
    s_docCounter = 0;
    
    // 创建一个默认的未命名文档
    s_currentDocIndex = createNewDocument();
}

// 创建新文档，返回文档索引
std::size_t DocManager::createNewDocument() {
    std::string documentName = "unnamed-" + std::to_string(s_docCounter);
    s_docCounter++;
    
    Document newDocument(documentName, "");
    s_documents.push_back(newDocument);
    
    return s_documents.size() - 1;
}

// 打开文件，返回文档索引
std::size_t DocManager::openFile(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open file: {}", filePath);
            return -1;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        Document newDocument("", filePath);
        newDocument.setContent(content);
        newDocument.markSaved(true);
        
        s_documents.push_back(newDocument);
        s_currentDocIndex = s_documents.size() - 1;
        
        // 添加到最近文件
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
    
    try {
        std::ofstream outFile(document.getFullPath());
        if (!outFile.is_open()) {
            LOG_ERROR("Failed to save document: {}", document.getFullPath());
            return false;
        }
        
        outFile << document.getContent();
        outFile.close();
        
        document.markSaved(true);
        
        // 添加到最近文件
        addToRecentFiles(document.getFullPath());
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error saving document: {}", e.what());
        return false;
    }
}

// 另存为
bool DocManager::saveFileAs(std::size_t index, const std::string& filePath) {
    if (index >= s_documents.size()) {
        return false;
    }
    
    try {
        std::ofstream outFile(filePath);
        if (!outFile.is_open()) {
            LOG_ERROR("Failed to save document as: {}", filePath);
            return false;
        }
        
        outFile << s_documents[index].getContent();
        outFile.close();
        
        // 更新文档信息
        s_documents[index].setFullPath(filePath);
        s_documents[index].markSaved(true);
        
        // 添加到最近文件
        addToRecentFiles(filePath);
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error saving document as: {}", e.what());
        return false;
    }
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
    } else {
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
const Document& DocManager::getCurrentDocument() {
    static Document emptyDocument;
    if (s_currentDocIndex < s_documents.size()) {
        return s_documents[s_currentDocIndex];
    }
    return emptyDocument;
}

// 获取指定索引的文档
const Document& DocManager::getDocument(std::size_t index) {
    static Document emptyDocument;
    if (index < s_documents.size()) {
        return s_documents[index];
    }
    return emptyDocument;
}

// 设置文档内容
void DocManager::setDocumentContent(std::size_t index, const std::string& content) {
    if (index < s_documents.size()) {
        if (s_documents[index].getContent() != content) {
            s_documents[index].setContent(content);
        }
    }
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
const std::vector<std::string>& DocManager::getCurrentDocumentCommandHistory() {
    static std::vector<std::string> emptyHistory;
    if (s_currentDocIndex < s_documents.size()) {
        return s_documents[s_currentDocIndex].getCommandHistory();
    }
    return emptyHistory;
}

// 向当前文档添加命令历史
void DocManager::addToCurrentDocumentCommandHistory(const std::string& command) {
    if (s_currentDocIndex < s_documents.size()) {
        s_documents[s_currentDocIndex].addToCommandHistory(command);
    }
}

// 清除当前文档的命令历史
void DocManager::clearCurrentDocumentCommandHistory() {
    if (s_currentDocIndex < s_documents.size()) {
        s_documents[s_currentDocIndex].clearCommandHistory();
    }
}

} // namespace tch
