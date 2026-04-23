// 对应头文件
#include "CommandU.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "UndoManager.h"
#include "StringUtils.h"


namespace tch {

CommandU::CommandU() = default;

void CommandU::onUpdate() {
    if (isCompleted()) {
        return;
    }
    
    auto& loc = LocalizationManager::getInstance();
    auto& undoManager = UndoManager::getInstance();
    
    // 先结束自己的 undo 组，避免撤销自己
    undoManager.endGroup();
    
    if (undoManager.canUndo()) {
        // 获取上一个操作名称
        std::string undoName = undoManager.getUndoName();
        // 执行撤销
        undoManager.undo();
        // 输出成功信息
        Utils::cmdLinePrint(StringUtils::format(loc.get("command.u.undoSuccess"), undoName)); // 已撤销操作：{}
    } else {
        // 没有操作可撤销
        Utils::cmdLinePrint(loc.get("command.u.noUndo")); // 没有操作可撤销
    }
    
    finish();
}

} // namespace tch
