#pragma once

// C++ 标准库

// 第三方库

// 项目头文件
#include "Command.h"

namespace tch {

// 重做命令
class CommandRedo : public Command {
public:
    CommandRedo();
    
    void onUpdate() override;
    
    // 跳过 undo 记录
    bool skipUndoRecording() const override { return true; }
};

} // namespace tch
