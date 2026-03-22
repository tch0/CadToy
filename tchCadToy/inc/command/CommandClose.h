#pragma once

// C++ 标准库

// 第三方库

// 项目头文件
#include "command/Command.h"

namespace tch {

// 关闭文档命令
class CommandClose : public Command {
public:
    CommandClose();
    
    void onUpdate() override;
};

} // namespace tch