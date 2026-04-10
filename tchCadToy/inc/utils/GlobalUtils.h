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


// 全局UI函数：对话框、窗口等


} // namespace Utils

} // namespace tch
