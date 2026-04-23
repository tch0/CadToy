#pragma once

// C++ 标准库
#include <string>
#include <vector>

// 第三方库

// 项目头文件
#include "UndoRecord.h"


namespace tch {

// 前向声明
class Database;

// Undo 栈（封装所有 Undo/Redo 逻辑）
class UndoStack {
public:
    // 默认构造函数
    UndoStack();

    // 析构函数
    ~UndoStack();

    // 设置关联的数据库（必须在开始记录前设置）
    void setDatabase(Database* pDb);

    // 是否可以 Undo
    bool canUndo() const { return m_currentIndex >= 0; }

    // 是否可以 Redo
    bool canRedo() const { return m_currentIndex < (int)m_records.size() - 1; }

    // 开始新组
    void beginGroup(const std::string& name);

    // 结束组
    void endGroup();

    // 记录添加实体
    void recordAdd(ObjectId objId);

    // 记录删除实体
    void recordRemove(ObjectId objId);

    // 记录修改实体
    void recordModify(ObjectId objId);

    // 执行 Undo
    void undo();

    // 执行 Redo
    void redo();

    // 获取 Undo 命令名
    std::string getUndoName() const;

    // 获取 Redo 命令名
    std::string getRedoName() const;

    // 清空栈
    void clear();

private:
    // 添加条目到当前活动记录（内部使用，带合并逻辑）
    void addEntry(UndoOpType type, ObjectId objId, ObjectId backupId);

    // 获取当前记录
    const UndoRecord* getCurrentRecord() const;

    // 获取下一个记录
    const UndoRecord* getNextRecord() const;

    // 添加记录（会清空当前索引之后的记录）
    void push(UndoRecord&& record);

    // 清除所有记录中引用的备份实体（用于 clear）
    void clearAllRecordsBackups();

    // 清除单个条目的备份实体，isUndoRecord=true表示这一条是undo记录，false表示这一条是redo记录
    void clearEntryBackup(const UndoEntry& entry, bool isUndoRecord);

    // 添加孤儿备份 ID（在 endGroup 时清理）
    void addOrphanBackup(ObjectId backupId);

    // 清理所有孤儿备份
    void clearOrphanBackups();

private:
    Database* m_pDb;                        // 关联的数据库
    std::vector<UndoRecord> m_records;      // 记录列表，undo/redo共用的栈，[0,m_currentIndex]是undo栈 (m_currentIndex, m_records.size()-1]是redo栈
    int m_currentIndex;                     // undo栈栈顶记录索引，-1表示undo栈为空
    int m_activeIndex;                      // 当前活动记录的索引（正在构建的组，-1 表示没有）
    std::vector<ObjectId> m_orphanBackups;  // 孤儿备份 ID 列表（合并时产生，endGroup 时清理）
};

} // namespace tch
