// 对应头文件
#include "CommandOptions.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "OptionsDialog.h"


namespace tch {

CommandOptions::CommandOptions()
    : m_state(CommandOptionsState::kOptionsDialogEntry) {
}

void CommandOptions::onUpdate() {
    if (isCompleted()) {
        return;
    }
    
    switch (m_state) {
        case CommandOptionsState::kOptionsDialogEntry: {
            // 初始化对话框
            OptionsDialog::getInstance().initialize();
            m_state = CommandOptionsState::kOptionsDialogShow;
            break;
        }
        
        case CommandOptionsState::kOptionsDialogShow: {
            // 显示并绘制对话框，返回false表示已关闭
            if (!OptionsDialog::getInstance().show()) {
                m_state = CommandOptionsState::kCompleted;
            }
            break;
        }
        
        case CommandOptionsState::kCompleted: {
            finish();
            break;
        }
    }
}

} // namespace tch
