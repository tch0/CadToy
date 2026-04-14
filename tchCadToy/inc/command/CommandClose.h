#pragma once

// C++ 标准库
#include <string>

// 第三方库

// 项目头文件
#include "Command.h"
#include "GlobalUtils.h"

namespace tch {

// 关闭命令状态枚举
enum class CommandCloseState {
    kCheckModified,         // 检查文档是否已修改
    kSaveConfirmEntry,      // 进入保存确认对话框
    kSaveConfirmShow,       // 显示保存确认对话框
    kFileDialogEntry,       // 进入内部文件对话框（如果需要保存）
    kFileDialogShow,        // 显示内部文件对话框
    kImFileDialogEntry,     // 进入ImFileDialog状态
    kImFileDialogShow,      // 显示ImFileDialog
    kCloseDocument,         // 执行关闭文档
    kCompleted              // 完成状态
};

// 关闭文档命令
class CommandClose : public Command {
private:
    CommandCloseState m_state;          // 当前状态
    bool m_showSaveConfirm;             // 控制保存确认对话框显示
    bool m_showFileDialog;              // 控制文件对话框显示
    bool m_dialogReturned;              // 对话框返回结果
    std::string m_selectedPath;         // 用户选择的路径
    Utils::TriStateResult m_saveConfirmResult; // 保存确认结果

public:
    CommandClose();
    
    void onUpdate() override;
};

} // namespace tch
