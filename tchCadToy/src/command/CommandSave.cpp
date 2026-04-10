// 对应头文件
#include "CommandSave.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "DocManager.h"
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "StringUtils.h"

namespace tch {

CommandSave::CommandSave() :
    m_state(CommandSaveState::kDocumentSavedQuery),
    m_showDialog(false),
    m_dialogReturned(false),
    m_selectedPath() {
}

void CommandSave::onUpdate() {
    auto& loc = LocalizationManager::getInstance();
    
    // 检查是否已经完成
    if (isCompleted()) {
        return;
    }
    
    switch (m_state) {
        case CommandSaveState::kDocumentSavedQuery:
        {
            // 获取当前文档
            Document& doc = DocManager::getCurrentDocument();
            
            // 检查文档是否已关联文件
            if (doc.isSaved()) {
                // 已关联文件，直接保存
                if (doc.saveToFile()) {
                    Utils::cmdLinePrint(StringUtils::format(loc.get("command.save.saved"), doc.getFullPath()));
                } else {
                    Utils::cmdLinePrint(StringUtils::format(loc.get("command.save.failed"), doc.getFullPath()));
                }
                m_state = CommandSaveState::kCompleted;
            } else {
                // 未关联文件，需要打开对话框
                m_state = CommandSaveState::kFileDialogEntry;
            }
            break;
        }
            
        case CommandSaveState::kFileDialogEntry:
        {
            // 初始化对话框参数
            m_showDialog = true;
            m_dialogReturned = false;
            m_selectedPath.clear();
            m_state = CommandSaveState::kFileDialogShow;
            break;
        }
            
        case CommandSaveState::kFileDialogShow:
        {
            // 显示文件对话框
            Utils::showFileDialog(m_showDialog, m_dialogReturned, m_selectedPath, false);
            
            // 先检查对话框是否已关闭
            if (!m_showDialog) {
                // 对话框已关闭，再检查返回值
                if (m_dialogReturned) {
                    // 用户确认了选择，执行保存
                    Document& doc = DocManager::getCurrentDocument();
                    if (doc.saveToFile(m_selectedPath)) {
                        Utils::cmdLinePrint(StringUtils::format(loc.get("command.save.saved"), doc.getFullPath()));
                    } else {
                        Utils::cmdLinePrint(StringUtils::format(loc.get("command.save.failed"), m_selectedPath));
                    }
                } else {
                    // 用户取消或关闭对话框
                    Utils::cmdLinePrint(loc.get("command.save.canceled"));
                }
                m_state = CommandSaveState::kCompleted;
            }
            break;
        }
            
        case CommandSaveState::kCompleted:
        {
            // 命令完成
            finish();
            break;
        }
    }
}

} // namespace tch
