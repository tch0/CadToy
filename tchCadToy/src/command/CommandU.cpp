// 对应头文件
#include "CommandU.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "GlobalUtils.h"

namespace tch {

CommandU::CommandU() {
    // 构造函数，不需要特殊初始化
}

void CommandU::onUpdate() {
    if (isCompleted()) {
        return;
    }
    
    // 输出占位提示
    cmdLinePrint("暂未实现命令: u");
    
    // 命令执行完成，标记为完成状态
    finish();
}

} // namespace tch
