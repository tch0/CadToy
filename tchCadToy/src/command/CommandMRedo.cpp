// 对应头文件
#include "CommandMRedo.h"

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

CommandMRedo::CommandMRedo()
    : m_state(kRedoNumberEntry) {
}

void CommandMRedo::onUpdate() {
    if (isCompleted()) {
        return;
    }
    
    auto& ctx = InputContext::getInstance();
    auto& loc = LocalizationManager::getInstance();
    
    switch (m_state) {
        case kRedoNumberEntry:
            // REDO数量入口
            m_state = kRedoNumberQuery;
            // 输入要重做的操作数目或 [全部(A)] <1>:
            ctx.waitForInteger(loc.get("command.mredo.prompt"), 1, INT_MAX, {"A"});
            break;
            
        case kRedoNumberQuery: {
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
                executeRedo(1, false);
                m_state = kCompleted;
            }
            // 关键字 "A"，全部重做
            else if (status == InputStatus::kKeywordInput) {
                std::string keyword;
                ctx.getKeyword(keyword);
                if (keyword == "A") {
                    executeRedo(0, true);
                }
                m_state = kCompleted;
            }
            // 整数输入
            else if (status == InputStatus::kIntegerInput) {
                int count;
                ctx.getInteger(count);
                executeRedo(count, false);
                m_state = kCompleted;
            }
            break;
        }
            
        case kCompleted:
            finish();
            break;
    }
}

void CommandMRedo::executeRedo(int count, bool allMode) {
    auto& undoManager = UndoManager::getInstance();
    auto& loc = LocalizationManager::getInstance();
    
    int redoneCount = 0;
    while ((allMode || redoneCount < count) && undoManager.canRedo()) {
        std::string name = undoManager.getRedoName();
        undoManager.redo();
        Utils::cmdLinePrint(StringUtils::format(
            loc.get("command.redo.redoSuccess"), name)); // 已重做操作：{}
        redoneCount++;
    }
    
    // 输出最终结果
    if (redoneCount > 0 && !undoManager.canRedo()) {
        Utils::cmdLinePrint(loc.get("command.mredo.allCompleted")); // 所有操作都已重做。
    } else if (redoneCount == 0) {
        Utils::cmdLinePrint(loc.get("command.redo.noRedo")); //没有操作可重做。
    }
}

} // namespace tch
