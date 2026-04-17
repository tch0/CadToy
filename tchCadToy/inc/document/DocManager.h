#pragma once

// C++ 标准库
#include <filesystem>
#include <string>
#include <vector>

// 第三方库

// 项目头文件
#include "Document.h"


namespace tch {

class DocManager {
private:
    static std::vector<Document> s_documents;       // 文档列表
    static std::size_t s_currentDocIndex;           // 当前文档索引
    static std::size_t s_docCounter;                // 文档计数器（用于生成默认文件名）
    static std::vector<std::filesystem::path> s_recentFiles;  // 最近打开的文件列表
    static std::size_t s_pendingIndexToBeSwitched;  // 来自命令的待切换文档索引
    // 私有构造函数，防止外部实例化
    DocManager() {}
    
public:
    // 无效索引，-1，用于初始化和判断
    static const std::size_t InvalidDocIndex;
    
    // 初始化文档管理器
    static void initialize();
    
    // 生成默认文件名（不含后缀，如 "unnamed-0"）
    static std::string generateDefaultFileName();
    
    // 文档操作
    static std::size_t createNewDocument();                                 // 创建新文档，返回文档索引
    static std::size_t openFile(const std::filesystem::path& filePath);     // 打开文件，返回文档索引
    static bool saveFile(std::size_t index);                                // 保存文档
    static bool saveFileAs(std::size_t index, const std::filesystem::path& filePath); // 另存为
    static bool closeDocument(std::size_t index);                           // 关闭文档
    
    // 文档管理
    static void setCurrentDocumentIndex(std::size_t index);         // 设置当前文档索引
    static std::size_t getCurrentDocumentIndex();                   // 获取当前文档索引
    static std::size_t getDocumentCount();                          // 获取文档数量
    static Document& getCurrentDocument();                          // 获取当前文档
    static Document& getDocument(std::size_t index);                // 获取指定索引的文档
    
    // 来自命令层的待切换文档索引，open打开已打开的文件需要切换到该文档，因为ImGui对此不知情，所以需要记录下之后在这一帧特殊处理，
    // 下一帧时已经切换命令也已经结束(文档切换后命令即应该结束)，可以放心获取文档，其余命令如果要切换文档也应该通过这两个接口实现
    static void setPendingSwitchIndexFromCommand(std::size_t index);
    static std::size_t getPendingSwitchIndexFromCommand();
    
    // 文档内容操作
    static void markDocumentModified(std::size_t index, bool modified = true);     // 标记文档为已修改
    
    // 文档标签操作
    static const std::string& getFileName(std::size_t index);           // 获取文件名（不含后缀）
    static const std::string& getFileExtension(std::size_t index);      // 获取文件后缀
    static std::string getFullFileName(std::size_t index);              // 获取完整文件名（含后缀）
    static bool isDocumentModified(std::size_t index);                  // 检查文档是否被修改
    static bool isDocumentSaved(std::size_t index);                     // 检查文档是否已保存
    static const std::filesystem::path& getFilePath(std::size_t index); // 获取文件路径
    
    // 最近文件
    static const std::vector<std::filesystem::path>& getRecentFiles();      // 获取最近打开的文件
    static void addToRecentFiles(const std::filesystem::path& filePath);    // 添加到最近文件
    
    // 命令历史相关方法
    static const std::vector<std::string>& getCurrentDocumentCommandLineHistory();  // 获取当前文档的命令历史
    static void addToCurrentDocumentCommandLineHistory(const std::string& content); // 向当前文档添加命令历史
    static void clearCurrentDocumentCommandLineHistory();                           // 清除当前文档的命令历史
};

} // namespace tch
