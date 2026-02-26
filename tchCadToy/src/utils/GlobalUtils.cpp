#include "utils/GlobalUtils.h"
#include "render/Renderer.h"

namespace tch {

/**
 * @brief 全局输出函数，在命令栏中输出信息
 * @param message 要输出的信息
 */
void cmdLinePrint(const std::string& message) {
    Renderer::addContentToCommandHistory(message);
}

} // namespace tch
