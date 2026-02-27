#pragma once

#include "file/File.h"
#include <string>
#include <vector>
#include <filesystem>

namespace tch {

class FileManager {
private:
    static std::vector<File> s_files;         // 文件列表
    static std::size_t s_currentFileIndex;    // 当前文件索引
    static std::size_t s_fileCounter;         // 文件计数器
    static std::vector<std::string> s_recentFiles; // 最近打开的文件
    
    // 私有构造函数，防止实例化
    FileManager() {}
    
public:
    // 初始化文件管理器
    static void initialize();
    
    // 文件操作
    static std::size_t createNewFile();                          // 创建新文件，返回文件索引
    static std::size_t openFile(const std::string& filePath);    // 打开文件，返回文件索引
    static bool saveFile(std::size_t index);                     // 保存文件
    static bool saveFileAs(std::size_t index, const std::string& filePath); // 另存为
    static bool closeFile(std::size_t index);                    // 关闭文件
    
    // 文件管理
    static void setCurrentFileIndex(std::size_t index);          // 设置当前文件索引
    static std::size_t getCurrentFileIndex();                   // 获取当前文件索引
    static std::size_t getFileCount();                          // 获取文件数量
    static const File& getCurrentFile();                // 获取当前文件
    static const File& getFile(std::size_t index);              // 获取指定索引的文件
    
    // 文件内容操作
    static void setFileContent(std::size_t index, const std::string& content); // 设置文件内容
    static void markFileModified(std::size_t index, bool modified = true);     // 标记文件为已修改
    
    // 文件标签操作
    static const std::string& getFileName(std::size_t index); // 获取文件名（不含后缀）
    static const std::string& getFileExtension(std::size_t index); // 获取文件后缀
    static std::string getFullFileName(std::size_t index); // 获取完整文件名（含后缀）
    static bool isFileModified(std::size_t index);      // 检查文件是否被修改
    static bool isFileSaved(std::size_t index);        // 检查文件是否已保存
    static const std::string& getFilePath(std::size_t index); // 获取文件路径
    
    // 最近文件
    static const std::vector<std::string>& getRecentFiles(); // 获取最近打开的文件
    static void addToRecentFiles(const std::string& filePath); // 添加到最近文件
    
    // 命令历史相关方法
    static const std::vector<std::string>& getCurrentFileCommandHistory(); // 获取当前文件的命令历史
    static void addToCurrentFileCommandHistory(const std::string& command); // 向当前文件添加命令历史
    static void clearCurrentFileCommandHistory(); // 清除当前文件的命令历史
};

} // namespace tch
