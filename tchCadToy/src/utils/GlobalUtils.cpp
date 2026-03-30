// 对应头文件
#include "utils/GlobalUtils.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "render/Renderer.h"
#include "utils/DisplayConfigManager.h"

namespace tch {

/**
 * @brief 全局输出函数，在命令行中打印内容
 * @param content 要输出的内容
 */
void cmdLinePrint(const std::string& content) {
    Renderer::addContentToCommandLineHistory(content);
}

/**
 * @brief 获取全局UI缩放比例，用于快速计算UI布局，因为UI随字号变化而变化，所以只与字号相关
 */
float getUIScaleFactor() {
    return DisplayConfigManager::getInstance().getCurrentFontSize() / 18.0f;
}

} // namespace tch
