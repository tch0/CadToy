// 对应头文件
#include "document/Document.h"

// C++ 标准库
#include <filesystem>

// 第三方库

// 项目头文件

namespace tch {

// 构造函数
Document::Document() :
    m_fileExtension(".cad.json"),
    m_modified(false),
    m_saved(false),
    m_showGrid(true),
    m_showAxes(true) {
}

Document::Document(const std::string& name, const std::string& path) :
    m_fileExtension(".cad.json"),
    m_modified(false),
    m_saved(false),
    m_showGrid(true),
    m_showAxes(true) {
    // 解析文件名和路径
    std::filesystem::path filePath(path);
    if (!path.empty()) {
        m_fullPath = path;
        // 从路径中提取文件名
        std::string filename = filePath.filename().string();
        // 分离文件名和后缀
        size_t dotPos = filename.rfind('.');
        if (dotPos != std::string::npos) {
            m_fileName = filename.substr(0, dotPos);
            m_fileExtension = filename.substr(dotPos);
        }
        else {
            m_fileName = filename;
        }
    }
    else {
        // 如果没有路径，直接使用传入的name
        m_fileName = name;
    }
}

// 获取文件名（不含后缀）
const std::string& Document::getFileName() const {
    return m_fileName;
}

// 获取文件后缀
const std::string& Document::getFileExtension() const {
    return m_fileExtension;
}

// 获取完整文件名（含后缀）
std::string Document::getFullFileName() const {
    return m_fileName + m_fileExtension;
}

// 获取文件完整路径
const std::string& Document::getFullPath() const {
    return m_fullPath;
}

// 设置文件完整路径
void Document::setFullPath(const std::string& path) {
    m_fullPath = path;
    // 从路径中提取文件名和后缀
    std::filesystem::path filePath(path);
    std::string filename = filePath.filename().string();
    size_t dotPos = filename.rfind('.');
    if (dotPos != std::string::npos) {
        m_fileName = filename.substr(0, dotPos);
        m_fileExtension = filename.substr(dotPos);
    }
    else {
        m_fileName = filename;
    }
}

// 获取文件内容
const std::string& Document::getContent() const {
    return m_content;
}

// 设置文件内容
void Document::setContent(const std::string& content) {
    this->m_content = content;
    m_modified = true;
}

// 检查文件是否被修改
bool Document::isModified() const {
    return m_modified;
}

// 检查文件是否已保存
bool Document::isSaved() const {
    return m_saved;
}

// 标记文件为已修改
void Document::markModified(bool isModified) {
    m_modified = isModified;
    if (isModified) {
        m_saved = false;
    }
}

// 标记文件为已保存
void Document::markSaved(bool isSaved) {
    m_saved = isSaved;
    if (isSaved) {
        m_modified = false;
    }
}

// 获取命令历史
const std::vector<std::string>& Document::getCommandHistory() const {
    return m_commandHistory;
}

// 添加命令到历史
void Document::addToCommandHistory(const std::string& command) {
    m_commandHistory.push_back(command);
}

// 清除命令历史
void Document::clearCommandHistory() {
    m_commandHistory.clear();
}

// 获取变换管理器
TransformManager& Document::getTransformManager() {
    return m_transformManager;
}

// 获取变换管理器（const版本）
const TransformManager& Document::getTransformManager() const {
    return m_transformManager;
}

// 检查是否显示栅格
bool Document::isShowGrid() const {
    return m_showGrid;
}

// 设置是否显示栅格
void Document::setShowGrid(bool show) {
    m_showGrid = show;
}

// 检查是否显示坐标轴
bool Document::isShowAxes() const {
    return m_showAxes;
}

// 设置是否显示坐标轴
void Document::setShowAxes(bool show) {
    m_showAxes = show;
}

} // namespace tch
