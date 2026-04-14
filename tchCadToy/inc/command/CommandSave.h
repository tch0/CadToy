#pragma once

// C++ 标准库
#include <string>

// 第三方库

// 项目头文件
#include "Command.h"

namespace tch {

// 保存命令
class CommandSave : public Command {
private:
    // 保存命令状态枚举
    enum class CommandSaveState {
        kDocumentSavedQuery,    // 查询文档是否已关联文件
        kFileDialogEntry,       // 进入内部文件对话框状态
        kFileDialogShow,        // 显示内部文件对话框并等待用户选择
        kImFileDialogEntry,     // 进入ImFileDialog状态
        kImFileDialogShow,      // 显示ImFileDialog并等待用户选择
        kCompleted              // 完成状态
    };
    
    CommandSaveState m_state;       // 当前状态
    bool m_showDialog;              // 控制对话框显示
    bool m_dialogReturned;          // 对话框是否返回结果
    std::string m_selectedPath;     // 用户选择的路径

public:
    CommandSave();
    
    // 命令更新方法
    void onUpdate() override;
};

} // namespace tch
