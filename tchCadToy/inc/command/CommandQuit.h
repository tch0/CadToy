#pragma once

// C++ 标准库

// 第三方库

// 项目头文件
#include "Command.h"
#include "GlobalUtils.h"

namespace tch {


// 退出应用程序命令（跨文档命令）
class CommandQuit : public Command {
private:
    // 退出命令状态枚举
    enum CommandQuitState {
        kCheckCurrentDocument,   // 检查当前文档（是否修改、是否唯一文档）
        kSaveConfirmEntry,       // 进入保存确认对话框
        kSaveConfirmShow,        // 显示保存确认对话框
        kImFileDialogEntry,      // 进入ImFileDialog（新文档需要保存时）
        kImFileDialogShow,       // 显示ImFileDialog
        kCloseAndContinue,       // 关闭文档并继续（多文档情况）
        kQuitApplication,        // 退出应用程序（单文档未修改情况）
        kCompleted               // 完成状态
    };
    
    CommandQuitState m_state;                   // 当前状态
    Utils::TriStateResult m_saveConfirmResult;  // 保存确认结果
    bool m_showSaveConfirm;                     // 控制保存确认对话框显示
    std::string m_saveConfirmFileName;          // 保存确认对话框显示的文件名（完整路径或文件名）
    
public:
    CommandQuit();
    
    void onUpdate() override;
};

} // namespace tch
