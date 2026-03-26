// 对应头文件
#include "utils/GlobalUtils.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "render/Renderer.h"

namespace tch {

/**
 * @brief 全局输出函数，在命令行中打印内容
 * @param content 要输出的内容
 */
void cmdLinePrint(const std::string& content) {
    Renderer::addContentToCommandLineHistory(content);
}

} // namespace tch
