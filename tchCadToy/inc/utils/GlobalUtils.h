#pragma once

// C++ 标准库
#include <string>

// 第三方库

// 项目头文件

namespace tch {

/**
 * @brief 全局输出函数，在命令栏中输出信息
 * @param message 要输出的信息
 */
void cmdLinePrint(const std::string& message);

/**
 * @brief 获取全局UI缩放比例，用于快速计算UI布局，因为UI随字号变化而变化，所以只与字号相关
 */
float getUIScaleFactor();

} // namespace tch
