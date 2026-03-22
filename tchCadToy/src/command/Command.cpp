// 对应头文件
#include "command/Command.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "input/InputContext.h"

namespace tch {

// 命令是否完成
bool Command::isCompleted() const {
    return m_completed;
}

// 宣告命令结束，每个命令在完成所有流程后都应该调用此方法，允许多次重复调用，无任何副作用
void Command::finish() {
    // 统一的结束操作
    InputContext::getInstance().resetStatus();
    m_completed = true;
}

} // namespace tch
