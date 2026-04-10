#pragma once

// C++ 标准库
#include <string>

// 第三方库

// 项目头文件
#include "Database.h"


namespace tch {
namespace Utils {

// 全局输出函数，在命令栏中输出信息
void cmdLinePrint(const std::string& message);

// 获取全局UI缩放比例，用于快速计算UI布局，因为UI随字号变化而变化，所以只与字号相关
float getUIScaleFactor();

// 获取当前文档对应Database
Database* getWorkingDatabase();

/**
 * @brief 显示文件对话框（模态）
 * @param bShowDialog 控制显示/隐藏，ImGui 会管理此值
 * @param bReturned 是否成功返回路径（true=确认，false=取消或关闭）
 * @param outFullPath 输出的完整路径（UTF-8 编码）
 * @param isOpen true=打开文件对话框，false=保存文件对话框
 * @param initialPath 初始路径，为空则使用上次路径或 g_pathCwd(第一次进入)
 * @param title 对话框标题，为空使用默认标题
 * 
 * @note 返回的路径是 UTF-8 编码，如需用于系统 API（如 fstream），
 *       请使用 PlatformUtils::Path 或 PlatformUtils::utf8ToLocal() 转换
 */
void showFileDialog(bool& bShowDialog, bool& bReturned, std::string& outFullPath,
                   bool isOpen = true, const std::string& initialPath = "",
                   const std::string& title = "");

/**
 * @brief 读取文本文件内容
 * @param filePathUtf8 文件路径（UTF-8 编码）
 * @param outContent 输出的文件内容
 * @return 是否成功读取
 * 
 * @note 内部自动处理编码转换，调用者只需传入 UTF-8 路径
 */
bool readTextFile(const std::string& filePathUtf8, std::string& outContent);

/**
 * @brief 写入文本文件内容
 * @param filePathUtf8 文件路径（UTF-8 编码）
 * @param content 要写入的内容
 * @return 是否成功写入
 * 
 * @note 内部自动处理编码转换，调用者只需传入 UTF-8 路径
 */
bool writeTextFile(const std::string& filePathUtf8, const std::string& content);

} // namespace Utils
} // namespace tch
