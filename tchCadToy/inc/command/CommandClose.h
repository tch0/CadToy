#pragma once

#include "command/Command.h"

namespace tch {

// 关闭文档命令
class CommandClose : public Command {
public:
    CommandClose();
    
    void onUpdate() override;
};

} // namespace tch