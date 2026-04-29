// 对应头文件
#include "CommandQuit.h"

// C++ 标准库

// 第三方库
#include <ImFileDialog.h>

// 项目头文件
#include "DocManager.h"
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "StringUtils.h"
#include "CommandManager.h"

namespace tch {

CommandQuit::CommandQuit() :
    m_state(kCheckCurrentDocument),
    m_saveConfirmResult(Utils::TriStateResult::kCancel),
    m_showSaveConfirm(false),
    m_saveConfirmFileName() {
}

void CommandQuit::onUpdate() {
    auto& loc = LocalizationManager::getInstance();

    if (isCompleted()) {
        return;
    }

    switch (m_state) {
        case kCheckCurrentDocument:
        {
            Document& doc = DocManager::getCurrentDocument();
            // 文档已修改
            if (doc.isModified()) {
                // 文档已修改，进入保存确认流程
                m_state = kSaveConfirmEntry;
            }
            // 文档未修改
            else {
                // 只有一个文档，直接退出应用程序（不需要关闭文档，避免自动创建新文档）
                if (DocManager::getDocumentCount() == 1) {
                    m_state = kQuitApplication;
                }
                // 多个文档，关闭当前文档继续处理下一个
                else {
                    m_state = kCloseAndContinue;
                }
            }
            break;
        }

        case kSaveConfirmEntry:
        {
            // 初始化保存确认对话框
            m_showSaveConfirm = true;
            m_saveConfirmResult = Utils::TriStateResult::kCancel;
            // 获取要显示的文件名：已保存文档显示完整路径，未保存文档显示文件名
            Document& doc = DocManager::getCurrentDocument();
            if (doc.isSaved()) {
                m_saveConfirmFileName = PathUtils::toString(doc.getFilePath());
            } else {
                m_saveConfirmFileName = doc.getFullFileName();
            }
            m_state = kSaveConfirmShow;
            break;
        }

        case kSaveConfirmShow:
        {
            // 显示保存确认对话框
            Utils::showSaveConfirmDialog(m_showSaveConfirm, m_saveConfirmResult, m_saveConfirmFileName);

            // 检查对话框是否已关闭
            if (!m_showSaveConfirm) {
                Document& doc = DocManager::getCurrentDocument();
                // 根据用户选择决定下一步
                // 用户选择保存，检查文档是否已保存过
                if (m_saveConfirmResult == Utils::TriStateResult::kYes) {
                    // 已保存过，直接保存
                    if (doc.isSaved()) {
                        // 保存成功，回到检查状态
                        if (doc.saveToFile(doc.getFilePath())) {
                            m_state = kCheckCurrentDocument;
                        }
                        // 保存失败，结束命令
                        else {
                            Utils::cmdLinePrint(StringUtils::format(loc.get("command.save.failed"), PathUtils::toString(doc.getFilePath())));
                            m_state = kCompleted;
                        }
                    }
                    // 未保存过，需要打开文件对话框
                    else {
                        m_state = kImFileDialogEntry;
                    }
                }
                // 用户选择不保存，关闭文档（关闭后会自动切换到下一个，然后回到检查状态）
                else if (m_saveConfirmResult == Utils::TriStateResult::kNo) {
                    m_state = kCloseAndContinue;
                }
                // 用户取消退出，结束命令
                else if (m_saveConfirmResult == Utils::TriStateResult::kCancel) {
                    m_state = kCompleted;
                }
            }
            break;
        }

        case kImFileDialogEntry:
        {
            // 获取初始文件名
            Document& doc = DocManager::getCurrentDocument();
            std::string fullFileName = doc.getFullFileName();
            if (fullFileName.empty()) {
                fullFileName = "unnamed";
            }

            // 打开ImFileDialog（只调用一次）
            ifd::FileDialog::getInstance().save("QuitSaveDialog",
                loc.get("fileDialog.title.save").c_str(),
                "*.cad.json {.json},.*",
                fullFileName);

            m_state = kImFileDialogShow;
            break;
        }

        case kImFileDialogShow:
        {
            // 检查对话框是否完成
            if (ifd::FileDialog::getInstance().isDone("QuitSaveDialog")) {
                // 用户确认了保存路径
                if (ifd::FileDialog::getInstance().hasResult()) {
                    std::filesystem::path result = ifd::FileDialog::getInstance().getResult();
                    Document& doc = DocManager::getCurrentDocument();
                    // 保存成功，回到检查状态
                    if (doc.saveToFile(result)) {
                        m_state = kCheckCurrentDocument;
                    }
                    // 保存失败，结束命令
                    else {
                        Utils::cmdLinePrint(StringUtils::format(loc.get("command.save.failed"), PathUtils::toString(result)));
                        m_state = kCompleted;
                    }
                }
                // 用户取消保存，结束命令
                else {
                    m_state = kCompleted;
                }
                // 关闭对话框
                ifd::FileDialog::getInstance().close();
            }
            break;
        }

        case kCloseAndContinue:
        {
            // 关闭当前文档，closeDocument会自动切换当前文档到下一个，关闭最后一个会自动创建一个未修改的新文档
            DocManager::closeDocument(DocManager::getCurrentDocumentIndex());
            // 继续检查新的当前文档
            m_state = kCheckCurrentDocument;
            break;
        }

        case kQuitApplication:
        {
            // 请求关闭应用程序
            CommandManager::getInstance().requestQuitApplication();
            finish();
            break;
        }

        case kCompleted:
        {
            finish();
            break;
        }
    }
}

} // namespace tch
