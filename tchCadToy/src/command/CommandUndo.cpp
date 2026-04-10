// 对应头文件
#include "CommandUndo.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "GlobalUtils.h"

namespace tch {

CommandUndo::CommandUndo() {
    // 构造函数，不需要特殊初始化
}

void CommandUndo::onUpdate() {
    if (isCompleted()) {
        return;
    }
    
    // 输出占位提示
    Utils::cmdLinePrint("暂未实现命令: undo");
    
    // 命令执行完成，标记为完成状态
    finish();
}

} // namespace tch
