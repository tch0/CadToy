#pragma once

#include <string>

namespace tch {

struct File {
private:
    std::string m_fileName;      // 文件名（不含后缀）
    std::string m_fileExtension; // 文件后缀，暂时仅支持.cad.json
    std::string m_fullPath;       // 文件完整路径
    std::string m_content;       // 文件内容
    bool m_modified;             // 是否被修改
    bool m_saved;                // 是否已保存
    
public:
    // 构造函数
    File();
    File(const std::string& name, const std::string& path);
    
    // 获取文件名（不含后缀）
    const std::string& getFileName() const;
    
    // 获取文件后缀
    const std::string& getFileExtension() const;
    
    // 获取完整文件名（含后缀）
    std::string getFullFileName() const;
    
    // 获取文件完整路径
    const std::string& getFullPath() const;
    
    // 设置文件完整路径
    void setFullPath(const std::string& path);
    
    // 获取文件内容
    const std::string& getContent() const;
    
    // 设置文件内容
    void setContent(const std::string& content);
    
    // 检查文件是否被修改
    bool isModified() const;
    
    // 检查文件是否已保存
    bool isSaved() const;
    
    // 标记文件为已修改
    void markModified(bool isModified = true);
    
    // 标记文件为已保存
    void markSaved(bool isSaved = true);
};

} // namespace tch
