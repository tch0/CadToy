#pragma once

// C++ 标准库

// 第三方库

// 项目头文件
#include "Command.h"


namespace tch {

// Options命令
class CommandOptions : public Command {
private:
    // Options命令状态枚举
    enum class CommandOptionsState {
        kOptionsDialogEntry,    // 初始化并进入显示状态
        kOptionsDialogShow,     // 显示对话框并等待关闭
        kCompleted              // 完成状态
    };
    
    CommandOptionsState m_state;    // 当前状态

public:
    CommandOptions();
    
    // 命令更新方法
    void onUpdate() override;
};

} // namespace tch
