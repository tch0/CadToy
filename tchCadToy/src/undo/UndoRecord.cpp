// 对应头文件
#include "UndoRecord.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "Database.h"


namespace tch {

void UndoRecord::executeUndo(Database* pDb) const {
    if (!pDb) {
        return;
    }

    // 逆序执行条目
    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
        const UndoEntry& entry = *it;

        switch (entry.type) {
            case UndoOpType::kAdd: {
                // 添加的 Undo：移到备份区
                if (pDb->hasObject(entry.objId)) {
                    pDb->moveToBackup(entry.objId, entry.backupId);
                }
                break;
            }

            case UndoOpType::kRemove: {
                // 删除的 Undo：从备份区恢复（使用记录的 backupId）
                if (pDb->hasBackup(entry.backupId)) {
                    pDb->restoreFromBackup(entry.objId, entry.backupId);
                }
                break;
            }

            case UndoOpType::kModify: {
                // 修改的 Undo：交换
                if (pDb->hasObject(entry.objId) && pDb->hasBackup(entry.backupId)) {
                    pDb->swapWithBackup(entry.objId, entry.backupId);
                }
                break;
            }
        }
    }
}

void UndoRecord::executeRedo(Database* pDb) const {
    if (!pDb) {
        return;
    }

    // 正序执行条目
    for (const UndoEntry& entry : entries) {
        switch (entry.type) {
            case UndoOpType::kAdd: {
                // 添加的 Redo：从备份区恢复
                if (pDb->hasBackup(entry.backupId)) {
                    pDb->restoreFromBackup(entry.objId, entry.backupId);
                }
                break;
            }

            case UndoOpType::kRemove: {
                // 删除的 Redo：再次删除（移动到备份区）
                if (pDb->hasObject(entry.objId)) {
                    pDb->moveToBackup(entry.objId, entry.backupId);
                }
                break;
            }

            case UndoOpType::kModify: {
                // 修改的 Redo：再次交换
                if (pDb->hasObject(entry.objId) && pDb->hasBackup(entry.backupId)) {
                    pDb->swapWithBackup(entry.objId, entry.backupId);
                }
                break;
            }
        }
    }
}

} // namespace tch
