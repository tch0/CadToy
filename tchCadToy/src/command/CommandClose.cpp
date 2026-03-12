#include "command/CommandClose.h"
#include "document/DocManager.h"
#include "utils/GlobalUtils.h"

namespace tch {

CommandClose::CommandClose() {
    // 构造函数，不需要特殊初始化
}

void CommandClose::onUpdate() {
    if (isCompleted()) {
        return;
    }
    // 关闭当前文档
    std::size_t currentIndex = DocManager::getCurrentDocumentIndex();
    if (DocManager::closeDocument(currentIndex)) {
        cmdLinePrint("Closed document. Current document: " + DocManager::getCurrentDocument().getFullFileName());
    } else {
        cmdLinePrint("Failed to close document");
    }
    
    // 命令执行完成，标记为完成状态
    finish();
}

} // namespace tch