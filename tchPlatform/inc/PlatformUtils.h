#pragma once

// C++ 标准库
#include <string>
#include <filesystem>

namespace tch {
namespace PlatformUtils {

/**
 * @brief 将本地编码字符串转换为 UTF-8
 * @param localStr 本地编码字符串（Windows: GBK/GB2312, Unix: UTF-8）
 * @return UTF-8 编码字符串
 */
std::string localToUtf8(const std::string& localStr);

/**
 * @brief 将 UTF-8 字符串转换为本地编码
 * @param utf8Str UTF-8 编码字符串
 * @return 本地编码字符串（Windows: GBK/GB2312, Unix: UTF-8）
 */
std::string utf8ToLocal(const std::string& utf8Str);

/**
 * @brief 跨平台路径类
 * 
 * 程序内部统一使用 UTF-8 编码，自动处理 Windows/Unix 平台差异。
 * 所有字符串交互（构造、获取、显示）都应该使用 UTF-8，
 * 只有在与系统 API（如 fstream）交互时才转换为本地编码。
 */
class Path {
private:
    std::string m_utf8Path;  // 内部统一使用 UTF-8

public:
    // 默认构造
    Path() = default;
    
    // 从 UTF-8 字符串构造
    explicit Path(const std::string& utf8Path) : m_utf8Path(utf8Path) {}
    explicit Path(const char* utf8Path) : m_utf8Path(utf8Path) {}
    
    // 从本地编码构造（用于从系统获取的路径）
    static Path fromLocal(const std::string& localPath) {
        return Path(localToUtf8(localPath));
    }
    
    // 获取 UTF-8 路径（默认，用于显示、ImGui、JSON）
    const std::string& string() const { return m_utf8Path; }
    const std::string& utf8() const { return m_utf8Path; }
    
    // 获取本地编码路径（仅用于系统 API，如 fstream）
    std::string local() const { return utf8ToLocal(m_utf8Path); }
    
    // 获取 std::filesystem::path（本地编码）
    std::filesystem::path fsPath() const { return std::filesystem::path(local()); }
    
    // 获取父目录
    // 注意：与 std::filesystem::path::parent_path() 行为一致
    // 如果没有父目录（如根目录），返回自身
    Path parent() const {
        auto p = fsPath();
        return fromLocal(p.parent_path().string());
    }
    
    // 获取文件名（UTF-8）
    std::string filename() const {
        return localToUtf8(fsPath().filename().string());
    }
    
    // 组合路径（传入 UTF-8 文件名）
    Path operator/(const std::string& utf8Filename) const {
        auto p = fsPath();
        p /= utf8ToLocal(utf8Filename);
        return fromLocal(p.string());
    }
    
    // 检查路径是否为空
    bool empty() const { return m_utf8Path.empty(); }
    
    // 比较操作
    bool operator==(const Path& other) const { return m_utf8Path == other.m_utf8Path; }
    bool operator!=(const Path& other) const { return m_utf8Path != other.m_utf8Path; }
    
    // 检查文件/目录是否存在
    bool exists() const {
        return std::filesystem::exists(fsPath());
    }
    
    // 检查是否是目录
    bool isDirectory() const {
        return std::filesystem::is_directory(fsPath());
    }
    
    // 检查是否是常规文件
    bool isRegularFile() const {
        return std::filesystem::is_regular_file(fsPath());
    }
    
    // 遍历目录（回调接收 Path 对象）
    template<typename Func>
    void iterate(Func&& callback) const {
        for (const auto& entry : std::filesystem::directory_iterator(local())) {
            callback(Path::fromLocal(entry.path().string()));
        }
    }
};

} // namespace PlatformUtils
} // namespace tch
