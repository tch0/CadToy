#pragma once

// C++ 标准库
#include <string>

// 第三方库

// 项目头文件
#include "DbCommon.h"


namespace tch {

// Undo 管理器（全局单例，无状态转发层）
// 所有逻辑都在 UndoStack 中实现，此类仅作为便捷的全局访问接口
class UndoManager {
public:
    // 获取单例实例
    static UndoManager& getInstance();

    // 开始新组（操作当前文档）
    void beginGroup(const std::string& name);

    // 结束组（操作当前文档）
    void endGroup();

    // 记录添加实体
    void recordAdd(ObjectId objId);

    // 记录删除实体
    void recordRemove(ObjectId objId);

    // 记录修改实体
    void recordModify(ObjectId objId);

    // 是否可以 Undo（当前文档）
    bool canUndo() const;

    // 是否可以 Redo（当前文档）
    bool canRedo() const;

    // 执行 Undo（当前文档）
    void undo();

    // 执行 Redo（当前文档）
    void redo();

    // 获取 Undo 命令名（当前文档）
    std::string getUndoName() const;

    // 获取 Redo 命令名（当前文档）
    std::string getRedoName() const;

    // 清空栈（当前文档）
    void clear();

private:
    // 私有构造函数
    UndoManager() = default;

    // 获取当前文档的 UndoStack
    class UndoStack& getCurrentStack() const;
};

} // namespace tch
