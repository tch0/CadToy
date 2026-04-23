// 对应头文件
#include "UndoStack.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "Database.h"
#include "Logger.h"


namespace tch {

UndoStack::UndoStack()
    : m_pDb(nullptr)
    , m_currentIndex(-1)
    , m_activeIndex(-1) {
}

UndoStack::~UndoStack() = default;

void UndoStack::setDatabase(Database* pDb) {
    m_pDb = pDb;
}

const UndoRecord* UndoStack::getCurrentRecord() const {
    if (m_currentIndex >= 0 && m_currentIndex < (int)m_records.size()) {
        return &m_records[m_currentIndex];
    }
    return nullptr;
}

const UndoRecord* UndoStack::getNextRecord() const {
    int nextIndex = m_currentIndex + 1;
    if (nextIndex >= 0 && nextIndex < (int)m_records.size()) {
        return &m_records[nextIndex];
    }
    return nullptr;
}

void UndoStack::clearEntryBackup(const UndoEntry& entry) {
    if (!m_pDb || entry.backupId == 0) {
        return;
    }

    // 只有 Modify 和 Remove 操作会在备份区创建实际数据
    // Add 操作的 backupId 只是预分配，没有实际数据
    switch (entry.type) {
        case UndoOpType::kModify:
            // Modify 操作在 backupForModify 时创建了实际备份
            m_pDb->removeBackup(entry.backupId);
            break;

        case UndoOpType::kRemove:
            // Remove 操作在 moveToBackup 时创建了实际备份
            // 删除备份 ID 对应的备份实体
            m_pDb->removeBackup(entry.backupId);
            break;

        case UndoOpType::kAdd:
            // Add 操作的 backupId 只是预分配，没有实际数据，无需清理
            break;
    }
}

void UndoStack::clearAllRecordsBackups() {
    for (const auto& record : m_records) {
        for (const auto& entry : record.entries) {
            clearEntryBackup(entry);
        }
    }
}

void UndoStack::addOrphanBackup(ObjectId backupId) {
    if (backupId != 0) {
        m_orphanBackups.push_back(backupId);
    }
}

void UndoStack::clearOrphanBackups() {
    if (!m_pDb) {
        m_orphanBackups.clear();
        return;
    }

    for (ObjectId backupId : m_orphanBackups) {
        m_pDb->removeBackup(backupId);
    }
    m_orphanBackups.clear();
}

void UndoStack::push(UndoRecord&& record) {
    // 清空当前索引之后的记录，并删除这些记录引用的备份实体
    if (m_currentIndex < (int)m_records.size() - 1) {
        // 原位遍历清理备份（从后向前避免索引问题）
        for (int i = (int)m_records.size() - 1; i > m_currentIndex; --i) {
            for (const auto& entry : m_records[i].entries) {
                clearEntryBackup(entry);
            }
        }
        m_records.erase(m_records.begin() + m_currentIndex + 1, m_records.end());
    }

    // 添加新记录
    m_records.push_back(std::move(record));
    m_currentIndex++;
}

void UndoStack::beginGroup(const std::string& name) {
    if (!m_pDb) {
        return;
    }

    // 检测嵌套 group，如果上一个没有结束则自动结束并记录警告
    if (m_activeIndex >= 0) {
        LOG_WARNING("Undo group '{}' was not properly ended, auto-ending previous group", m_records[m_activeIndex].name);
        endGroup();
    }

    // 创建新记录
    UndoRecord record;
    record.name = name;

    // push 并保存索引用于后续添加条目
    push(std::move(record));
    m_activeIndex = m_currentIndex;
}

void UndoStack::endGroup() {
    if (!m_pDb || m_activeIndex < 0) {
        return;
    }

    // 如果记录为空，移除它
    if (m_records[m_activeIndex].entries.empty()) {
        m_records.pop_back();
        m_currentIndex--;
    }

    // 清理孤儿备份
    clearOrphanBackups();

    m_activeIndex = -1;
}

void UndoStack::addEntry(UndoOpType type, ObjectId objId, ObjectId backupId) {
    if (m_activeIndex < 0 || objId == 0) {
        return;
    }

    auto& entries = m_records[m_activeIndex].entries;

    // 查找是否已有该实体的记录
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        if (it->objId == objId) {
            // 合并规则
            switch (it->type) {
                case UndoOpType::kAdd:
                    if (type == UndoOpType::kModify) {
                        // 添加 + 修改 = 添加（不重复记录）
                        return;
                    }
                    if (type == UndoOpType::kRemove) {
                        // 添加 + 删除 = 取消（删除记录）
                        // 记录 Remove 的 backupId 为孤儿备份，在 endGroup 时清理
                        addOrphanBackup(backupId);
                        entries.erase(it);
                        return;
                    }
                    break;

                case UndoOpType::kRemove:
                    // 删除后不可能再操作该实体
                    return;

                case UndoOpType::kModify:
                    if (type == UndoOpType::kModify) {
                        // 修改 + 修改 = 修改（保持第一次备份）
                        // 清理后一个 Modify 创建的备份
                        clearEntryBackup({type, objId, backupId});
                        return;
                    }
                    if (type == UndoOpType::kRemove) {
                        // 修改 + 删除 = 删除
                        // 保留 Modify 的备份，类型改为 Remove
                        // Undo 时执行 Remove 的 Undo（从备份恢复），恢复到修改前状态
                        // 记录 Remove 的 backupId 为孤儿备份，在 endGroup 时清理
                        addOrphanBackup(Database::getRemoveBackupId(objId));
                        it->type = UndoOpType::kRemove;
                        return;
                    }
                    break;
            }
        }
    }

    // 添加新条目
    entries.push_back({type, objId, backupId});
}

void UndoStack::recordAdd(ObjectId objId) {
    if (!m_pDb) {
        return;
    }

    // 预分配 backupId
    ObjectId backupId = m_pDb->allocateBackupId();
    addEntry(UndoOpType::kAdd, objId, backupId);
}

void UndoStack::recordRemove(ObjectId objId) {
    if (!m_pDb) {
        return;
    }

    // 计算删除 backupId 并记录
    ObjectId backupId = Database::getRemoveBackupId(objId);
    addEntry(UndoOpType::kRemove, objId, backupId);
}

void UndoStack::recordModify(ObjectId objId) {
    if (!m_pDb) {
        return;
    }

    // 分配 backupId 并执行备份
    ObjectId backupId = m_pDb->allocateBackupId();
    m_pDb->backupForModify(objId, backupId);

    addEntry(UndoOpType::kModify, objId, backupId);
}

void UndoStack::undo() {
    if (!canUndo() || !m_pDb) {
        return;
    }

    const UndoRecord* pRecord = getCurrentRecord();
    if (pRecord) {
        pRecord->executeUndo(m_pDb);
    }

    m_currentIndex--;
}

void UndoStack::redo() {
    if (!canRedo() || !m_pDb) {
        return;
    }

    m_currentIndex++;

    const UndoRecord* pRecord = getCurrentRecord();
    if (pRecord) {
        pRecord->executeRedo(m_pDb);
    }
}

std::string UndoStack::getUndoName() const {
    if (!canUndo()) {
        return "";
    }

    const UndoRecord* pRecord = getCurrentRecord();
    return pRecord ? pRecord->name : "";
}

std::string UndoStack::getRedoName() const {
    if (!canRedo()) {
        return "";
    }

    const UndoRecord* pRecord = getNextRecord();
    return pRecord ? pRecord->name : "";
}

void UndoStack::clear() {
    // 清除所有记录引用的备份实体
    clearAllRecordsBackups();
    // 清理孤儿备份
    clearOrphanBackups();
    m_records.clear();
    m_currentIndex = -1;
    m_activeIndex = -1;
}

} // namespace tch
