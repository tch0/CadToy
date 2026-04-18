// 对应头文件
#include "CommandNew.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "DocManager.h"

namespace tch {

CommandNew::CommandNew() {
}

void CommandNew::onUpdate() {
    if (isCompleted()) {
        return;
    }
    
    // 创建新文档
    std::size_t newDocIndex = DocManager::createNewDocument();
    
    // 直接切换文档
    DocManager::setCurrentDocumentIndex(newDocIndex);
    
    // 命令完成
    finish();
}

} // namespace tch
