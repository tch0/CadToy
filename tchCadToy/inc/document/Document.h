#pragma once

// C++ 标准库
#include <string>
#include <vector>

// 第三方库

// 项目头文件
#include "transform/TransformManager.h"

namespace tch {

// CAD文档，关联到系统中的文件（如果保存了的话）、保存视口、命令历史等一系列文档相关的信息
class Document {
private:
    std::string m_fileName;         // 文件名（不含后缀）
    std::string m_fileExtension;    // 文件后缀，暂时仅支持.cad.json
    std::string m_fullPath;         // 文档关联文件的完整路径
    std::string m_content;          // 文档内容
    bool m_modified;                // 是否已修改
    bool m_saved;                   // 是否已保存
    std::vector<std::string> m_commandLineHistory;  // 命令行输出历史
    TransformManager m_transformManager;            // 文档专属变换管理器
    bool m_showGrid;                                // 是否显示栅格
    bool m_showAxes;                                // 是否显示坐标轴
    
public:
    // 构造函数
    Document();
    Document(const std::string& name, const std::string& path);
    
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
    
    // 获取文档内容
    const std::string& getContent() const;
    
    // 设置文档内容
    void setContent(const std::string& content);
    
    // 检查文档是否被修改
    bool isModified() const;
    
    // 检查文档是否已保存
    bool isSaved() const;
    
    // 标记文档为已修改
    void markModified(bool isModified = true);
    
    // 标记文档为已保存
    void markSaved(bool isSaved = true);
    
    // 命令行历史输出相关方法
    const std::vector<std::string>& getCommandLineHistory() const;
    void addToCommandLineHistory(const std::string& content);
    void clearCommandLineHistory();
    
    // 变换管理器相关方法
    TransformManager& getTransformManager();
    const TransformManager& getTransformManager() const;
    
    // 栅格和坐标轴相关方法
    bool isShowGrid() const;
    void setShowGrid(bool show);
    bool isShowAxes() const;
    void setShowAxes(bool show);
};

} // namespace tch
