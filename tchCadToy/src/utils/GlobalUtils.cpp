// 对应头文件
#include "GlobalUtils.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "Renderer.h"
#include "DisplayConfigManager.h"
#include "DocManager.h"


namespace tch {

namespace Utils {

// 全局输出函数，在命令栏中输出信息
void cmdLinePrint(const std::string& content) {
    Renderer::addContentToCommandLineHistory(content);
}

// 获取全局UI缩放比例，用于快速计算UI布局，因为UI随字号变化而变化，所以只与字号相关
float getUIScaleFactor() {
    return DisplayConfigManager::getInstance().getCurrentFontSize() / 18.0f;
}

// 获取当前文档对应Database
Database* getWorkingDatabase() {
    return DocManager::getCurrentDocument().getDatabase();
}


} // namespace Utils

} // namespace tch
