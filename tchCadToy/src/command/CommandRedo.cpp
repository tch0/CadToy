// 对应头文件
#include "CommandRedo.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "UndoManager.h"
#include "StringUtils.h"


namespace tch {

CommandRedo::CommandRedo() = default;

void CommandRedo::onUpdate() {
    if (isCompleted()) {
        return;
    }
    
    auto& loc = LocalizationManager::getInstance();
    auto& undoManager = UndoManager::getInstance();
    
    // 先结束自己的 undo 组，避免重做自己的空组
    undoManager.endGroup();
    
    if (undoManager.canRedo()) {
        // 获取下一个操作名称
        std::string redoName = undoManager.getRedoName();
        // 执行重做
        undoManager.redo();
        // 输出成功信息
        Utils::cmdLinePrint(StringUtils::format(loc.get("command.redo.redoSuccess"), redoName)); // 已重做操作：{}
    } else {
        // 没有操作可重做
        Utils::cmdLinePrint(loc.get("command.redo.noRedo")); // 没有操作可重做。
    }
    
    finish();
}

} // namespace tch
