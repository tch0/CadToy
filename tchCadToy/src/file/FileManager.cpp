#include "file/FileManager.h"
#include "file/File.h"
#include "render/Renderer.h"
#include "imgui.h"
#include "utils/LocalizationManager.h"
#include "debug/Logger.h"
#include <fstream>
#include <algorithm>

namespace tch {

// 静态成员初始化
std::vector<File> FileManager::s_files;
std::size_t FileManager::s_currentFileIndex = 0;
std::size_t FileManager::s_fileCounter = 0;
std::vector<std::string> FileManager::s_recentFiles;

// 初始化文件管理器
void FileManager::initialize() {
    s_files.clear();
    s_recentFiles.clear();
    s_fileCounter = 0;
    
    // 创建一个默认的未命名文件
    s_currentFileIndex = createNewFile();
}

// 创建新文件，返回文件索引
std::size_t FileManager::createNewFile() {
    std::string fileName = "unnamed-" + std::to_string(s_fileCounter);
    s_fileCounter++;
    
    File newFile(fileName, "");
    s_files.push_back(newFile);
    
    return s_files.size() - 1;
}

// 打开文件，返回文件索引
std::size_t FileManager::openFile(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open file: {}", filePath);
            return -1;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        File newFile("", filePath);
        newFile.setContent(content);
        newFile.markSaved(true);
        
        s_files.push_back(newFile);
        s_currentFileIndex = s_files.size() - 1;
        
        // 添加到最近文件
        addToRecentFiles(filePath);
        
        return s_currentFileIndex;
    } catch (const std::exception& e) {
        LOG_ERROR("Error opening file: {}", e.what());
        return -1;
    }
}

// 保存文件
bool FileManager::saveFile(std::size_t index) {
    if (index >= s_files.size()) {
        return false;
    }
    
    File& file = s_files[index];
    if (file.getFullPath().empty()) {
        // 没有路径，需要另存为
        return false;
    }
    
    try {
        std::ofstream outFile(file.getFullPath());
        if (!outFile.is_open()) {
            LOG_ERROR("Failed to save file: {}", file.getFullPath());
            return false;
        }
        
        outFile << file.getContent();
        outFile.close();
        
        file.markSaved(true);
        
        // 添加到最近文件
        addToRecentFiles(file.getFullPath());
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error saving file: {}", e.what());
        return false;
    }
}

// 另存为
bool FileManager::saveFileAs(std::size_t index, const std::string& filePath) {
    if (index >= s_files.size()) {
        return false;
    }
    
    try {
        std::ofstream outFile(filePath);
        if (!outFile.is_open()) {
            LOG_ERROR("Failed to save file as: {}", filePath);
            return false;
        }
        
        outFile << s_files[index].getContent();
        outFile.close();
        
        // 更新文件信息
        s_files[index].setFullPath(filePath);
        s_files[index].markSaved(true);
        
        // 添加到最近文件
        addToRecentFiles(filePath);
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error saving file as: {}", e.what());
        return false;
    }
}

// 关闭文件
bool FileManager::closeFile(std::size_t index) {
    if (index >= s_files.size()) {
        return false;
    }
    
    if (s_files.size() == 1) {
        // 如果只剩最后一个文件，关闭后创建新文件
        s_files.clear();
        createNewFile();
    } else {
        s_files.erase(s_files.begin() + index);
        if (s_currentFileIndex >= index) {
            s_currentFileIndex = std::max(static_cast<std::size_t>(0), s_currentFileIndex - 1);
        }
    }
    
    return true;
}

// 设置当前文件索引
void FileManager::setCurrentFileIndex(std::size_t index) {
    if (index < s_files.size()) {
        s_currentFileIndex = index;
    }
}

// 获取当前文件索引
std::size_t FileManager::getCurrentFileIndex() {
    return s_currentFileIndex;
}

// 获取文件数量
std::size_t FileManager::getFileCount() {
    return s_files.size();
}

// 获取当前文件
const File& FileManager::getCurrentFile() {
    static File emptyFile;
    if (s_currentFileIndex < s_files.size()) {
        return s_files[s_currentFileIndex];
    }
    return emptyFile;
}

// 获取指定索引的文件
const File& FileManager::getFile(std::size_t index) {
    static File emptyFile;
    if (index < s_files.size()) {
        return s_files[index];
    }
    return emptyFile;
}

// 设置文件内容
void FileManager::setFileContent(std::size_t index, const std::string& content) {
    if (index < s_files.size()) {
        if (s_files[index].getContent() != content) {
            s_files[index].setContent(content);
        }
    }
}

// 标记文件为已修改
void FileManager::markFileModified(std::size_t index, bool modified) {
    if (index < s_files.size()) {
        s_files[index].markModified(modified);
    }
}

// 获取文件名
const std::string& FileManager::getFileName(std::size_t index) {
    static std::string emptyString;
    if (index < s_files.size()) {
        return s_files[index].getFileName();
    }
    return emptyString;
}

// 获取文件后缀
const std::string& FileManager::getFileExtension(std::size_t index) {
    static std::string emptyString;
    if (index < s_files.size()) {
        return s_files[index].getFileExtension();
    }
    return emptyString;
}

// 获取完整文件名（含后缀）
std::string FileManager::getFullFileName(std::size_t index) {
    static std::string emptyString;
    if (index < s_files.size()) {
        return s_files[index].getFullFileName();
    }
    return emptyString;
}

// 检查文件是否被修改
bool FileManager::isFileModified(std::size_t index) {
    if (index < s_files.size()) {
        return s_files[index].isModified();
    }
    return false;
}

// 检查文件是否已保存
bool FileManager::isFileSaved(std::size_t index) {
    if (index < s_files.size()) {
        return s_files[index].isSaved();
    }
    return false;
}

// 获取文件路径
const std::string& FileManager::getFilePath(std::size_t index) {
    static std::string emptyString;
    if (index < s_files.size()) {
        return s_files[index].getFullPath();
    }
    return emptyString;
}

// 获取最近打开的文件
const std::vector<std::string>& FileManager::getRecentFiles() {
    return s_recentFiles;
}

// 添加到最近文件
void FileManager::addToRecentFiles(const std::string& filePath) {
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

// 获取当前文件的命令历史
const std::vector<std::string>& FileManager::getCurrentFileCommandHistory() {
    static std::vector<std::string> emptyHistory;
    if (s_currentFileIndex < s_files.size()) {
        return s_files[s_currentFileIndex].getCommandHistory();
    }
    return emptyHistory;
}

// 向当前文件添加命令历史
void FileManager::addToCurrentFileCommandHistory(const std::string& command) {
    if (s_currentFileIndex < s_files.size()) {
        s_files[s_currentFileIndex].addToCommandHistory(command);
    }
}

// 清除当前文件的命令历史
void FileManager::clearCurrentFileCommandHistory() {
    if (s_currentFileIndex < s_files.size()) {
        s_files[s_currentFileIndex].clearCommandHistory();
    }
}

} // namespace tch
