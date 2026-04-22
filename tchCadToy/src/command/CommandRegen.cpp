// 对应头文件
#include "CommandRegen.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "DocManager.h"
#include "Document.h"
#include "LocalizationManager.h"
#include "GlobalUtils.h"


namespace tch {

CommandRegen::CommandRegen() = default;

void CommandRegen::onUpdate() {
    if (isCompleted()) {
        return;
    }
    
    auto& doc = DocManager::getCurrentDocument();
    auto* pCache = doc.getGraphicsDataCache();
    if (pCache) {
        pCache->markAllDirty();
    }
    
    auto& loc = LocalizationManager::getInstance();
    Utils::cmdLinePrint(loc.get("command.regen.completed"));
    
    finish();
}

} // namespace tch
