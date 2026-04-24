#pragma once

// C++ 标准库

// 第三方库

// 项目头文件
#include "Command.h"


namespace tch {

// 撤销命令（支持批量撤销）
class CommandUndo : public Command {
public:
    CommandUndo();
    
    void onUpdate() override;
    
    // 跳过 undo 记录
    bool skipUndoRecording() const override { return true; }
    
private:
    enum CommandUndoState {
        kUndoNumberEntry,       // UNDO数量入口
        kUndoNumberQuery,       // UNDO数量输入查询
        kCompleted              // 结束状态
    };
    
    CommandUndoState m_state;
    
    // 执行撤销
    void executeUndo(int count, bool allMode);
};

} // namespace tch
