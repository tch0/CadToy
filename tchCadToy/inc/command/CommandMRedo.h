#pragma once

// C++ 标准库

// 第三方库

// 项目头文件
#include "Command.h"


namespace tch {

// 重做命令（支持批量重做）
class CommandMRedo : public Command {
public:
    CommandMRedo();
    
    void onUpdate() override;
    
    // 跳过 undo 记录
    bool skipUndoRecording() const override { return true; }
    
private:
    enum class CommandMRedoState {
        kRedoNumberEntry,       // REDO数量入口
        kRedoNumberQuery,       // REDO数量输入查询
        kCompleted              // 结束状态
    };
    
    CommandMRedoState m_state;
    
    // 执行重做
    void executeRedo(int count, bool allMode);
};

} // namespace tch
