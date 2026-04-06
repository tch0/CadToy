#pragma once

// C++ 标准库

// 第三方库

// 项目头文件
#include "Command.h"

namespace tch {

// 撤销命令（复杂版本，带分支）
class CommandUndo : public Command {
public:
    CommandUndo();
    
    void onUpdate() override;
};

} // namespace tch
