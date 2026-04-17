#pragma once

// C++ 标准库
#include <string>
#include <filesystem>

// 第三方库

// 项目头文件
#include "Database.h"


namespace tch {

// =========================================================================================================================
// 路径工具命名空间
// =========================================================================================================================
namespace PathUtils {

// std::filesystem::path和UTF-8编码的std::string字符串之间的转换函数：
// 所有字符串与路径互相转换都应该使用下面的函数
// 项目中不应该再使用任何std::filesystem::path(std::string)和std::filesystem::path::string

// UTF-8 string -> filesystem::path
inline std::filesystem::path toPath(const std::string& utf8String) {
    return std::filesystem::path(reinterpret_cast<const char8_t*>(utf8String.data()),
                                 reinterpret_cast<const char8_t*>(utf8String.data()) + utf8String.size());
}

// filesystem::path -> UTF-8 string
inline std::string toString(const std::filesystem::path& path) {
    const std::u8string u8 = path.u8string();
    return std::string(u8.begin(), u8.end());
}

} // namespace PathUtils


// =========================================================================================================================
// 通用全局函数命名空间
// =========================================================================================================================

namespace Utils {

// ============================================================================
// 全局函数
// ============================================================================
    
// 全局输出函数，在命令栏中输出信息
void cmdLinePrint(const std::string& message);

// 获取全局UI缩放比例，用于快速计算UI布局，因为UI随字号变化而变化，所以只与字号相关
float getUIScaleFactor();

// 获取当前文档对应Database
Database* getWorkingDatabase();

// ============================================================================
// 对话框接口
// ============================================================================

// 三态对话框返回结果（是/否/取消）
enum class TriStateResult {
    kYes,       // 是/保存
    kNo,        // 否/不保存
    kCancel     // 取消
};

/**
 * @brief 显示文件对话框（模态）
 * @param bShowDialog 控制显示/隐藏，ImGui 会管理此值
 * @param bReturned 是否成功返回路径（true=确认，false=取消或关闭）
 * @param outFullPath 输出的完整路径（UTF-8 编码）
 * @param isOpen true=打开文件对话框，false=保存文件对话框
 * @param initialFileName 初始文件名，为空则使用上次文件名或默认"unnamed"
 * @param initialPath 初始路径，为空则使用上次路径或 g_pathCwd(第一次进入)
 * @param title 对话框标题，为空使用默认标题
 */
void showFileDialog(bool& bShowDialog, bool& bReturned, std::filesystem::path& outFullPath,
                   bool isOpen = true,
                   const std::string& initialFileName = "",
                   const std::filesystem::path& initialPath = std::filesystem::path(),
                   const std::string& title = "");

/**
 * @brief 显示消息框（模态对话框）
 * @param bShow 控制显示/隐藏，调用后置为true，关闭后ImGui会设置为false
 * @param message 要显示的消息内容（支持多行，使用\n换行）
 * @param title 对话框标题，为空则使用默认标题"错误"
 * 
 * @note 消息框包含一个确定按钮，按Esc或点击按钮均可关闭
 */
void showMessageBox(bool& bShow, const std::string& message, const std::string& title = "");

/**
 * @brief 显示是/否/取消三态对话框
 * @param bShow 控制显示/隐藏，调用后置为true，关闭后ImGui会设置为false
 * @param result 输出用户选择的结果（kYes/kNo/kCancel）
 * @param title 对话框标题
 * @param message 提示信息内容
 * @param yesLabel "是"按钮标签，为空使用默认"是"
 * @param noLabel "否"按钮标签，为空使用默认"否"
 * @param cancelLabel "取消"按钮标签，为空使用默认"取消"
 * 
 * @note 按钮布局：是 | 否 | 取消（居中）
 *       支持快捷键：Enter=当前焦点按钮（默认是"是"），Esc=取消
 */
void showYesNoCancelDialog(bool& bShow, TriStateResult& result,
                          const std::string& title, const std::string& message,
                          const std::string& yesLabel = "",
                          const std::string& noLabel = "",
                          const std::string& cancelLabel = "");

/**
 * @brief 显示保存确认对话框
 * @param bShow 控制显示/隐藏，调用后置为true，关闭后ImGui会设置为false
 * @param result 输出用户选择的结果（kYes=保存, kNo=不保存, kCancel=取消）
 * @param fileName 文件名（用于显示提示信息）
 * @param title 对话框标题，为空使用默认"确认保存"
 * 
 * @note 内部调用showYesNoCancelDialog，按钮标签为"保存"/"不保存"/"取消"
 */
void showSaveConfirmDialog(bool& bShow, TriStateResult& result,
                          const std::string& fileName,
                          const std::string& title = "");

// ============================================================================
// 文件读写接口
// ============================================================================

/**
 * @brief 读取文本文件内容
 * @param filePath 文件路径
 * @param outContent 输出的文件内容
 * @return 是否成功读取
 */
bool readTextFile(const std::filesystem::path& filePath, std::string& outContent);

/**
 * @brief 写入文本文件内容
 * @param filePath 文件路径
 * @param content 要写入的内容
 * @return 是否成功写入
 */
bool writeTextFile(const std::filesystem::path& filePath, const std::string& content);

} // namespace Utils
} // namespace tch
