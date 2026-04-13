// 对应头文件
#include "CommandSaveAs.h"

// C++ 标准库

// 第三方库
#include <ImFileDialog.h>

// 项目头文件
#include "DocManager.h"
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "StringUtils.h"

namespace tch {

CommandSaveAs::CommandSaveAs() :
    // m_state初始化为kFileDialogEntry即可启用内部实现的文件对话框
    m_state(CommandSaveAsState::kImFileDialogEntry),
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
            // 显示文件对话框（另存为模式），传入当前文件名作为初始文件名
            Document& doc = DocManager::getCurrentDocument();
            Utils::showFileDialog(m_showDialog, m_dialogReturned, m_selectedPath, false, doc.getFileName());
            
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
        
        case CommandSaveAsState::kImFileDialogEntry:
        {
            // 获取初始文件名
            Document& doc = DocManager::getCurrentDocument();
            std::string fullFileName = doc.getFullFileName();
            if (fullFileName.empty()) {
                fullFileName = "unnamed";
            }
            
            // 打开ImFileDialog（只调用一次）
            ifd::FileDialog::getInstance().save("SaveAsDialog",
                loc.get("fileDialog.title.save").c_str(),
                "*.cad.json {.json},.*",
                fullFileName);
            
            m_state = CommandSaveAsState::kImFileDialogShow;
            break;
        }
            
        case CommandSaveAsState::kImFileDialogShow:
        {
            // 检查对话框是否完成
            if (ifd::FileDialog::getInstance().isDone("SaveAsDialog")) {
                if (ifd::FileDialog::getInstance().hasResult()) {
                    // 用户确认了选择
                    std::string result = ifd::u8_to_string(ifd::FileDialog::getInstance().getResult().u8string());
                    Document& doc = DocManager::getCurrentDocument();
                    if (doc.saveToFile(result)) {
                        Utils::cmdLinePrint(StringUtils::format(loc.get("command.saveas.saved"), doc.getFullPath()));
                    } else {
                        Utils::cmdLinePrint(StringUtils::format(loc.get("command.saveas.failed"), result));
                    }
                } else {
                    // 用户取消
                    Utils::cmdLinePrint(loc.get("command.saveas.canceled"));
                }
                // 关闭对话框
                ifd::FileDialog::getInstance().close();
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
