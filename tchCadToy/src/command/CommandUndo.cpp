// 对应头文件
#include "CommandUndo.h"

// C++ 标准库
#include <climits>

// 第三方库

// 项目头文件
#include "GlobalUtils.h"
#include "InputContext.h"
#include "LocalizationManager.h"
#include "StringUtils.h"
#include "UndoManager.h"


namespace tch {

CommandUndo::CommandUndo()
    : m_state(kUndoNumberEntry) {
}

void CommandUndo::onUpdate() {
    if (isCompleted()) {
        return;
    }
    
    auto& ctx = InputContext::getInstance();
    auto& loc = LocalizationManager::getInstance();
    
    switch (m_state) {
        case kUndoNumberEntry:
            // UNDO数量入口
            m_state = kUndoNumberQuery;
            // 输入要撤销的操作数目或 [全部(A)] <1>:
            ctx.waitForInteger(loc.get("command.undo.prompt"), 1, INT_MAX, {"A"});
            break;
            
        case kUndoNumberQuery: {
            InputStatus status = ctx.getCurrentStatus();
            
            // 无输入，继续等待
            if (status == InputStatus::kNone) {
                break;
            }
            // Esc 取消
            else if (status == InputStatus::kCanceled) {
                m_state = kCompleted;
            }
            // Enter/Space，使用默认值 1
            else if (status == InputStatus::kEnterInput) {
                executeUndo(1, false);
                m_state = kCompleted;
            }
            // 关键字 "A"，全部撤销
            else if (status == InputStatus::kKeywordInput) {
                std::string keyword;
                ctx.getKeyword(keyword);
                if (keyword == "A") {
                    executeUndo(0, true);
                }
                m_state = kCompleted;
            }
            // 整数输入
            else if (status == InputStatus::kIntegerInput) {
                int count;
                ctx.getInteger(count);
                executeUndo(count, false);
                m_state = kCompleted;
            }
            break;
        }
            
        case kCompleted:
            finish();
            break;
    }
}

void CommandUndo::executeUndo(int count, bool allMode) {
    auto& undoManager = UndoManager::getInstance();
    auto& loc = LocalizationManager::getInstance();
    
    int undoneCount = 0;
    while ((allMode || undoneCount < count) && undoManager.canUndo()) {
        std::string name = undoManager.getUndoName();
        undoManager.undo();
        Utils::cmdLinePrint(StringUtils::format(
            loc.get("command.u.undoSuccess"), name)); // 已撤销操作：{}
        undoneCount++;
    }
    
    // 输出最终结果
    if (undoneCount > 0 && !undoManager.canUndo()) {
        Utils::cmdLinePrint(loc.get("command.undo.allCompleted")); // 所有操作都已撤销。
    } else if (undoneCount == 0) {
        Utils::cmdLinePrint(loc.get("command.u.noUndo")); // 没有操作可撤销。
    }
}

} // namespace tch
