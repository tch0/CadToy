#pragma once

#include "command/Command.h"

namespace tch {

// 关闭文档命令
class CommandClose : public Command {
public:
    CommandClose();
    
    // 命令更新方法
    void onUpdate() override;
};

} // namespace tch