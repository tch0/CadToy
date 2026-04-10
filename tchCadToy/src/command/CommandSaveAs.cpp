// 对应头文件
#include "CommandSaveAs.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "DocManager.h"
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "StringUtils.h"

namespace tch {

CommandSaveAs::CommandSaveAs() :
    m_state(CommandSaveAsState::kFileDialogEntry),
    m_showDialog(false),
    m_dialogReturned(false),
    m_selectedPath() {
}

void CommandSaveAs::onUpdate() {
    auto& loc = LocalizationManager::getInstance();
    
    // 检查是否已经完成
    if (isCompleted()) {
        return;
    }
    
    switch (m_state) {
        case CommandSaveAsState::kFileDialogEntry:
        {
            // 初始化对话框参数
            m_showDialog = true;
            m_dialogReturned = false;
            m_selectedPath.clear();
            m_state = CommandSaveAsState::kFileDialogShow;
            break;
        }
            
        case CommandSaveAsState::kFileDialogShow:
        {
            // 显示文件对话框（另存为模式）
            Utils::showFileDialog(m_showDialog, m_dialogReturned, m_selectedPath, false);
            
            // 先检查对话框是否已关闭
            if (!m_showDialog) {
                // 对话框已关闭，再检查返回值
                if (m_dialogReturned) {
                    // 用户确认了选择，执行另存为
                    Document& doc = DocManager::getCurrentDocument();
                    if (doc.saveToFile(m_selectedPath)) {
                        Utils::cmdLinePrint(StringUtils::format(loc.get("command.saveas.saved"), doc.getFullPath()));
                    } else {
                        Utils::cmdLinePrint(StringUtils::format(loc.get("command.saveas.failed"), m_selectedPath));
                    }
                } else {
                    // 用户取消或关闭对话框
                    Utils::cmdLinePrint(loc.get("command.saveas.canceled"));
                }
                m_state = CommandSaveAsState::kCompleted;
            }
            break;
        }
            
        case CommandSaveAsState::kCompleted:
        {
            // 命令完成
            finish();
            break;
        }
    }
}

} // namespace tch
