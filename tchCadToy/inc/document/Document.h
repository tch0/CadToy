#pragma once

// C++ 标准库
#include <string>
#include <vector>
#include <memory>

// 第三方库

// 项目头文件
#include "TransformManager.h"
#include "Database.h"


namespace tch {

// CAD文档，关联到系统中的文件（如果保存了的话）、保存视口、命令历史等一系列文档相关的信息
class Document {
private:
    std::string m_fileName;         // 文件名（不含后缀）
    std::string m_fileExtension;    // 文件后缀，暂时仅支持.cad.json
    std::string m_fullPath;         // 文档关联文件的完整路径
    bool m_modified;                // 是否已修改
    bool m_saved;                   // 是否已保存，即关联到文件系统中的某个文件
    std::vector<std::string> m_commandLineHistory;          // 命令行输出历史
    std::vector<std::string> m_commandExecutionHistory;     // 命令执行历史
    TransformManager m_transformManager;                    // 文档专属变换管理器
    bool m_showGrid;                                        // 是否显示栅格
    bool m_showAxes;                                        // 是否显示坐标轴
    
    // 数据库（每个文档拥有自己的 CAD 数据库）
    std::unique_ptr<Database> m_database;

public:
    // 默认构造函数：创建空文档，不构造 Database
    Document();
    
    // 构造函数：新建文档，使用指定文件名
    explicit Document(const std::string& fileName);
    
    // 禁用拷贝（因为包含 unique_ptr<Database>）
    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;
    
    // 允许移动
    Document(Document&&) = default;
    Document& operator=(Document&&) = default;
    
    // 获取文件名（不含后缀）
    const std::string& getFileName() const { return m_fileName; }
    
    // 获取文件后缀
    const std::string& getFileExtension() const { return m_fileExtension; }
    
    // 获取完整文件名（含后缀）
    std::string getFullFileName() const { return m_fileName + m_fileExtension; }
    
    // 获取文件完整路径
    const std::string& getFullPath() const { return m_fullPath; }
    
    // 检查文档是否被修改
    bool isModified() const { return m_modified; }
    
    // 检查文档是否已保存
    bool isSaved() const { return m_saved; }
    
    // 标记文档为已修改
    void markModified(bool isModified = true);
    
    // 标记文档为已保存
    void markSaved(bool isSaved = true);
    
    // 命令行历史输出相关方法
    const std::vector<std::string>& getCommandLineHistory() const { return m_commandLineHistory; }
    void addToCommandLineHistory(const std::string& content);
    void clearCommandLineHistory() { m_commandLineHistory.clear(); }
    
    // 命令执行历史相关方法
    const std::vector<std::string>& getCommandExecutionHistory() const { return m_commandExecutionHistory; }
    void addToCommandExecutionHistory(const std::string& content);
    void clearCommandExecutionHistory() { m_commandExecutionHistory.clear(); }
    
    // 变换管理器相关方法
    TransformManager& getTransformManager() { return m_transformManager; }
    const TransformManager& getTransformManager() const { return m_transformManager; }
    
    // 栅格和坐标轴相关方法
    bool isShowGrid() const { return m_showGrid; }
    void setShowGrid(bool show) { m_showGrid = show; }
    bool isShowAxes() const { return m_showAxes; }
    void setShowAxes(bool show) { m_showAxes = show; }
    
    // =======================================================================
    // 数据库相关方法
    // =======================================================================
    
    // 获取数据库
    Database* getDatabase() const { return m_database.get(); }
    
    // 从文件加载（使用 Database 反序列化）
    bool loadFromFile(const std::string& filePath);
    bool loadFromFile();
    
    // 保存到文件（使用 Database 序列化）
    bool saveToFile(const std::string& filePath);
    bool saveToFile();
    
    // 标记数据库已修改
    void markDatabaseModified();

private:
    // 从路径解析文件名和后缀
    void parseFilePath(const std::string& path);
};

} // namespace tch
