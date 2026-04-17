// 对应头文件
#include "CommandOpen.h"

// C++ 标准库

// 第三方库
#include <ImFileDialog.h>

// 项目头文件
#include "DocManager.h"
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "StringUtils.h"

namespace tch {

CommandOpen::CommandOpen() :
    // m_state初始化为kFileDialogEntry即可启用内部实现的文件对话框
    m_state(CommandOpenState::kImFileDialogEntry),
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
                    
                    // 文件已打开
                    if (alreadyOpened && existingDocIndex != DocManager::InvalidDocIndex) {
                        // 且不是当前文档，则需要切换
                        if (existingDocIndex != DocManager::getCurrentDocumentIndex()) {
                            // 但不能立即切换，而是延迟到ImGui逻辑中通知ImGui去切换，这一帧后续就会处理，
                            // 如果直接设置则ImGui内部状态未被改变，ImGui不知情的情况下又被切回ImGui内部状态对应的那一个，
                            // 这是使用ImGui不得不面临的问题，只有这个办法解决，
                            DocManager::setPendingSwitchIndexFromCommand(existingDocIndex);
                        }
                        // else 是当前文档，则什么也不做
                        // 直接调用finish提前结束命令，命令不能跨文档，确保完成切换之前命令已经结束，旧命令也不会在新文档中输出任何信息
                        finish();
                        break;
                    } else {
                        // 文件未打开，执行打开
                        std::size_t docIndex = DocManager::openFile(m_selectedPath);
                        if (docIndex != DocManager::InvalidDocIndex) {
                            // 打开成功，此时文档已经自动在OpenFile中切换，但这里依然要通知以处理命令中的切换
                            DocManager::setPendingSwitchIndexFromCommand(docIndex);
                            // 同样提前结束命令
                            finish();
                            break;
                        } else {
                            // 打开失败
                            Utils::cmdLinePrint(StringUtils::format(loc.get("command.open.failed"), PathUtils::toString(m_selectedPath)));
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
        
        case CommandOpenState::kImFileDialogEntry:
        {
            // 打开ImFileDialog（只调用一次）
            // 打开时不需要传入文件名，使用单选模式
            ifd::FileDialog::getInstance().open("OpenDialog",
                loc.get("fileDialog.title.open").c_str(),
                "*.cad.json {.json},.*");
            
            m_state = CommandOpenState::kImFileDialogShow;
            break;
        }
        
        case CommandOpenState::kImFileDialogShow:
        {
            // 检查对话框是否完成
            if (ifd::FileDialog::getInstance().isDone("OpenDialog")) {
                if (ifd::FileDialog::getInstance().hasResult()) {
                    // 用户确认了选择
                    std::filesystem::path result = ifd::FileDialog::getInstance().getResult();
                    
                    // 查重：遍历所有文档，检查该文件是否已打开
                    bool alreadyOpened = false;
                    std::size_t existingDocIndex = DocManager::InvalidDocIndex;
                    std::size_t docCount = DocManager::getDocumentCount();
                    
                    for (std::size_t i = 0; i < docCount; ++i) {
                        if (DocManager::getFilePath(i) == result) {
                            alreadyOpened = true;
                            existingDocIndex = i;
                            break;
                        }
                    }
                    
                    // 文件已打开
                    if (alreadyOpened && existingDocIndex != DocManager::InvalidDocIndex) {
                        // 且不是当前文档，则需要切换
                        if (existingDocIndex != DocManager::getCurrentDocumentIndex()) {
                            DocManager::setPendingSwitchIndexFromCommand(existingDocIndex);
                        }
                        // 直接调用finish提前结束命令
                        finish();
                        // 关闭对话框
                        ifd::FileDialog::getInstance().close();
                        break;
                    } else {
                        // 文件未打开，执行打开
                        std::size_t docIndex = DocManager::openFile(result);
                        if (docIndex != DocManager::InvalidDocIndex) {
                            // 打开成功
                            DocManager::setPendingSwitchIndexFromCommand(docIndex);
                            // 同样提前结束命令
                            finish();
                            // 关闭对话框
                            ifd::FileDialog::getInstance().close();
                            break;
                        } else {
                            // 打开失败
                            Utils::cmdLinePrint(StringUtils::format(loc.get("command.open.failed"), PathUtils::toString(result)));
                        }
                    }
                } else {
                    // 用户取消
                    Utils::cmdLinePrint(loc.get("command.open.canceled"));
                }
                // 关闭对话框
                ifd::FileDialog::getInstance().close();
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
