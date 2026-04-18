#pragma once

// C++ 标准库

// 第三方库

// 项目头文件
#include "Command.h"

namespace tch {

// 新建命令 - 创建新文档并切换
class CommandNew : public Command {
public:
    CommandNew();
    
    // 命令更新方法
    void onUpdate() override;
};

} // namespace tch
