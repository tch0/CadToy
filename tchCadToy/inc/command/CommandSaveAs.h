#pragma once

// C++ 标准库
#include <string>

// 第三方库

// 项目头文件
#include "Command.h"

namespace tch {

// 另存为命令
class CommandSaveAs : public Command {
private:
    // 另存为命令状态枚举
    enum class CommandSaveAsState {
        kFileDialogEntry,       // 进入文件对话框状态
        kFileDialogShow,        // 显示文件对话框并等待用户选择
        kCompleted              // 完成状态
    };
    
    CommandSaveAsState m_state;     // 当前状态
    bool m_showDialog;              // 控制对话框显示
    bool m_dialogReturned;          // 对话框是否返回结果
    std::string m_selectedPath;     // 用户选择的路径

public:
    CommandSaveAs();
    
    // 命令更新方法
    void onUpdate() override;
};

} // namespace tch
