#pragma once

// C++ 标准库
#include <filesystem>

// 第三方库

// 项目头文件
#include "Command.h"

namespace tch {

// 打开命令
class CommandOpen : public Command {
private:
    // 打开命令状态枚举
    enum class CommandOpenState {
        kFileDialogEntry,       // 进入内部文件对话框状态
        kFileDialogShow,        // 显示内部文件对话框并等待用户选择
        kImFileDialogEntry,     // 进入ImFileDialog状态
        kImFileDialogShow,      // 显示ImFileDialog并等待用户选择
        kCompleted              // 完成状态
    };
    
    CommandOpenState m_state;               // 当前状态
    // 内部文件对话框相关
    bool m_showDialog;                      // 控制对话框显示
    bool m_dialogReturned;                  // 对话框是否返回结果
    std::filesystem::path m_selectedPath;   // 用户选择的路径

public:
    CommandOpen();
    
    // 命令更新方法
    void onUpdate() override;
};

} // namespace tch
