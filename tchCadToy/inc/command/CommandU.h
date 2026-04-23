#pragma once

// C++ 标准库

// 第三方库

// 项目头文件
#include "Command.h"

namespace tch {

// 撤销命令（简版，只撤销上一条命令/操作）
class CommandU : public Command {
public:
    CommandU();
    
    void onUpdate() override;
    
    // 跳过 undo 记录
    bool skipUndoRecording() const override { return true; }
};

} // namespace tch
