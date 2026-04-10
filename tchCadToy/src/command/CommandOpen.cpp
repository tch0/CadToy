// 对应头文件
#include "CommandOpen.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "DocManager.h"
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "StringUtils.h"

namespace tch {

CommandOpen::CommandOpen() :
    m_state(CommandOpenState::kFileDialogEntry),
    m_showDialog(false),
    m_dialogReturned(false),
    m_selectedPath() {
}

void CommandOpen::onUpdate() {
    auto& loc = LocalizationManager::getInstance();
    
    // 检查是否已经完成
    if (isCompleted()) {
        return;
    }
    
    switch (m_state) {
        case CommandOpenState::kFileDialogEntry:
        {
            // 初始化对话框参数
            m_showDialog = true;
            m_dialogReturned = false;
            m_selectedPath.clear();
            m_state = CommandOpenState::kFileDialogShow;
            break;
        }
            
        case CommandOpenState::kFileDialogShow:
        {
            // 显示文件对话框（打开模式）
            Utils::showFileDialog(m_showDialog, m_dialogReturned, m_selectedPath, true);
            
            // 先检查对话框是否已关闭
            if (!m_showDialog) {
                // 对话框已关闭，再检查返回值
                if (m_dialogReturned) {
                    // 查重：遍历所有文档，检查该文件是否已打开
                    bool alreadyOpened = false;
                    std::size_t existingDocIndex = DocManager::InvalidDocIndex;
                    std::size_t docCount = DocManager::getDocumentCount();
                    
                    for (std::size_t i = 0; i < docCount; ++i) {
                        if (DocManager::getFilePath(i) == m_selectedPath) {
                            alreadyOpened = true;
                            existingDocIndex = i;
                            break;
                        }
                    }
                    
                    if (alreadyOpened && existingDocIndex != DocManager::InvalidDocIndex) {
                        // 文件已打开，切换到该文档
                        DocManager::setCurrentDocumentIndex(existingDocIndex);
                        Document& doc = DocManager::getCurrentDocument();
                        Utils::cmdLinePrint(StringUtils::format(loc.get("command.open.switched"), doc.getFullPath()));
                    } else {
                        // 文件未打开，执行打开
                        std::size_t docIndex = DocManager::openFile(m_selectedPath);
                        if (docIndex != DocManager::InvalidDocIndex) {
                            // 打开成功
                            Document& doc = DocManager::getCurrentDocument();
                            Utils::cmdLinePrint(StringUtils::format(loc.get("command.open.opened"), doc.getFullPath()));
                        } else {
                            // 打开失败
                            Utils::cmdLinePrint(StringUtils::format(loc.get("command.open.failed"), m_selectedPath));
                        }
                    }
                } else {
                    // 用户取消或关闭对话框
                    Utils::cmdLinePrint(loc.get("command.open.canceled"));
                }
                m_state = CommandOpenState::kCompleted;
            }
            break;
        }
            
        case CommandOpenState::kCompleted:
        {
            // 命令完成
            finish();
            break;
        }
    }
}

} // namespace tch
