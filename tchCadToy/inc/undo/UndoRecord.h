#pragma once

// C++ 标准库
#include <string>
#include <vector>

// 第三方库

// 项目头文件
#include "DbCommon.h"


namespace tch {

// 前向声明
class Database;

// Undo 操作类型
enum class UndoOpType : uint8_t {
    kAdd,       // 添加实体
    kRemove,    // 删除实体
    kModify     // 修改实体
};

// Undo 条目
struct UndoEntry {
    UndoOpType type;       // 操作类型
    ObjectId objId;        // 实体 ID
    ObjectId backupId;     // 备份 ID（删除时通过公式计算）
};

// Undo 记录（一个命令组）
class UndoRecord {
public:
    std::string name;                  // 命令名称
    std::vector<UndoEntry> entries;    // 条目列表

    // 执行 Undo（逆序）
    void executeUndo(Database* pDb) const;

    // 执行 Redo（正序）
    void executeRedo(Database* pDb) const;
};

} // namespace tch
