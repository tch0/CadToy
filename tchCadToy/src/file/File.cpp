#include "file/File.h"
#include <filesystem>

namespace tch {

// 构造函数
File::File() : m_fileExtension(".cad.json"), m_modified(false), m_saved(false) {
}

File::File(const std::string& name, const std::string& path) 
    : m_fileExtension(".cad.json"), m_modified(false), m_saved(false) {
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
        } else {
            m_fileName = filename;
        }
    } else {
        // 如果没有路径，直接使用传入的name
        m_fileName = name;
    }
}

// 获取文件名（不含后缀）
const std::string& File::getFileName() const {
    return m_fileName;
}

// 获取文件后缀
const std::string& File::getFileExtension() const {
    return m_fileExtension;
}

// 获取完整文件名（含后缀）
std::string File::getFullFileName() const {
    return m_fileName + m_fileExtension;
}

// 获取文件完整路径
const std::string& File::getFullPath() const {
    return m_fullPath;
}

// 设置文件完整路径
void File::setFullPath(const std::string& path) {
    m_fullPath = path;
    // 从路径中提取文件名和后缀
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

// 获取文件内容
const std::string& File::getContent() const {
    return m_content;
}

// 设置文件内容
void File::setContent(const std::string& content) {
    this->m_content = content;
    m_modified = true;
}

// 检查文件是否被修改
bool File::isModified() const {
    return m_modified;
}

// 检查文件是否已保存
bool File::isSaved() const {
    return m_saved;
}

// 标记文件为已修改
void File::markModified(bool isModified) {
    m_modified = isModified;
    if (isModified) {
        m_saved = false;
    }
}

// 标记文件为已保存
void File::markSaved(bool isSaved) {
    m_saved = isSaved;
    if (isSaved) {
        m_modified = false;
    }
}

} // namespace tch
