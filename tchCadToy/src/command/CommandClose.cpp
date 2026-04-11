// 对应头文件
#include "CommandClose.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "DocManager.h"
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "StringUtils.h"

namespace tch {

CommandClose::CommandClose() :
    m_state(CommandCloseState::kCheckModified),
    m_showSaveConfirm(false),
    m_showFileDialog(false),
    m_dialogReturned(false),
    m_selectedPath(),
    m_saveConfirmResult(Utils::TriStateResult::kCancel) {
}

void CommandClose::onUpdate() {
    auto& loc = LocalizationManager::getInstance();
    
    if (isCompleted()) {
        return;
    }
    
    switch (m_state) {
        case CommandCloseState::kCheckModified:
        {
            // 检查当前文档是否已修改
            Document& doc = DocManager::getCurrentDocument();
            if (doc.isModified()) {
                // 文档已修改，需要显示保存确认对话框
                m_state = CommandCloseState::kSaveConfirmEntry;
            } else {
                // 文档未修改，直接关闭
                m_state = CommandCloseState::kCloseDocument;
            }
            break;
        }
            
        case CommandCloseState::kSaveConfirmEntry:
        {
            // 初始化保存确认对话框
            m_showSaveConfirm = true;
            m_saveConfirmResult = Utils::TriStateResult::kCancel;
            m_state = CommandCloseState::kSaveConfirmShow;
            break;
        }
            
        case CommandCloseState::kSaveConfirmShow:
        {
            // 显示保存确认对话框
            Document& doc = DocManager::getCurrentDocument();
            Utils::showSaveConfirmDialog(m_showSaveConfirm, m_saveConfirmResult, doc.getFullFileName());
            
            // 检查对话框是否已关闭
            if (!m_showSaveConfirm) {
                // 根据用户选择决定下一步
                switch (m_saveConfirmResult) {
                    case Utils::TriStateResult::kYes:
                        // 用户选择保存
                        m_state = CommandCloseState::kSaveDialogEntry;
                        break;
                    case Utils::TriStateResult::kNo:
                        // 用户选择不保存，直接关闭
                        m_state = CommandCloseState::kCloseDocument;
                        break;
                    case Utils::TriStateResult::kCancel:
                        // 用户取消关闭
                        Utils::cmdLinePrint(loc.get("command.close.canceled"));
                        m_state = CommandCloseState::kCompleted;
                        break;
                }
            }
            break;
        }
            
        case CommandCloseState::kSaveDialogEntry:
        {
            // 初始化文件保存对话框
            m_showFileDialog = true;
            m_dialogReturned = false;
            m_selectedPath.clear();
            m_state = CommandCloseState::kSaveDialogShow;
            break;
        }
            
        case CommandCloseState::kSaveDialogShow:
        {
            // 显示文件保存对话框，传入当前文件名作为初始文件名
            Document& doc = DocManager::getCurrentDocument();
            Utils::showFileDialog(m_showFileDialog, m_dialogReturned, m_selectedPath, false, doc.getFileName());
            
            // 检查对话框是否已关闭
            if (!m_showFileDialog) {
                if (m_dialogReturned) {
                    // 用户确认了保存路径，执行保存
                    Document& doc = DocManager::getCurrentDocument();
                    if (doc.saveToFile(m_selectedPath)) {
                        // 保存成功，关闭文档
                        m_state = CommandCloseState::kCloseDocument;
                    } else {
                        // 保存失败，取消关闭
                        Utils::cmdLinePrint(StringUtils::format(loc.get("command.save.failed"), m_selectedPath));
                        m_state = CommandCloseState::kCompleted;
                    }
                } else {
                    // 用户取消保存，取消关闭
                    Utils::cmdLinePrint(loc.get("command.close.canceled"));
                    m_state = CommandCloseState::kCompleted;
                }
            }
            break;
        }
            
        case CommandCloseState::kCloseDocument:
        {
            // 执行关闭文档
            std::size_t currentIndex = DocManager::getCurrentDocumentIndex();
            DocManager::closeDocument(currentIndex);
            // 关闭后不需要输出，并且直接提前结束命令，而不是等到下一个状态才结束
            finish();
            break;
        }
            
        case CommandCloseState::kCompleted:
        {
            // 命令完成
            finish();
            break;
        }
    }
}

} // namespace tch
