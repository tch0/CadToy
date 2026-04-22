#pragma once

// C++ 标准库

// 第三方库

// 项目头文件
#include "Command.h"


namespace tch {

// 重生成命令
class CommandRegen : public Command {
public:
    CommandRegen();
    
    // 命令更新方法
    void onUpdate() override;
};

} // namespace tch
